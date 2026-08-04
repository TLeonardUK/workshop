@echo off

SET ScriptPath=%~dp0
SET RootPath=%ScriptPath%..\..\..\
SET BuildPath=%ScriptPath%..\..\..\intermediate\vs2026\
SET CMakeExePath="C:\Program Files\CMake\bin\cmake.exe"

echo Generating %RootPath%
echo %CMakeExePath% -S %RootPath% -B %BuildPath%

%CMakeExePath% -S %RootPath% -B %BuildPath% -G "Visual Studio 18 2026"