# Introduction
100% overkill and 20% cooler way to reformat your entire code base resulting in 313% overall stupidness of this project.
This project is a result of me getting annoyed by scripts that was too long to reformat directory.

Code written in C++17 Standard.
Thanks to `<execution>`, i can execute clang-format in parallel policy that results in actual good perfomance of that application. 

There are no special dependencies, project can be build and run on: Windows and Unix. 

Demo of me formatting Unreal Engine 4.26.2 `Engine/Source` folder and also providing args to ignore folders `Editor Developers . . .`

[Watch Demo](https://youtu.be/9gjA-pANNsA)

# Prerequisites
Please make sure you have installed [clang](https://github.com/llvm/llvm-project/releases) and set LLVM environment variable.
You can also provide command line argument for clang-format executable manually. Please see options section below.

# Options
- `--verbose` enable a little bit more detail logging
- `-S` optional, specify source directory to reformat, if not prodived program will use current working directory as source
- `-E` optional, specify clang-format executable, if not provided program will try to find it
- `-I` optional, ignore folders or files in releative paths to `-S`

example command (git bash, windows):

`./clang-format-all.exe -S C:/4.26.2-release/Engine/Source -E "C:\Program Files\LLVM\bin\clang-format.exe" -I Editor Developer ThirdParty --verbose`

# Support
If you found this usefull, give it a star. Stars, yay ⭐

If you encounter issue, use Issues section above and write everyting about it.
