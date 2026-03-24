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

#include <string>

#include "bdu_options.h"
#include "bdu_print.h"
#include <vector>
#include <string_view>
#include "size_calculator.h"

int main(int argc, char** argv) {
  std::vector<std::string_view> args(argv, argv + argc);
  auto opts_result = bdu::ParseOptions(args);
  if (!opts_result) {
    bdu::println(std::cerr, "bdu: {}\nTry 'bdu --help' for more information.", opts_result.error());
    return 1;
  }

  const auto& opts = opts_result.value();
  if (opts.help_requested) {
    constexpr std::string_view kHelpMessage = R"(Usage: bdu [OPTION]... [FILE]...
Summarize disk usage of the set of FILEs, recursively for directories.
Options:
  -a            write counts for all files, not just directories
  -c            produce a grand total
  -d, --max-depth=N
                print the total for a directory (or file, with -a) only if it
                is N or fewer levels below the command line argument
  -h            print sizes in human readable format (e.g., 1K 234M 2G)
  -H            dereference command line symlinks
  -k            like --block-size=1K
  -L            dereference all symlinks
  -s            display only a total for each argument
  -x            skip directories on different file systems
  --help        display this help and exit
Advanced:
  --color       colorize output based on size (red >1GB, yellow >1MB, green)
  --exclude=PAT ignore files matching the regex pattern PAT
  --progress    show an animated scanning indicator
  --sort        sort the output by size ascending
  --top=N       print only the N largest items in the specified depth)";
    
    bdu::println(std::cout, "{}", kHelpMessage);
    return 0;
  }
  
  if (opts.version_requested) {
    bdu::println(std::cout, "better_du version {}", bdu::kVersionString);
    return 0;
  }

  auto run_result = bdu::RunDiskUsage(opts_result.value());
  if (!run_result.has_value()) {
    return 1;
  }

  return 0;
}
