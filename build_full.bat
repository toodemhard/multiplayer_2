@echo off

mkdir b2
xcopy data b2 /E /I /Y

set out=.\b2\build.exe
cl /Zi /std:c++20 /MP /Fe:%out% .\build.cpp

pushd b2

.\build.exe

popd

