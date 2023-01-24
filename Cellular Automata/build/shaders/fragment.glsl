#version 460 core
	
layout (location = 0) out vec4 color;

layout (location = 0) in vec2 UV;

layout (binding = 0) uniform sampler2D screen_texture;

vec4 RGB(float r, float g, float b) {
	return vec4(
		r / 255.0f,
		g / 255.0f,
		b / 255.0f,
		1.0f
	);
}

void main() {
	color = texture(screen_texture, UV);
}
