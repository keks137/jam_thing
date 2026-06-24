REM call C:/raylib/emsdk/emsdk_env.bat
mkdir bin
robocopy assets bin\assets /E
set NODE_SKIP_PLATFORM_CHECK=1
C:/raylib/emsdk/upstream/emscripten/emcc --embed-file assets -sASSERTIONS -o bin/index.html src/unity.c -Os -Wall -Wextra -DPLATFORM_WEB -I c:/raylib/raylib/src c:/raylib/raylib/src/libraylib.web.a -I vendor -s USE_GLFW=3 --shell-file src/shell.html

