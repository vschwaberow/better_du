// better_du - Disk usage analyzer
// Copyright (C) 2026 Volker Schwaberow <volker@schwaberow.de>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "size_calculator.h"

#include <filesystem>
#include <vector>
#include <string>
#include <system_error>
#include <optional>
#include <algorithm>
#include <regex>
#include <iostream>
#include <unordered_set>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <memory>
#include <ranges>
#include <functional>


#include "bdu_print.h"
#include "size_formatter.h"

namespace bdu {

namespace {

constexpr std::uintmax_t kKilobyte = 1024;
constexpr std::uintmax_t kStandardBlockSize = 512;
constexpr std::uintmax_t kMegabyte = 1024 * 1024;
constexpr std::uintmax_t kGigabyte = 1024 * 1024 * 1024;

#ifdef __cpp_lib_hardware_interference_size
    constexpr size_t kCacheLineSize = std::hardware_destructive_interference_size;
#else
    constexpr size_t kCacheLineSize = 64;
#endif

class ThreadPool {
public:
    ThreadPool(size_t threads) : stop_(false) {
        for(size_t i = 0; i < threads; ++i) {
            workers_.emplace_back([this] {
                while(true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex_);
                        condition_.wait(lock, [this]{ return stop_ || !tasks_.empty(); });
                        if(stop_ && tasks_.empty()) return;
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    task();
                }
            });
        }
    }
    
    template<class F>
    void enqueue(F&& f) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            tasks_.emplace(std::forward<F>(f));
        }
        condition_.notify_one();
    }
    
    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            stop_ = true;
        }
        condition_.notify_all();
        for(std::thread &worker: workers_) {
            worker.join();
        }
    }
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    bool stop_;
};

template <typename T>
class ThreadSafeQueue {
    std::mutex mutex_;
    std::queue<T> queue_;
public:
    void push(T value) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(value));
    }
    bool pop(T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) return false;
        value = std::move(queue_.front());
        queue_.pop();
        return true;
    }
};



struct DirNode {
    std::string path;
    const Options* opts;
    std::atomic<std::uintmax_t> size{0};
    std::atomic<int> pending_children{1};
    std::shared_ptr<DirNode> parent;
    int depth;
    
    DirNode(std::string p, const Options* o, std::shared_ptr<DirNode> par, int d) 
        : path(std::move(p)), opts(o), parent(std::move(par)), depth(d) {}
};

void FinalizeNode(std::shared_ptr<DirNode> node, ThreadSafeQueue<std::shared_ptr<DirNode>>& ui_queue) {
    int remaining = --node->pending_children;
    if (remaining == 0) {
        ui_queue.push(node);
        if (node->parent) {
            node->parent->size.fetch_add(node->size.load(std::memory_order_relaxed), std::memory_order_relaxed);
            FinalizeNode(node->parent, ui_queue);
        }
    }
}

void ProcessDirectorySTD(const std::string& path, std::shared_ptr<DirNode> node, ThreadPool& pool, ThreadSafeQueue<std::shared_ptr<DirNode>>& ui_queue, const std::vector<std::regex>& exclude_regexes) {
    std::error_code ec;
    auto iter = std::filesystem::directory_iterator(path, std::filesystem::directory_options::skip_permission_denied, ec);
    if (!ec) {
        std::uintmax_t immediate_local_sum = 4096;
        for (const auto& entry : iter) {
            std::string name = entry.path().filename().string();
            bool excluded = false;
            for (const auto& re : exclude_regexes) {
                if (std::regex_search(name, re)) {
                    excluded = true;
                    break;
                }
            }
            if (excluded) continue;

            if (entry.is_directory(ec)) {
                node->pending_children++;
                std::string child_path = entry.path().string();
                auto child_node = std::make_shared<DirNode>(child_path, node->opts, node, node->depth + 1);
                pool.enqueue([child_path, child_node, &pool, &ui_queue, exclude_regexes]() {
                    ProcessDirectorySTD(child_path, child_node, pool, ui_queue, exclude_regexes);
                });
            } else {
                immediate_local_sum += entry.file_size(ec);
            }
        }
        node->size.fetch_add(immediate_local_sum, std::memory_order_relaxed);
    }
    FinalizeNode(node, ui_queue);
}

struct ResultEntry {
    std::uintmax_t size;
    std::string path;
};

struct MinHeapCompare {
    bool operator()(const ResultEntry& a, const ResultEntry& b) const {
        return a.size > b.size; 
    }
};

std::string FormatSizeWrapper(std::uintmax_t size_bytes, const Options& opts) {
  if (opts.human_readable) {
    return FormatHumanReadable(size_bytes);
  } else if (opts.kilobytes) {
    return std::to_string((size_bytes + kKilobyte - 1) / kKilobyte);
  } else {
    return std::to_string((size_bytes + kStandardBlockSize - 1) / kStandardBlockSize); 
  }
}

void PrintColorizedEntry(std::uintmax_t size_bytes, const std::string& path_str, const Options& opts, std::uintmax_t max_size = 0) {
  std::string size_str = FormatSizeWrapper(size_bytes, opts);
  
  std::string bar_str;
  if (opts.show_bars && max_size > 0) {
    const int kBarWidth = 20;
    int blocks = static_cast<int>((static_cast<double>(size_bytes) / max_size) * kBarWidth);
    bar_str = "│";
    for (int i = 0; i < kBarWidth; ++i) {
      if (i < blocks) bar_str += "█";
      else bar_str += "░";
    }
    bar_str += "│ ";
  }

  if (opts.use_color) {
    std::string color;
    if (size_bytes > kGigabyte) {
      color = "\033[31m";
    } else if (size_bytes > kMegabyte) {
      color = "\033[33m";
    } else {
      color = "\033[32m";
    }
    bdu::println(std::cout, "{}{}\033[0m\t{}{}", color, size_str, bar_str, path_str);
  } else {
    bdu::println(std::cout, "{}\t{}{}", size_str, bar_str, path_str);
  }
}

struct TreeDisplayNode {
    std::string name;
    std::uintmax_t size{0};
    std::vector<std::unique_ptr<TreeDisplayNode>> children;
};

void CompressTree(TreeDisplayNode* node) {
    if (node->children.size() == 1 && node->size == 0) {
        node->name += (node->name.empty() || node->name.back() == '/' ? "" : "/") + node->children[0]->name;
        if (node->children[0]->size > 0) node->size = node->children[0]->size;
        auto grandchild = std::move(node->children[0]->children);
        node->children = std::move(grandchild);
        CompressTree(node);
    } else {
        for (auto& child : node->children) {
            CompressTree(child.get());
        }
    }
}

void RenderTree(TreeDisplayNode* node, const std::string& prefix, bool is_last, const Options& opts, std::uintmax_t max_size) {
    if (!node) return;
    
    std::string branch = "";
    std::string next_prefix = prefix;
    if (!node->name.empty() && node->name != "/") {
        branch = prefix + (is_last ? "└── " : "├── ");
        next_prefix += (is_last ? "    " : "│   ");
    }
    
    if (node->size > 0 || !node->children.empty()) {
        std::string display_path = branch + node->name;
        PrintColorizedEntry(node->size, display_path, opts, max_size);
    }
    
    std::ranges::sort(node->children, [](const auto& a, const auto& b) {
        return a->size > b->size;
    });
    
    for (size_t i = 0; i < node->children.size(); ++i) {
        RenderTree(node->children[i].get(), next_prefix, i == node->children.size() - 1, opts, max_size);
    }
}

} // namespace

std::expected<void, std::error_code> RunDiskUsage(const Options& opts) {
  std::vector<std::regex> exclude_regexes;
  for (const auto& pat : opts.exclude_patterns) {
      exclude_regexes.emplace_back(pat);
  }

  unsigned int hw_threads = std::thread::hardware_concurrency();
  ThreadSafeQueue<std::shared_ptr<DirNode>> ui_queue;
  ThreadPool pool(hw_threads == 0 ? 4 : hw_threads);

  auto super_root = std::make_shared<DirNode>("", &opts, nullptr, -1);
  super_root->pending_children = opts.paths.size();

  for (const auto& p : opts.paths) {
    auto path_root = std::make_shared<DirNode>(p, &opts, super_root, 0);
    pool.enqueue([p_str = p, path_root, &pool, &ui_queue, exclude_regexes]() {
        ProcessDirectorySTD(p_str, path_root, pool, ui_queue, exclude_regexes);
    });
  }

  bool traversal_done = false;
  std::vector<ResultEntry> buffered_entries;
  std::priority_queue<ResultEntry, std::vector<ResultEntry>, MinHeapCompare> top_k_heap;
  std::uintmax_t items_scanned = 0;

  while(true) {
      std::shared_ptr<DirNode> completed_node;
      while (ui_queue.pop(completed_node)) {
          if (completed_node == super_root) {
              traversal_done = true;
              continue;
          }
          
          if (opts.max_depth < 0 || completed_node->depth <= opts.max_depth || completed_node->depth == 0) {
              ResultEntry entry{completed_node->size.load(std::memory_order_relaxed), completed_node->path};
              
              if (opts.top_n > 0) {
                  top_k_heap.push(entry);
                  if (top_k_heap.size() > static_cast<size_t>(opts.top_n)) {
                      top_k_heap.pop();
                  }
              } else if (opts.sort_by_size || opts.show_bars) {
                  buffered_entries.push_back(entry);
              } else {
                  if (opts.show_progress) std::cout << "\r\033[K";
                  PrintColorizedEntry(entry.size, entry.path, opts);
              }
          }
          
          items_scanned++;
          if (opts.show_progress && items_scanned % 100 == 0) {
              std::string trunc_path = completed_node->path;
              if (trunc_path.length() > 60) trunc_path = trunc_path.substr(0, 57) + "...";
              std::cout << "\r\033[KScanning: " << items_scanned << " dirs (" << trunc_path << ")" << std::flush;
          }
      }
      
      if (traversal_done) break;
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  if (opts.show_progress) {
      std::cout << "\r\033[K" << std::flush;
  }

  if (opts.top_n > 0) {
      while (!top_k_heap.empty()) {
          buffered_entries.push_back(top_k_heap.top());
          top_k_heap.pop();
      }
      std::reverse(buffered_entries.begin(), buffered_entries.end()); 
  } else if (opts.sort_by_size) {
      std::sort(buffered_entries.begin(), buffered_entries.end(), [](const auto& a, const auto& b) {
          return a.size < b.size;
      });
  }

  std::uintmax_t max_size = 0;
  if (opts.show_bars && !buffered_entries.empty()) {
      auto max_entry = std::ranges::max_element(buffered_entries, {}, &ResultEntry::size);
      if (max_entry != buffered_entries.end()) max_size = max_entry->size;
  }

  if (opts.top_n > 0 || opts.sort_by_size || opts.show_bars) {
      std::unique_ptr<TreeDisplayNode> tree_root = std::make_unique<TreeDisplayNode>();
      for (const auto& e : buffered_entries) {
          if (e.path.empty()) continue;
          std::filesystem::path p(e.path);
          TreeDisplayNode* current = tree_root.get();
          for (const auto& part : p) {
              std::string part_str = part.string();
              TreeDisplayNode* next = nullptr;
              for (auto& child : current->children) {
                  if (child->name == part_str) {
                      next = child.get();
                      break;
                  }
              }
              if (!next) {
                  auto new_node = std::make_unique<TreeDisplayNode>();
                  new_node->name = part_str;
                  current->children.push_back(std::move(new_node));
                  next = current->children.back().get();
              }
              current = next;
          }
          current->size = e.size;
      }
      
      for (auto& child : tree_root->children) {
          CompressTree(child.get());
      }
      
      std::ranges::sort(tree_root->children, [](const auto& a, const auto& b) {
          return a->size > b->size;
      });
      
      for (size_t i = 0; i < tree_root->children.size(); ++i) {
          RenderTree(tree_root->children[i].get(), "", i == tree_root->children.size() - 1, opts, max_size);
      }
  }

  if (opts.grand_total) {
      PrintColorizedEntry(super_root->size.load(std::memory_order_relaxed), "total", opts);
  }

  return {};
}

}  // namespace bdu
