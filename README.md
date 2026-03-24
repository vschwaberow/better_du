# better_du (bdu)

A fast, modern, and feature-rich disk usage analyzer written in C++23. It aims to be a robust alternative to `du`, providing extreme performance through concurrency architecture and visual terminal outputs.

## Features

- **Blazing Fast**: Uses a wait-free dependency graph, cache-aligned inode shards, and a core thread pool with MPSC queues for high-performance concurrent directory traversal.
- **Visual Output**: Displays a graphical tree UI, ASCII bar charts (`--bars`), and ANSI colorized output based on size (`--color`).
- **Advanced Filtering**: Exclude files by regex pattern (`--exclude=PATTERN`), show only the top N results (`--top=N`), and sort by size (`--sort`).
- **Standard `du` Command Line**: Supports flags like `-a` (all files), `-c` (grand total), `-d` (max depth), `-h` (human-readable), `-k` (kilobytes), `-s` (summarize), and `-x` (one file system).
- **Progress Indicator**: Animated scanning indicator (`--progress`) for long-running analyses.

## Build Instructions

### Prerequisites
- CMake 3.14 or higher
- C++23 compatible compiler (e.g., GCC 13+, Clang 16+)

### Building
```bash
mkdir build
cd build
cmake ..
cmake --build . -j
```
The compiled binary `bdu` will be available in the `build` directory.

## Testing

The project uses Google Test. Tests are downloaded automatically by CMake via FetchContent.

To run the test suite:
```bash
cd build
ctest --output-on-failure
```

## Usage

```bash
bdu [OPTION]... [FILE]...

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
  --top=N       print only the N largest items in the specified depth
```

## License

This project is licensed under the GNU General Public License v3.0 (GPLv3). See the [LICENSE](LICENSE) file for more details.
