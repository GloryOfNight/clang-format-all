> [!WARNING]
> That project lost its purpose since clang-format introduced .clang-format-ignore and --files commandline option. You can find better way of doing things below.
>
> I hope even so, you might find some useful stuff in that repo to help you out.

> [!NOTE]
> Project served it's time. If you want to perform formatting of many files you can do it better with clang-format right now using scripts and files. Sample just below. 

Script gathers all .cpp and .h files, put them into files.temp and feeds it to clang-format that does good job of reformating them.
```bash
### clang-format-all.sh ###
#!/bin/bash

# Create a temporary file to store the list of .cpp and .h files
temp_file="files.temp"

# Find all .cpp and .h files recursively from the current directory
find . -type f \( -name "*.cpp" -o -name "*.h" \) > "$temp_file"

# Format all .cpp and .h files
clang-format --files files.temp -i

# Remove the temporary file
rm -f "$temp_file"
```

If you need to ignore files, consider using [.clang-format-ignore](https://clang.llvm.org/docs/ClangFormat.html#clang-format-ignore). Additionally you can disable formatting for centain directories by placing .clang-format with [DisableFormat](https://clang.llvm.org/docs/ClangFormatStyleOptions.html#disableformat) options turned on there.

> [!NOTE]
> Old project description below

# Introduction
This tool will help you reformat C++ project directory (or any directory containing Cxx files) using clang-format in fast multithreaded manor.
Typical scripts are not very effective at reformatting huge amounts of files at one go. This tool solves the issue by using features of C++17-20 standard.

There are no special dependencies, project can be easily compiled for Windows and Unix systems. 

# Download
Pre-build download available thru [releases](https://github.com/GloryOfNight/clang-format-all/releases) page.

# Build
On Linux: `cmake --preset linux64-release && cmake --build build` NOTE: you might need to install libtbb for successfull build!

On Windows: `cmake --preset win64-release && cmake --build build`

# Options
- `--help` print help with available arguments list
- `-S` optional, specify source directory to reformat, if not provided program will use current working directory as source
- `-I` optional, ignore folders or files in relative paths to `-S`
- `-E` optional, specify clang-format executable path, if not provided program will try to find it from [LLVM](https://github.com/llvm/llvm-project/releases) path env variable
- `-C` optional, specify additional command-line arguments for clang-format command, for example --dry-run --Werror --style firefox
- `--verbose` enable more logging (could slow down the performance!)
- `--no-logs` disable console output (could slightly improve the performance)

example command (git bash, windows):

`./clang-format-all.exe -S C:/4.26.2-release/Engine/Source -E Tools/clang-format.exe -I Editor Developer ThirdParty -C "--dry-run --Werror" --verbose`

# Support
If you found this useful, give it a star. Stars, yay ⭐

If you encounter issue, use Issues section above and write everything about it.


### Main branch
[![Windows](https://github.com/GloryOfNight/clang-format-all/actions/workflows/windows.yml/badge.svg)](https://github.com/GloryOfNight/clang-format-all/actions/workflows/windows.yml)
[![Linux](https://github.com/GloryOfNight/clang-format-all/actions/workflows/linux.yml/badge.svg)](https://github.com/GloryOfNight/clang-format-all/actions/workflows/linux.yml)
