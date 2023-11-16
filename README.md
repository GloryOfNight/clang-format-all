# Introduction
This tool will help you reformat C++ project directory (or any directory containing Cxx files) using clang-format in fast multithreaded manor.
Typical scripts are not very effective at reformatting huge amounts of files at one go. This tool solves the issue by using features of C++17 standard.

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
