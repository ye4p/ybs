# Build system that prioritizes simplicity and ease of use.


REQUIREMENTS:
    [-] GCC installed
    [-] ts installed

How to use:

Ybs.txt reads stuff about your project and that's where it gets info about what files to build, with what compiler flags, how to run, which tests to run etc.

Ybs.txt req syntax:

The very first line is compiler, then list of C++ files to compile _in braces_, then optional list of compiler flags, also _in braces_, followed by the name you want your executable to be.
It could look like this:

```
gcc [main.cpp src/my_class.cpp] [-std=c++23 -Wall -Wextra -Werror] project
```

Unlike cmake it saves executable in your current directory not in the build folder.


ybs build
 - this will build the project

ybs run
 - build and run [args...]

ybs test
 - build and run tests(if you set them up)

running ybs with no arguments will run build by default.

