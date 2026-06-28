mkdir bin
robocopy assets bin\assets /E
set COMPILER_DIR=C:/raylib/w64devkit/bin
set PATH=%PATH%;%COMPILER_DIR%
cmd /c gcc src/unity.c -I vendor -I C:/raylib/raylib/src -L C:/raylib/raylib/src -lraylib -lopengl32 -lgdi32 -lwinmm -lcomdlg32 -lole32 --embed-file assets -o bin\jam_thing.exe && bin\jam_thing.exe
