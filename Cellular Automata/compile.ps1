glslc.exe -fshader-stage=compute .\build\shaders\compute.glsl -o build\shaders\compute.spv
glslc.exe -fshader-stage=compute .\build\shaders\compute_initialize.glsl -o build\shaders\compute_initialize.spv
glslc.exe -fshader-stage=vertex .\build\shaders\vertex.glsl -o build\shaders\vertex.spv
glslc.exe -fshader-stage=fragment .\build\shaders\fragment.glsl -o build\shaders\fragment.spv

clang -g *.c .\glad\src\*.c -Iglad\include -lUser32 -lopengl32 -lgdi32 -o build\a.exe
