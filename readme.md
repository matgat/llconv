# [llconv](https://github.com/matgat/llconv.git)
This is a conversion utility between these formats:
* `*.h` (Sipro *#defines* file)
* `*.pll` (LogicLab3 library file)
* `*.plclib` (LogicLab5 library file)

Sipro `*.h` files resemble a c-like header with `#define` directives.
LogicLab library files are xml containers of IEC 61131-3 ST code.

The supported transformations are:
* `*.h` → `*.pll`, `*.plclib`
* `*.pll` → `*.plclib`


## Limitations
Input files must:
* Be encoded in ANSI or UTF-8
* Have strictly unix line breaks (`\n`)
* PLL descriptions (`{DE: ...}`) cannot contain line breaks or XML special characters


## Additional data in PLL files
Some additional library data will be extracted in the
first comment of the PLL file:
```
(*
    descr: Machine logic stuff
    version: 0.5.1
    author: Foo bar
    dependencies: defvar.pll, iomap.pll
    machine: StratoW
*)
```


## Usage
To print some info:
```
$ llconv -help
```

To transform all `*.h` files in the current directory to `*.pll` into `LogicLab/generated-libs`:
```
$ llconv -fussy *.h -output LogicLab/generated-libs
```

To transform all `*.pll` files in the current directory to `*.plclib` into `LogicLab/generated-libs`:
```
$ llconv -fussy -schemaver 2.8 *.pll -output LogicLab/generated-libs
```

Input files can be a mixed set of `*.h` and `*.pll`.
The `*.h` files will generate both formats `*.pll` and `*.plclib`.


## Build
```
$ git clone https://github.com/matgat/llconv.git
$ cd llconv/source
$ g++ -std=c++2b -funsigned-char -Wextra -Wall -pedantic -O3 -DFMT_HEADER_ONLY -Isource/fmt/include -o "llconv" "llconv.cpp"
```
