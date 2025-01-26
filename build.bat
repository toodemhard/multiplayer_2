@echo off

pushd build\debug
cmake --build . --target client
popd
