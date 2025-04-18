@echo off

pushd b2

set lib_path=C:\Users\toodemhard\projects\games\multiplayer_2\lib
set src_path=..\src

set includes=/I %lib_path%\SDL\include /I %lib_path%\yojimbo\serialize /I ..\lib\yojimbo\include\ /I ..\lib\glm /I ..\lib\box2d\include /I ..\lib\tracy\public /I ..\lib\imgui ^
/I %lib_path%\EASTL\include ^
/I EASTL\_deps\eabase-src\include\Common ^
/I %lib_path%\stb ^
/I %src_path%\common ^
/I %src_path%


REM cl /std:c++20 /EHsc /Zi /MP %includes% /c ../src/client/*.cpp /c ../src/common/*.cpp 
cl /std:c++20 /EHsc /Zi /MP %includes% /Yu"../pch.h" /Fp"pch.pch" /c ../src/client/*.cpp /c ../src/common/*.cpp 
REM cl /std:c++20 /EHsc /Zi /MP %includes% /c ../lib/imgui/*.cpp

popd
