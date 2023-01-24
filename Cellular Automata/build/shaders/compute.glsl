#version 460 core
layout(local_size_x = 8, local_size_y = 4, local_size_z = 1) in;
layout(rgba32f, binding = 0) uniform image2D read_texture;
layout(rgba32f, binding = 1) uniform image2D write_texture;
layout(rgba32f, binding = 2) uniform image2D render_texture;

layout (binding = 0) uniform cca_parameters {
	int range;
	uint n_states;
	uint threshold;
} parameters;

vec3 rgb2hsv(vec3 c) {
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
	
	vec2 pixel_coords = gl_GlobalInvocationID.xy;
	vec2 resolution = imageSize(write_texture);
	vec2 uv = pixel_coords / resolution;
	uv -= 0.5f;
	uv.x *= resolution.x / resolution.y;
	
	uint state = uint(imageLoad(read_texture, ivec2(pixel_coords)).x * float(parameters.n_states));
	if (state > parameters.n_states) state = 0;
	uint next = (state + 1) % parameters.n_states;
	uint count = 0;
	
	for (int x = -parameters.range; x <= parameters.range; ++x) {
		for (int y = -parameters.range; y <= parameters.range; ++y) {
			if (x == 0 && y == 0) continue;
			
			if (x == 0 || y == 0) {
				ivec2 coords = ivec2(pixel_coords) + ivec2(x, y);
				uint s = uint(imageLoad(read_texture, coords).x * float(parameters.n_states));
				count += uint(s == next);
			}
		}
	}
	
	if (count >= parameters.threshold) {
		state = (state + 1) % parameters.n_states;
	}
	
	float s = state / float(parameters.n_states);
	float c = count / float(parameters.threshold);
	
	imageStore(write_texture, ivec2(pixel_coords), vec4(s));

	vec4 pixel = vec4(0);
	// pixel = vec4(s);

	// pixel = vec4(c);

	
	vec4 prev_pixel = imageLoad(render_texture, ivec2(pixel_coords));
	pixel.x = prev_pixel.z * 0.8f + c * 0.2f;
	pixel.z = prev_pixel.x * 0.7f + s * 0.3f;
	
	imageStore(render_texture, ivec2(pixel_coords), pixel);
	
}