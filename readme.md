# [llconv](https://github.com/matgat/llconv.git)

This is a conversion utility between these formats:
* `*.h` (Sipro *#defines* file)
* `*.pll` (LogicLab3 library file)
* `*.plclib` (LogicLab5 library file)

The supported transformations are:
* `*.h` -> `*.pll`
* `*.pll` -> `*.plclib`


## Limitations
Input files must:
* Be encoded in ANSI or UTF-8
* Have strictly unix line breaks (`\n`)
* Descriptions in `{DE: ...}` cannot contain special characters and line breaks


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

In case of mixed input files (`*.h` and `*.pll`), the `*.h` files
will be converted first, and the resulting `*.pll` will be added
to the others in input.


## Build
```
$ git clone https://github.com/matgat/llconv.git
```
