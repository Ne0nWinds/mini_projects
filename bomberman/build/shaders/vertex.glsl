#version 330 core

layout (location = 0) in vec4 vertex;
layout (location = 1) in uint texture_index;

uniform mat4 ortho;
uniform mat4 model;

out vec2 tex_coords;

void main() {

	vec2 instance_position = vec2(gl_InstanceID % 11, gl_InstanceID / 11);

	gl_Position = ortho * model * vec4(instance_position + vertex.xy, 0.0f, 1.0f);

	vec2 texture_coordinates = vertex.zw;
	texture_coordinates.x /= 3.0f;
	texture_coordinates.x += float(texture_index) / 3.0f;
	tex_coords = texture_coordinates;
}
