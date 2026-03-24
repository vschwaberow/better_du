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

#ifndef BETTER_DU_SRC_BDU_PRINT_H_
#define BETTER_DU_SRC_BDU_PRINT_H_

#include <iostream>
#include <string_view>
#include <format>

namespace bdu {

template <typename... Args>
void println(std::ostream& os, std::format_string<Args...> fmt, Args&&... args) {
    os << std::format(fmt, std::forward<Args>(args)...) << "\n";
}

template <typename... Args>
void println(std::format_string<Args...> fmt, Args&&... args) {
    println(std::cout, fmt, std::forward<Args>(args)...);
}

} // namespace bdu

#endif // BETTER_DU_SRC_BDU_PRINT_H_
