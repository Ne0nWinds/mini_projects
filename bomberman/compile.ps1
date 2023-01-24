clang -g -O0 -mavx .\main.c .\GLAD\src\glad.c -Llib\Release\x64\glew32 -lUser32 -lOpengl32 -lGdi32 -lWinmm -lXinput -IGLAD\include -Wno-incompatible-pointer-types -o build\a.exe
