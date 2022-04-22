@echo off
set fname=test
set out_dir=D:\HD\Desktop
set wmerge="%ProgramFiles%\WinMerge\WinMergeU.exe" /e /u

set in_file=%fname%.pll

:CONVERT
..\msvc\x64-Debug\llconv.exe --fussy --verbose "%in_file%" --output "%out_dir%"
if errorlevel 1 goto ERROR

:COMPARE
echo.
setlocal EnableDelayedExpansion
if exist "%out_dir%\%fname%.plclib" (
echo Generated %fname%.plclib
set out_file=%out_dir%\%fname%.plclib
%wmerge% "%in_file%" "!out_file!"
del /S "!out_file!"
) else (
echo PLL_TEST should generate %fname%-1.pll, %fname%-2.pll
set out_file1=%out_dir%\%fname%-1.pll
set out_file2=%out_dir%\%fname%-2.pll
%wmerge% "%in_file%" "!out_file1!" "!out_file2!"
del /S "!out_file1!"
del /S "!out_file2!"
)
exit

:ERROR
pause
