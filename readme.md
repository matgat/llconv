## [llconv](https://github.com/matgat/llconv.git)
This is a conversion utility between these formats:
* `.h` (Sipro source header)
* `.pll` (Plc LogicLab3 Library)
* `.plclib` (LogicLab5 PLC LIBrary)

Sipro `.h` files resemble a c-like header with `#define` directives.
LogicLab library files are text files that contain *IEC 61131-3* ST code,
the latest format (`.plclib`) is xml based.

The supported transformations are:
* `.h` → `.pll`, `.plclib`
* `.pll` → `.plclib`



### Limitations

#### Input files
Input files must:
* Be encoded in `ANSI` or `UTF-8`\
  _Rationale: Other encodings have only disadvantages and must be avoided:
              much more size (text mostly ASCII),
              slower parsing (endianness),
              less compatibility and comparability_
* Have strictly unix line breaks (`LF`, `\n`)\
  _Rationale: There's really no point for two-chars lines breaks
              unless you're stuck with `notepad.exe` or sending
              the character stream to an ancient typewriter_

#### Syntax
* _IEC 61131-3_ multidimensional arrays like `ARRAY[1..2, 1..2]` not supported
* `.h` `#define`s are required to have a line comment (`// ...`) containing the description
* Descriptions (`.h` `#define`s comment and `.pll` `{DE: ...}`) cannot contain XML special characters nor line breaks\
  _Rationale: Parsing is much more faster without dynamic allocations to
              store a modified input (in this case to escape characters)_



### `.h` files
Sipro header files supported syntax is:
```
// line comment
/*  -------------
    block comment
    ------------- */
#define reg_label register // description
#define val_label value // [type] description
```
Definitions inlined comments cannot contain XML special characters.

Sipro registers will be recognized and exported:
```
#define vqPos vq100 // Position [mm]
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
| ~~`LINT`~~  | ~~*Long INTeger*~~          |  8   | -2⁶³ … 2⁶³⁻¹              |
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



### `.pll` files
Some additional library data will be extracted from the
first comment of the `.pll` file:
```
(*
    descr: Machine logic stuff
    version: 0.5.1
*)
```



### Usage
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



### Build
```
$ git clone https://github.com/matgat/llconv.git
$ cd llconv
$ make stuff/makefile
$ g++ -std=c++2b -funsigned-char -Wall -Wextra -Wpedantic -Wconversion -O3 -DFMT_HEADER_ONLY -Isource/fmt/include -o "llconv" "source/llconv.cpp"
```
Not so sure about these:
```
> msbuild msvc/llconv.vcxproj -t:llconv -p:Configuration=Release
> cl /std:c++latest /utf-8 /J /W4 /O2 /D_CRT_SECURE_NO_WARNINGS /DFMT_HEADER_ONLY /I../source/fmt/include "source/llconv.cpp" /link /out:llconv.exe
```
