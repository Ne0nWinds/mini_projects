#version 460 core
layout(local_size_x = 8, local_size_y = 4, local_size_z = 1) in;
layout(rgba32f, binding = 0) uniform image2D write_texture;

vec2 random(vec2 p) {
	vec3 a = fract(p.xyx * vec3(123.34, 234.34, 345.65));
	a += dot(a, a + 34.45);
	return fract(vec2(a.x * a.y, a.y * a.z));
}

void main() {
	vec2 pixel_coords = gl_GlobalInvocationID.xy;
	vec4 pixel = vec4(random(pixel_coords).xxx, 1.0f);
	imageStore(write_texture, ivec2(pixel_coords), pixel);
}