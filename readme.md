## [llconv](https://github.com/matgat/llconv.git)
This is a conversion tool between these formats:
* `.h` (Sipro source header)
* `.pll` (Plc LogicLab3 Library)
* `.plclib` (LogicLab5 PLC LIBrary)

Sipro header files resemble a *c-like* header containing just
`#define` directives.
LogicLab libraries are *text* files embedding *IEC 61131-3* ST code;
the latest format (`.plclib`) embraces a pervasive *xml* structure.

The supported transformations are:
* `.h` → `.pll`, `.plclib`
* `.pll` → `.plclib`

This tool aims to be very fast and efficient:
in my case it takes less than 0.08 s to parse
13 files (2.8 MB) and generate 15 converted files (3.5 MB).


_________________________________________________________________________
## Limitations
To be as fast as possible there's a price to pay: to minimize
dynamic memory allocations the raw content of the input is directly
written on the output (working mostly with `std::string_view`),
so no chance to fix invalid characters or to convert encodings and
line breaks.
This introduces some limitations on the accepted input files.

A couple of draconian choices have been made:

* Support just `UTF-8` encoding\
  _Rationale: It's just the only sane choice
              since encodings based on bigger chars
              have only disadvantages:
              much more size (sources are mostly `ASCII`),
              slower parsing, dealing with endianness,
              less compatibility with external tools.
              `8-bit ANSI` encodings will work but will
              introduce a dependency on the codepage
              and should be avoided_

* Support just _unix_ line breaks (`LF`, `\n`)\
  _Rationale: Two-chars lines breaks are just a waste of space
              and parsing time. There's really no reason for
              them even in *Windows*, unless you're stuck with
              `notepad.exe` or an ancient teletype device_

### Input files
Input files must:
* Be syntactically correct
* Be encoded in `UTF-8`
* Have unix line breaks (`\n`)
* Descriptions (`.h` `#define` inlined comments and `.pll` `{DE: ...}`) cannot contain XML special characters nor line breaks

### _IEC 61131-3_ syntax
* Parser is case sensitive (uppercase keywords)
* Not supported:
  * Multidimensional arrays like `ARRAY[1..2, 1..2]`
  * `RETAIN` specifier
  * Pointers
  * `IFDEF` LogicLab extension for conditional compilation
  * Latest standard (`INTERFACE`, `THIS`, `PUBLIC`, `IMPLEMENTS`, `METHOD`, …)


_________________________________________________________________________
## `.h` files
Sipro header files supported syntax is:
```
// line comment
/*  -------------
    block comment
    ------------- */
#define reg_label register // description
#define val_label value // [type] description
```
Inlined comments (defines description) cannot contain XML special characters.

Sipro registers will be recognized and exported:
```
#define vqPos vq100 // Position
```

It's possible to export also numeric constants
declaring their *IEC 61131-3* type as in:
```
#define PI 3.14159 // [LREAL] Circumference/diameter ratio
```

The recognized types are:

| type        | description                 | size | range                     |
| ----------- | --------------------------- | ---- | ------------------------- |
|   `BOOL`    | *BOOLean*                   |  1   | FALSE/TRUE                |
|   `SINT`    | *Short INTeger*             |  1   | -128 … 127                |
|   `INT`     | *INTeger*                   |  2   | -32768 … 32767            |
|   `DINT`    | *Double INTeger*            |  4   |  -2147483648 … 2147483647 |
| ~~`LINT`~~  | ~~*Long INTeger*~~          |  8   | -2⁶³ … 2⁶³-1              |
|   `USINT`   | *Unsigned Short INTeger*    |  1   | 0 … 255                   |
|   `UINT`    | *Unsigned INTeger*          |  2   | 0 … 65535                 |
|   `UDINT`   | *Unsigned Double INTeger*   |  4   | 0 … 4294967295            |
| ~~`ULINT`~~ | ~~*Unsigned Long INTeger*~~ |  8   | 0 … 18446744073709551615  |
| ~~`REAL`~~  | ~~*REAL number*~~           |  4   | ±10³⁸                     |
|   `LREAL`   | *Long REAL number*          |  8   | ±10³⁰⁸                    |
|   `BYTE`    | *1 byte*                    |  1   | 0 … 255                   |
|   `WORD`    | *2 bytes*                   |  2   | 0 … 65535                 |
|   `DWORD`   | *4 bytes*                   |  4   | 0 … 4294967295            |
| ~~`LWORD`~~ | ~~*8 bytes*~~               |  8   | 0 … 18446744073709551615  |


_________________________________________________________________________
## `.pll` files
Authors are encouraged to include custom additional library data in the first comment of the `.pll` file.
The recognized fields are:

```
(*
    descr: Machine logic
    version: 1.2.31
*)
```


_________________________________________________________________________
## Usage
To print usage info:
```
$ llconv -help
```

Input files can be a mixed set of `.h` and `.pll`.
The `.h` files will generate both formats `.pll` and `.plclib`.

Transform all `.h` files in the current directory into a given output directory:
```
$ llconv -fussy -verbose *.h -output plc/LogicLab/generated-libs
```

Transform all `.h` files in `prog/` and all `.pll` files in `plc/`,
deleting existing files in the output folder,
sorting objects by name and indicating the `plclib` schema version:
```
$ llconv -fussy -options sort:by-name,schemaver:2.8 prog/*.h plc/*.pll -clear -output plc/LogicLab/generated-libs
```
Parsing issues will be reported in `*.log` files in
the output folder. In case of critical errors the program
will try to open the offending file with the associated
program (passing `-nosession -p <offset>`, so I'm really
just supporting `notepad++` at this time: for the
best experience it's recommended to associate both `*.h`
and `*.pll` extensions to it).


_________________________________________________________________________
## Build
```
$ git clone https://github.com/matgat/llconv.git
$ cd llconv
$ make stuff/makefile
$ g++ -std=c++2b -funsigned-char -Wall -Wextra -Wpedantic -Wconversion -O3 -DFMT_HEADER_ONLY -Isource/fmt/include -o "llconv" "source/llconv.cpp"
```
In windows just use the latest Microsoft Visual Studio Community.
From the command line, something like:
```
> msbuild msvc/llconv.vcxproj -t:llconv -p:Configuration=Release
```
The project will call the compiler as:
```
> cl /std:c++latest /utf-8 /J /W4 /O2 /D_CRT_SECURE_NO_WARNINGS /DFMT_HEADER_ONLY /I../source/fmt/include "source/llconv.cpp" /link /out:llconv.exe
```
