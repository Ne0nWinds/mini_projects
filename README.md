# Mini Project Showcase
A collection of small projects that I had created to learn OpenGL, SIMD, and other advanced programming topics

## Particle Physics Engine
A project to learn how to render thousands of particles efficiently in OpenGL

Features include:
- Custom SIMD-optimized vector library ([vector.h](https://github.com/Ne0nWinds/mini_projects/blob/master/particle_physics_engine/vector.h))
- Efficient Memory Allocator ([win32_memory.h](https://github.com/Ne0nWinds/mini_projects/blob/master/particle_physics_engine/win32_memory.c))
- Use of modern OpenGL draw calls ([renderer.c](https://github.com/Ne0nWinds/mini_projects/blob/master/particle_physics_engine/renderer.c#L250))

![particle_physics_engine](https://user-images.githubusercontent.com/36315399/214190011-41ea9ed7-99f3-4498-865f-0857a2239d9f.gif)

## Bomberman
A recreation of an old hackathon project in C and OpenGL

Features include:
- Draw calls using instancing ([main.c](https://github.com/Ne0nWinds/mini_projects/blob/87c5fe29af2f65ad6b9e61c9bab90e47a3e84962/bomberman/main.c#L511))
- Controller input with Xinput ([main.c](https://github.com/Ne0nWinds/mini_projects/blob/87c5fe29af2f65ad6b9e61c9bab90e47a3e84962/bomberman/main.c#L411))
- Smoothed collision detection ([main.c](https://github.com/Ne0nWinds/mini_projects/blob/87c5fe29af2f65ad6b9e61c9bab90e47a3e84962/bomberman/main.c#L468))
- Texture atlasing

![PXL_20221210_020541410_1__AdobeExpress](https://user-images.githubusercontent.com/36315399/214193011-a7483bb7-7245-4d04-8629-74a4878b2f5b.gif)

## Cellular Automata
A project similar to [John Conway's Game of Life](https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life) to learn about OpenGL compute shaders

Features include:
- A compute shader that reads / writes to a texture ([compute.glsl](https://github.com/Ne0nWinds/mini_projects/blob/master/Cellular%20Automata/build/shaders/compute.glsl))
- SPIR-V compilation ([win32_main.c](https://github.com/Ne0nWinds/mini_projects/blob/97541c1116be05b7e1efd3b0a2da420a781bfc8e/Cellular%20Automata/win32_main.c#L106))

![CCAPostProcessSmall2](https://user-images.githubusercontent.com/36315399/214195592-43ad0e2a-c435-49c1-9585-34b64041a699.jpg)
