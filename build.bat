@echo off
echo Running developer command prompt...
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools\Common7\Tools\VsDevCmd.bat"
if %errorlevel% neq 0 exit /b %errorlevel%
echo Building...
cl.exe /Zi /EHsc /Fe:example\example.exe example\example.cpp
if %errorlevel% neq 0 exit /b %errorlevel%
echo Build successful.
