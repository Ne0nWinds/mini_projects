#version 330 core

layout (location = 0) in vec2 local_pos;
layout (location = 1) in vec3 world_pos;
layout (location = 2) in vec2 scale;

uniform mat3 camera;

void main() {
	vec2 model_pos;
	
	model_pos.x = local_pos.x * cos(world_pos.z) - local_pos.y * sin(world_pos.z);
	model_pos.y = local_pos.x * sin(world_pos.z) + local_pos.y * cos(world_pos.z);
	
	model_pos *= scale;
	
	vec3 new_pos = camera * vec3(model_pos + world_pos.xy, 1.0f);
	gl_Position = vec4(new_pos, 1.0f);
}
