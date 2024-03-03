@REM Build for Visual Studio compiler. Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.

set OUT_DIR=x86_64
set OUT_EXE=halma

set SOURCES=src\main.cpp
set INCLUDES=/Ilibs\eigen-3.4.0 /Ilibs\dirent /Ilibs/gl3w /Isrc\data_structures /Isrc/imgui /Ilibs\imgui-1.88 /Ilibs/glm /Ilibs\SDL2-2.26.1\include /Ilibs\SDL2_image-2.0.5\include

set LIBGL=/libpath:libs\gl OpenGL32.Lib
set LIBSDL=/libpath:libs\SDL2-2.26.1\lib\x64 SDL2.lib SDL2main.lib
set LIBSDLIMAGE=/libpath:libs\SDL2_image-2.0.5\lib\x64 SDL2_image.lib
set LIBS=%LIBGL% %LIBSDL% %LIBSDLIMAGE%

mkdir %OUT_DIR%
cl /nologo /Zi /W3 /wd4996 /MD /Zo /std:c++20 /EHsc %INCLUDES% %SOURCES% /Fe%OUT_DIR%\%OUT_EXE%.exe /Fo%OUT_DIR%/ /link %LIBS% /subsystem:console

:: /W2 /fsanitize=address /MD