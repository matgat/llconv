@echo off
rem -----------------------------------------------------------
rem This will compare the generated files of two version
rem of the program, one in path and the other in development
rem -----------------------------------------------------------
set llconv_pre=llconv.exe
set llconv_dev=..\msvc\x64-Debug\llconv.exe
set llconv_args=-fussy -clear
set out_dir=D:\HD\Desktop
set h_sources=%UserProfile%\Macotec\Machines\m32-Strato\sde\PROG\*.h
set pll_sources=%UserProfile%\Macotec\Machines\m32-Strato\sde\PLC\*.pll
set wmerge="%ProgramFiles%\WinMerge\WinMergeU.exe" /e /u

set out_dir_pre=%out_dir%\pre
set out_dir_dev=%out_dir%\dev


:PRE
echo.
echo ---- Previous version ----
if exist "%out_dir_pre%" (
  rem @del /F /S /Q "%out_dir_pre%\*.pll"
  rem @del /F /S /Q "%out_dir_pre%\*.plclib"
) else (
  mkdir "%out_dir_pre%"
)
prctime.exe %llconv_pre% %llconv_args% "%h_sources%" "%pll_sources%" -output "%out_dir_pre%"
if errorlevel 1 goto ERROR


:DEV
echo.
echo ---- Development version ----
if exist "%out_dir_dev%" (
  rem @del /F /S /Q "%out_dir_dev%\*.pll"
  rem @del /F /S /Q "%out_dir_dev%\*.plclib"
) else (
  mkdir "%out_dir_dev%"
)
prctime.exe %llconv_dev% %llconv_args% "%h_sources%" "%pll_sources%" -output "%out_dir_dev%"
if errorlevel 1 goto ERROR


:COMPARE
%wmerge% "%out_dir_pre%" "%out_dir_dev%"
rem @rd /S /Q "%out_dir_pre%"
rem @rd /S /Q "%out_dir_dev%"
exit


:ERROR
pause
