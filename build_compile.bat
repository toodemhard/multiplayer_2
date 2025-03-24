@echo off

set out=.\build.exe
cl /Zi /std:c++20 /MP /Fe:%out% ..\build.cpp


REM clang -g -std=c++20 -fuse-ld=lld .\build_2.cpp -I . -I .\src\
