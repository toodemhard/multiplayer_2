mkdir %1

xcopy dist.bat %1
xcopy clean.bat %1
xcopy copy_assets.bat %1
xcopy build_compile.bat %1

pushd %1
call .\build_compile.bat
call .\copy_assets.bat
popd
