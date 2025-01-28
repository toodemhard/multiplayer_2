set out=.\b2\build.exe
cl /Zi /std:c++20 /Fe:%out% .\build.cpp

pushd b2

.\build.exe
REM rm build.exe

popd
