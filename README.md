# Introduction
This project is a result of me getting annoyed by scripts that were taking too long to reformat directory of somewhat big project.
Consider using it when you have > 200 code files. It improves speed of reformating all files in specified directory compared to shell script alternatives.

Code written in C++17 Standard.
Thanks to `<execution>`, I execute clang-format in parallel policy that results in actual good perfomance of that application. 

There are no special dependencies, project can be easily compiled for Windows and Unix systems. 

# Download
Pre-build download avaible thru [releases](https://github.com/GloryOfNight/clang-format-all/releases) page.

# Options
- `-S` optional, specify source directory to reformat, if not prodived program will use current working directory as source
- `-I` optional, ignore folders or files in releative paths to `-S`
- `-E` optional, specify clang-format executable path, if not provided program will try to find it from [LLVM](https://github.com/llvm/llvm-project/releases) path env variable
- `-C` optional, specify additional command-line arguments for clang-format command, for example --dry-run --Werror --style firefox
- `--verbose` enable more logging (could slow down the perfomance!)

example command (git bash, windows):

`./clang-format-all.exe -S C:/4.26.2-release/Engine/Source -E Tools/clang-format.exe -I Editor Developer ThirdParty -C "--dry-run --Werror" --verbose`

# Support
If you found this usefull, give it a star. Stars, yay ⭐

If you encounter issue, use Issues section above and write everyting about it.
