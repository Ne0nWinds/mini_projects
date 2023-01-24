#version 330 core
	
in vec2 tex_coords;
out vec4 color;

uniform sampler2D image;

void main()
{
	float distance = texture(image, tex_coords).r;
	float alpha = smoothstep(0.49, 0.5f, distance);
	
	float shadow_distance = texture(image, tex_coords + vec2(-0.05f, -0.05f)).r;
	float outline_alpha =  smoothstep(0.49, 0.5f, shadow_distance);

	float overall_alpha = alpha + (1.0f - alpha) * outline_alpha;
	vec3 overall_color = mix(vec3(0.1f, 0.1f, 0.1f), vec3(1.0f, 1.0f, 1.0f), alpha / overall_alpha);
	
	color = vec4(overall_color, overall_alpha);
	
	// color = vec4(texture(image, tex_coords).rrr, 1.0f);
}
