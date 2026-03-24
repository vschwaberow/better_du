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

#ifndef BETTER_DU_SRC_SIZE_FORMATTER_H_
#define BETTER_DU_SRC_SIZE_FORMATTER_H_

#include <array>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>

namespace bdu {

constexpr double kBaseMultiplier = 1024.0;

template <typename ValueType>
std::string FormatHumanReadable(ValueType bytes) {
  constexpr std::array<std::string_view, 6> kSuffixes = {
      "B", "K", "M", "G", "T", "P"};

  double size = static_cast<double>(bytes);
  size_t suffix_index = 0;

  while (size >= kBaseMultiplier && suffix_index < kSuffixes.size() - 1) {
    size /= kBaseMultiplier;
    ++suffix_index;
  }

  if (suffix_index == 0) {
    return std::format("{}{}", static_cast<std::uintmax_t>(bytes),
                       kSuffixes[suffix_index]);
  }
  return std::format("{:.1f}{}", size, kSuffixes[suffix_index]);
}

}  // namespace bdu

#endif // BETTER_DU_SRC_SIZE_FORMATTER_H_
