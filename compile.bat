set lib_path=C:\Users\toodemhard\projects\games\multiplayer_2\lib
set src_path=..\src

set includes=/I %lib_path%\SDL\include /I %lib_path%\yojimbo\serialize /I ..\lib\yojimbo\include\ /I ..\lib\glm /I ..\lib\box2d\include /I ..\lib\tracy\public /I ..\lib\imgui ^
REM /I %lib_path%\EASTL\include ^
/I %lib_path%\stb ^
/I %src_path%\common 


cl /std:c++20 %includes% /EHsc /c ../src/client/*.cpp
