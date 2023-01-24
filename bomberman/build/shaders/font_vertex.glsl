#version 330 core

layout (location = 0) in vec4 vertex;

out vec2 tex_coords;

uniform mat4 ortho;

void main() {
	gl_Position = ortho * vec4(vertex.xy, 0.0f, 1.0f);
	tex_coords = vertex.zw;
	tex_coords.y = 1.0f - tex_coords.y;
}
