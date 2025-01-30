@echo off

set out=.\b2\build.exe
cl /Zi /std:c++20 /MP /Fe:%out% .\build.cpp

REM pushd b2
REM
REM .\build.exe
REM
REM popd
