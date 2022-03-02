# [llconv](https://github.com/matgat/llconv.git)
This is a conversion utility between these formats:
* `*.h` (Sipro *#defines* file)
* `*.pll` (LogicLab3 library file)
* `*.plclib` (LogicLab5 library file)

Sipro `*.h` files resemble a c-like header with `#define` directives.
LogicLab library files are proprietary containers of IEC 61131-3 ST code,
the latest (`*.plclib`) are xml based.

The supported transformations are:
* `*.h` → `*.pll`, `*.plclib`
* `*.pll` → `*.plclib`


## Limitations
Input files must:
* Be encoded in ANSI or UTF-8
* Have strictly unix line breaks (`\n`)
* PLL descriptions (`{DE: ...}`) cannot contain line breaks or XML special characters


## Additional data in PLL files
Some additional library data will be extracted from the
first comment of the PLL file:
```
(*
    descr: Machine logic stuff
    version: 0.5.1
*)
```


## Usage
To print some info:
```
$ llconv -help
```

Input files can be a mixed set of `*.h` and `*.pll`.
The `*.h` files will generate both formats `*.pll` and `*.plclib`.

To transform all `*.h` files in the current directory into a given output directory:
```
$ llconv -fussy *.h -output LogicLab/generated-libs
```

Similarly, to transform all files in the current directory:
```
$ llconv -fussy -schemaver 2.8 *.h *.pll -output LogicLab/generated-libs
```


## Build
```
$ git clone https://github.com/matgat/llconv.git
$ cd llconv
$ make stuff/makefile
$ g++ -std=c++2b -funsigned-char -Wextra -Wall -pedantic -O3 -DFMT_HEADER_ONLY -Isource/fmt/include -o "llconv" "source/llconv.cpp"
Not so sure about these:
> msbuild msvc/llconv.vcxproj -t:llconv -p:Configuration=Release
> cl /std:c++latest /utf-8 /J /W4 /O2 /D_CRT_SECURE_NO_WARNINGS /DFMT_HEADER_ONLY /I../source/fmt/include "source/llconv.cpp" /link /out:llconv.exe
```
