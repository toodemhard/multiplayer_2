@echo off

REM set out=.\b2\build.exe
REM cl /Zi /std:c++20 /MP /Fe:%out% .\build.cpp

pushd b2

.\build.exe
REM rm build.exe

popd
