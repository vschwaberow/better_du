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

#ifndef BETTER_DU_SRC_BDU_OPTIONS_H_
#define BETTER_DU_SRC_BDU_OPTIONS_H_

#include <expected>
#include <string>
#include <vector>
#include <string_view>

namespace bdu {

constexpr std::string_view kVersionString = "0.1.0";

struct Options {
  bool all = false;
  bool grand_total = false;
  int max_depth = -1;
  bool human_readable = false;
  bool kilobytes = false;
  bool summarize = false;
  bool one_file_system = false;
  bool help_requested = false;
  bool version_requested = false;
  bool dereference_all = false;
  bool dereference_args = false;
  bool sort_by_size = true;
  bool use_color = true;
  bool show_progress = false;
  bool show_bars = true;
  int top_n = -1;
  std::vector<std::string> exclude_patterns;

  std::vector<std::string> paths;
};

using ParseResult = std::expected<Options, std::string>;

ParseResult ParseOptions(const std::vector<std::string_view>& args);

} // namespace bdu

#endif // BETTER_DU_SRC_BDU_OPTIONS_H_
