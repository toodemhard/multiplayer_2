pushd b2

set lib_path=C:\Users\toodemhard\projects\games\multiplayer_2\lib
set src_path=..\src

set includes=/I %lib_path%\SDL\include /I %lib_path%\yojimbo\serialize /I ..\lib\yojimbo\include\ /I ..\lib\glm /I ..\lib\box2d\include /I ..\lib\tracy\public /I ..\lib\imgui ^
/I %lib_path%\stb ^
/I %lib_path%\enet\include

REM cl /std:c++20 /EHsc /Zi /MP /MDd /Yc"pch.h" /D TRACY_ENABLE  /c %src_path%\pch.cpp %includes%
cl /std:c++20 /EHsc /Zi /MP /MDd /Yc"pch.h" /c %src_path%\pch.cpp %includes%

popd
