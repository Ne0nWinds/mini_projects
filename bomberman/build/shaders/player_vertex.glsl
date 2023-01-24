#version 330 core

layout (location = 0) in vec2 vertex;
layout (location = 1) in vec2 pos;

uniform mat4 ortho;
uniform mat4 model;

void main() {

	vec4 instance_position = vec4(pos, 0.0f, 0.0f);
	vec4 local_pos = model * vec4(vertex.xy, 0.0f, 1.0f);

	gl_Position = ortho * (local_pos + instance_position);
}
