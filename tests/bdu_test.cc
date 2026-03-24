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

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "../src/bdu_options.h"
#include "../src/size_calculator.h"
#include "../src/size_formatter.h"

TEST(BduOptions, ParseVersion) {
    std::vector<std::string_view> args = {"bdu", "--version"};
    auto res = bdu::ParseOptions(args);
    ASSERT_TRUE(res.has_value());
    EXPECT_TRUE(res->version_requested);
}

TEST(BduOptions, ParseSummarize) {
    std::vector<std::string_view> args = {"bdu", "-s"};
    auto res = bdu::ParseOptions(args);
    ASSERT_TRUE(res.has_value());
    EXPECT_TRUE(res->summarize);
}

TEST(SizeFormatter, HumanReadable) {
    EXPECT_EQ(bdu::FormatHumanReadable(1024), "1.0K");
    EXPECT_EQ(bdu::FormatHumanReadable(1048576), "1.0M");
    EXPECT_EQ(bdu::FormatHumanReadable(500), "500B");
}

TEST(SizeCalculator, BasicTraversal) {
    std::filesystem::path temp_dir = std::filesystem::temp_directory_path() / "bdu_test_dir";
    std::filesystem::create_directories(temp_dir / "subdir");
    std::ofstream(temp_dir / "file1.txt").put('a');
    std::ofstream(temp_dir / "subdir" / "file2.txt").put('b');

    bdu::Options opts;
    opts.paths = {temp_dir.string()};
    opts.summarize = true;

    auto res = bdu::RunDiskUsage(opts);
    EXPECT_TRUE(res.has_value());

    std::filesystem::remove_all(temp_dir);
}
