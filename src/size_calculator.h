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

#ifndef BETTER_DU_SRC_SIZE_CALCULATOR_H_
#define BETTER_DU_SRC_SIZE_CALCULATOR_H_

#include <expected>
#include <system_error>

#include "bdu_options.h"

namespace bdu {

std::expected<void, std::error_code> RunDiskUsage(const Options &opts);

} // namespace bdu

#endif // BETTER_DU_SRC_SIZE_CALCULATOR_H_
