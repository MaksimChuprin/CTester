@echo off

rem wmic OS get OSArchitecture | findstr 64 > NUL 2>&1
rem if %errorlevel%==1 goto X86

if "%PROCESSOR_ARCHITEW6432%" == "AMD64" goto X64
if "%PROCESSOR_ARCHITECTURE%" == "AMD64" goto X64

:X86
VCP_V1.5.0_Setup_W7_x86_32bits.exe
goto END

:X64
VCP_V1.5.0_Setup_W7_x64_64bits.exe

:END
exit
