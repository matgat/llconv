@echo off
set fname=test
set out_dir=D:\HD\Desktop

..\msvc\x64-Debug\llconv.exe --fussy --verbose "%fname%.pll" --output "%out_dir%"
if errorlevel 1 goto END

echo.
if exist "%out_dir%\%fname%.plclib" (
echo Generated %fname%.plclib
"%out_dir%\%fname%.plclib"
) else (
echo Compiled with PLL_TEST
"%ProgramFiles%\WinMerge\WinMergeU.exe" /e /u "%fname%.pll" "%out_dir%\%fname%-1.pll" "%out_dir%\%fname%-2.pll"
)

:END
pause
