#version 330 core

layout (location = 0) in vec2 local_pos;
layout (location = 1) in vec3 world_pos;
layout (location = 2) in float width;

uniform mat3 camera;

out vec2 m_pos;

mat2 rotation_matrix = mat2(
	cos(world_pos.z), -sin(world_pos.z),
	sin(world_pos.z), cos(world_pos.z)
);

void main() {
	m_pos = rotation_matrix * local_pos.xy;
	vec2 model_pos = local_pos.xy * width;
	
	vec3 new_pos = camera * vec3(model_pos + world_pos.xy, 1.0f);
	gl_Position = vec4(new_pos, 1.0f);
}
