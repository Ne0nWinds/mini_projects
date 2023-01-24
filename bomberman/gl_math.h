#pragma once
#include "general.h"
#define _USE_MATH_DEFINES
#include <float.h>
#include <math.h>
#include "immintrin.h"

struct __attribute__((aligned(16))) m4 {
	f32 m[4][4];
};

struct __attribute__((aligned(16))) v4 {
	f32 x, y, z, w;
};
struct __attribute__((aligned(16))) v3 {
	f32 x, y, z;
};
struct v2 {
	f32 x, y;
};
struct v2i {
	s32 x, y;
};

typedef struct m4 m4;
typedef struct v2i v2i;
typedef struct v2 v2;
typedef struct v3 v3;
typedef struct v4 v4;

v2 v2_add(v2 a, v2 b) {
	return (v2){
		a.x + b.x,
		a.y + b.y
	};
}
v2 v2_sub(v2 a, v2 b) {
	return (v2){
		a.x - b.x,
		a.y - b.y
	};
}
v2 v2_scale(v2 a, f32 b) {
	return (v2){
		a.x * b,
		a.y * b
	};
}

f32 v2_dot(v2 a, v2 b) {
	return a.x * b.x + a.y * b.y;
}

f32 v2_length(v2 a) {
	f32 length_squared = v2_dot(a, a);
	return sqrtf(length_squared);
}

v2 v2_normalize(v2 a) {
	f32 length = v2_length(a);

	if (length <= FLT_EPSILON)
		return (v2){ 0.0f, 0.0f };

	return (v2){
		a.x / length,
		a.y / length
	};
}

v3 v3_sub(v3 a, v3 b) {
	return (v3){
		a.x - b.x,
		a.y - b.y,
		a.z - b.z
	};
}
v3 v3_add(v3 a, v3 b) {
	return (v3){
		a.x + b.x,
		a.y + b.y,
		a.z + b.z
	};
}
v3 v3_scale(v3 a, f32 b) {
	return (v3){
		a.x * b,
		a.y * b,
		a.z * b,
	};
}

f32 v3_dot(v3 a, v3 b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

f32 v3_length(v3 a) {
	f32 length_squared = v3_dot(a, a);
	return sqrtf(length_squared);
}

v3 v3_normalize(v3 a) {
	f32 length = v3_length(a);

	if (length == 0.0f) return a;

	return (v3){
		a.x / length,
		a.y / length,
		a.z / length
	};
}

v3 v3_cross(v3 a, v3 b) {
	return (v3){
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x
	};
}

#define v4_from_v3(v) (v4){v.x, v.y, v.z, 0.0f}

m4 m4_identity() {
	return (m4){
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};
}

m4 m4_rotate_x(f32 radians) {
	return (m4){
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, cosf(radians), -sinf(radians), 0.0f,
		0.0f, sinf(radians), cosf(radians), 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};
}

m4 m4_rotate_y(f32 radians) {
	return (m4){
		cosf(radians), 0.0f, sinf(radians), 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		-sinf(radians), 0.0f, cosf(radians), 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};
}

m4 m4_rotate_z(f32 radians) {
	return (m4){
		cosf(radians), -sinf(radians), 0.0f, 0.0f,
		sinf(radians), cosf(radians), 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};
}

m4 m4_scale(f32 scale) {
	return (m4){
		scale, 0.0f, 0.0f, 0.0f,
		0.0f, scale, 0.0f, 0.0f,
		0.0f, 0.0f, scale, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};
}

__vectorcall void m4_translate(v4 translation, m4 *matrix) {
	__m128 translation_m128 = _mm_load_ps((f32 *)&translation);
	f32 *matrix_translation = ((f32 *)matrix) + 12;
	__m128 matrix_translation_m128 = _mm_load_ps(matrix_translation);
	__m128 result = _mm_add_ps(translation_m128, matrix_translation_m128);
	_mm_store_ps(matrix_translation, result);
}

m4 m4_mul(m4 a, m4 b) {
	m4 result = {0};

	for (u32 i = 0; i < 4; ++i) {
		for (u32 j = 0; j < 4; ++j) {
			result.m[i][j] += a.m[i][0] * b.m[0][j];
			result.m[i][j] += a.m[i][1] * b.m[1][j];
			result.m[i][j] += a.m[i][2] * b.m[2][j];
			result.m[i][j] += a.m[i][3] * b.m[3][j];
		}
	}

	return result;
}

m4 m4_perspective(f32 fov, f32 aspect_ratio, f32 near_clip, f32 far_clip) {
	const f32 tanHalfFovy = tan(fov / 2.0f);

	m4 result = {0};
	result.m[0][0] = 1.0f / (aspect_ratio * tanHalfFovy);
	result.m[1][1] = 1.0f / tanHalfFovy;
	result.m[2][2] = - (far_clip + near_clip) / (far_clip - near_clip);
	result.m[2][3] = - 1.0f;
	result.m[3][2] = - (2.0f * far_clip * near_clip) / (far_clip - near_clip);
	return result;
}

m4 m4_look_at(v3 pos, v3 target) {
	v3 world_up = { 0.0f, 1.0f, 0.0f };

	// v3 f = v3_normalize(v3_sub(target, pos));
	v3 f = target;
	v3 s = v3_normalize(v3_cross(f, world_up));
	v3 u = v3_cross(s, f);

	m4 result = m4_identity();
	result.m[0][0] = s.x;
	result.m[1][0] = s.y;
	result.m[2][0] = s.z;
	result.m[0][1] = u.x;
	result.m[1][1] = u.y;
	result.m[2][1] = u.z;
	result.m[0][2] =-f.x;
	result.m[1][2] =-f.y;
	result.m[2][2] =-f.z;
	result.m[3][0] =-v3_dot(s, pos);
	result.m[3][1] =-v3_dot(u, pos);
	result.m[3][2] = v3_dot(f, pos);
	return result;
}

m4 m4_ortho(f32 window_width, f32 window_height, f32 zoom, v2 camera_pos) {
	m4 result = m4_identity();

	result.m[0][0] = 2.0f / window_width * zoom;
	result.m[1][1] = 2.0f / window_height * zoom;

	camera_pos = v2_scale(camera_pos, 2.0f * zoom);

	result.m[3][0] = -camera_pos.x / window_width;
	result.m[3][1] = -camera_pos.y / window_height;

	return result;
}
