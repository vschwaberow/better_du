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

#include "bdu_options.h"
#include <charconv>
#include <string_view>
#include <system_error>

namespace bdu {

namespace {
std::expected<int, std::string> ParseInt(std::string_view str) {
  int val = 0;
  auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), val);
  if (ec != std::errc() || ptr != str.data() + str.size()) {
    return std::unexpected("Invalid integer format");
  }
  return val;
}
} // namespace

ParseResult ParseOptions(const std::vector<std::string_view> &args) {
  Options opts;

  if (args.empty()) {
    opts.paths.push_back(".");
    return opts;
  }

  for (size_t i = 1; i < args.size(); ++i) {
    std::string_view arg = args[i];

    if (arg.empty())
      continue;

    if (arg == "-v" || arg == "--version") {
      opts.version_requested = true;
      return opts;
    }

    if (arg[0] == '-' && arg.size() > 1 && arg != "--") {
      if (arg == "--help") {
        opts.help_requested = true;
        return opts;
      }
      if (arg == "--sort") {
        opts.sort_by_size = true;
        continue;
      }
      if (arg == "--color") {
        opts.use_color = true;
        continue;
      }
      if (arg == "--bars") {
        opts.show_bars = true;
        continue;
      }
      if (arg == "--progress") {
        opts.show_progress = true;
        continue;
      }
      if (arg.starts_with("--top=")) {
        auto val = ParseInt(arg.substr(6));
        if (!val)
          return std::unexpected("Invalid argument for --top");
        opts.top_n = val.value();
        continue;
      }
      if (arg.starts_with("--exclude=")) {
        opts.exclude_patterns.push_back(std::string(arg.substr(10)));
        continue;
      }
      if (arg.starts_with("--max-depth=")) {
        auto val = ParseInt(arg.substr(12));
        if (!val)
          return std::unexpected("Invalid argument for --max-depth");
        opts.max_depth = val.value();
        continue;
      }
      if (arg.starts_with("--block-size=")) {
        // Ignored for identical logic
        continue;
      }
      if (arg == "--max-depth") { // This option still exists for compatibility
                                  // with old style
        if (i + 1 >= args.size())
          return std::unexpected("Option --max-depth requires an argument");
        auto val = ParseInt(args[++i]);
        if (!val)
          return std::unexpected("Invalid argument for --max-depth");
        opts.max_depth = val.value();
        continue;
      }

      for (size_t j = 1; j < arg.size(); ++j) {
        switch (arg[j]) {
        case 'a':
          opts.all = true;
          break;
        case 'c':
          opts.grand_total = true;
          break;
        case 'h':
          opts.human_readable = true;
          break;
        case 'k':
          opts.kilobytes = true;
          break;
        case 's':
          opts.summarize = true;
          break;
        case 'x':
          opts.one_file_system = true;
          break;
        case 'L':
          opts.dereference_all = true;
          break;
        case 'H':
          opts.dereference_args = true;
          break;
        case 'd': {
          if (j + 1 < arg.size()) {
            auto val = ParseInt(arg.substr(j + 1));
            if (!val)
              return std::unexpected("Invalid argument for -d");
            opts.max_depth = val.value();
            j = arg.size();
          } else if (i + 1 < args.size()) {
            auto val = ParseInt(args[++i]);
            if (!val)
              return std::unexpected("Invalid argument for -d");
            opts.max_depth = val.value();
          } else {
            return std::unexpected("Option -d requires an argument");
          }
          break;
        }
        default:
          return std::unexpected("Unknown option: -" + std::string(1, arg[j]));
        }
      }
    } else {
      opts.paths.emplace_back(arg);
    }
  }

  if (opts.summarize) {
    opts.max_depth = 0;
  }

  if (opts.paths.empty()) {
    opts.paths.push_back(".");
  }

  return opts;
}

} // namespace bdu
