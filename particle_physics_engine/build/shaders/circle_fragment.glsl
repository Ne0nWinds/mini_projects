#version 330 core

out vec4 color;
in vec2 m_pos;

vec3 RGB(float r, float g, float b) {
	return vec3(r / 255.0f, g / 255.0f, b / 255.0f);
}

void main() {
	float distance = length(m_pos);
	float blur = 1.0f - smoothstep(0.49, 0.5, distance);
	
	float pct = smoothstep(0.02f, 0.01f, abs(m_pos.y));
	pct *= step(0.0f, m_pos.x);
	
	vec3 circle_color = RGB(253, 133, 87);
	vec3 line_color = vec3(0.0f, 0.0f, 0.0f);
	
	vec3 output_color = (1.0 - pct) * circle_color + line_color;
	color = vec4(output_color, blur);
	
	// color *= vec4(smoothstep(0.44, 0.45, distance));
}
