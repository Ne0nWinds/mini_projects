#version 330 core

out vec4 color;

vec3 RGB(float r, float g, float b) {
	return vec3(r / 255.0f, g / 255.0f, b / 255.0f);
}

void main() {
	color = vec4(RGB(253, 133, 87), 1.0f);
}
