#pragma once
#include "general.h"

struct v2 {
	f32 x, y;
};

typedef struct v2 v2;

struct m3 {
	f32 m[3][3];
};
typedef struct m3 m3;

#define FLT_MAX __FLT_MAX__
#define FLT_EPSILON 1E-5

inline v2 v2_add(v2 a, v2 b);
inline v2 v2_sub(v2 a, v2 b);
inline v2 v2_mul(v2 a, v2 b);
inline v2 v2_div(v2 a, v2 b);
inline v2 v2_negate(v2 a);
inline v2 v2_scale(v2 a, f32 s);
inline f32 v2_dot(v2 a, v2 b);
inline v2 v2_normalize(v2 a);
inline f32 v2_length(v2 a);
inline v2 v2_rotate(v2 a, f32 radians);
inline v2 v2_normal(v2 a);

inline void m3_camera(f32 window_width, f32 window_height, f32 zoom, m3 *out);
inline void m3_scale(f32 scale, m3 *out);
inline void m3_translate(v2 translation, m3 *out);
inline void m3_rotate(f32 radians, m3 *out);
inline m3 m3_model(v2 translation, f32 scale, f32 radians);
#define m3_identity() ((m3){\
	1.0f, 0.0f, 0.0f,\
	0.0f, 1.0f, 0.0f,\
	0.0f, 0.0f, 1.0f,\
})

inline f32 f32_abs(f32 value);
inline f32 f32_safe_divide(f32 value, f32 divisor);
inline f32 f32_negate(f32 value);
inline f32 f32_min(f32 a, f32 b);
inline f32 f32_max(f32 a, f32 b);

#include <immintrin.h>

inline v2 v2_add(v2 a, v2 b) {
	__m128 m_a, m_b;
	m_a = _mm_loadl_pi(m_a, &a);
	m_b = _mm_loadl_pi(m_b, &b);
	
	__m128 m_result = _mm_add_ps(m_a, m_b);
	
	return *((v2 *)&m_result);
}

v2 v2_sub(v2 a, v2 b) {
	__m128 m_a, m_b;
	m_a = _mm_loadl_pi(m_a, &a);
	m_b = _mm_loadl_pi(m_b, &b);
	
	__m128 m_result = _mm_sub_ps(m_a, m_b);
	
	return *((v2 *)&m_result);
}

v2 v2_mul(v2 a, v2 b) {
	__m128 m_a, m_b;
	m_a = _mm_loadl_pi(m_a, &a);
	m_b = _mm_loadl_pi(m_b, &b);
	
	__m128 m_result = _mm_mul_ps(m_a, m_b);
	
	return *((v2 *)&m_result);
}

v2 v2_div(v2 a, v2 b) {
	__m128 m_a, m_b;
	m_a = _mm_loadl_pi(m_a, &a);
	m_b = _mm_loadl_pi(m_b, &b);
	
	__m128 m_result = _mm_div_ps(m_a, m_b);
	
	return *((v2 *)&m_result);
}

v2 v2_negate(v2 a) {
	u64 mask = ((u64)1 << 31) | ((u64)1 << 63);
	
	*(u64 *)&a ^= mask;

	return a;
}

v2 v2_scale(v2 a, f32 s) {
	__m128 m_a, m_b;
	m_a = _mm_loadl_pi(m_a, &a);
	m_b = _mm_load_ps1(&s);
	
	__m128 m_result = _mm_mul_ps(m_a, m_b);
	
	return *(v2 *)&m_result;
}

f32 v2_dot(v2 a, v2 b) {
	__m128 m_a, m_b;
	m_a = _mm_loadl_pi(m_a, &a);
	m_b = _mm_loadl_pi(m_b, &b);
	
	__m128 m_result = _mm_mul_ps(m_a, m_b);
	__m128 throwaway;
	m_result = _mm_hadd_ps(m_result, throwaway);
	
	return *(f32 *)&m_result;
}

f32 v2_length(v2 a) {
	__m128 m_a = _mm_loadl_pi(m_a, &a);

	__m128 length_squared = _mm_mul_ps(m_a, m_a);
	__m128 throwaway;
	length_squared = _mm_hadd_ps(length_squared, throwaway);
	
	__m128 length = _mm_sqrt_ss(length_squared);
	
	return *(f32 *)&length;
}

v2 v2_normalize(v2 a) {
	__m128 m_a;
	m_a = _mm_loadl_pi(m_a, &a);
	
	__m128 length_squared = _mm_mul_ps(m_a, m_a);
	__m128 throwaway;
	length_squared = _mm_hadd_ps(length_squared, throwaway);
	
	__m128 length = _mm_sqrt_ss(length_squared);
	length = _mm_shuffle_ps(length, length, 0);
	// length = _mm_set1_ps(*(f32 *)&length);
	
	__m128 division_result = _mm_div_ps(m_a, length);
	
	__m128i mask = _mm_cmpgt_ps(length, _mm_set1_ps(FLT_EPSILON));
	
	__m128 result = _mm_and_ps(division_result, mask);
	
	return *(v2 *)&result;
}

#define cos(radians) __builtin_cosf(radians)
#define sin(radians) __builtin_sinf(radians)

v2 v2_rotate(v2 a, f32 radians) {
	
	// // TODO: don't use std library cos and sin
	v2 sin_cos_v2 = {
		 __builtin_cosf(radians),
		 __builtin_sinf(radians),
	};
	
	// __m128 sin_cos = _mm_set_ps(cos, sin, sin, cos);
	__m128 sin_cos;
	sin_cos = _mm_loadl_pi(sin_cos, &sin_cos_v2);
	sin_cos = _mm_shuffle_ps(sin_cos, sin_cos, (1 << 2) | (1 << 4));
	
	// __m128 xy = _mm_set_ps(a.y, a.x, -a.y, a.x);
	__m128 xy;
	xy = _mm_loadl_pi(xy, &a);
	xy = _mm_movedup_pd(xy);
	((f32 *)&xy)[1] *= -1.0f;
	
	__m128 mul_result = _mm_mul_ps(sin_cos, xy);
	__m128 throwaway;
	__m128 result = _mm_hadd_ps(mul_result, throwaway);
	
	return *(v2 *)&result;
	
	// v2 result;
	// result.x = a.x * cos(radians) - a.y * sin(radians);
	// result.y = a.x * sin(radians) + a.y * cos(radians);
	// return result;
}

inline v2 v2_normal(v2 a) {
	a.x = f32_negate(a.x);
	
	__m128 m_a = _mm_loadl_pi(m_a, &a);
	__m128 throwaway;
	m_a = _mm_shuffle_ps(m_a, throwaway, 1);
	
	return *(v2 *)&m_a;
}

void m3_camera(f32 window_width, f32 window_height, f32 zoom, m3 *out) {
	out->m[0][0] *= zoom * 1.0f / window_width;
	out->m[1][1] *= zoom * 1.0f / window_height;
}

m3 m3_model(v2 translation, f32 scale, f32 radians) {
	m3 out = m3_identity();
	
	f32 cos = __builtin_cosf(radians);
	f32 sin = __builtin_sinf(radians);
	
	out.m[0][0] = cos * scale;
	out.m[1][1] = cos * scale;

	out.m[0][1] = sin * scale;
	out.m[1][0] = -sin * scale;
	
	// translation
	__m128 xy;
	xy = _mm_loadl_pi(xy, &translation);
	_mm_storel_pi(&out.m[2][0], xy);
	
	return out;
}

void m3_scale(f32 scale, m3 *out) {
	out->m[0][0] *= scale;
	out->m[1][1] *= scale;
}

void m3_translate(v2 translation, m3 *out) {
	
	__m128 xy;
	xy = _mm_loadl_pi(xy, &translation);
	
	_mm_storel_pi(&out->m[2][0], xy);
	
	// out->m[2][0] = translation.x;
	// out->m[2][0] = translation.y;
}

void m3_rotate(f32 radians, m3 *out) {
	
	f32 cos = __builtin_cosf(radians);
	f32 sin = __builtin_sinf(radians);
	
	out->m[0][0] = cos;
	out->m[1][1] = cos;

	out->m[0][1] = sin;
	out->m[1][0] = -sin;
}

f32 f32_abs(f32 value) {
	*((u32 *)&value) &= ~(1 << 31);
	return value;
}

f32 f32_safe_divide(f32 value, f32 divisor) {
	value /= divisor;
	bool divisor_not_zero = divisor > FLT_EPSILON;
	*(u32 *)&value *= divisor_not_zero;
	return value;
}

f32 f32_negate(f32 value) {
	*(u32 *)&value ^= (1 << 31);
	return value;
}

f32 f32_min(f32 a, f32 b) {
	__m128 m_a = _mm_load_ss(&a);
	__m128 m_b = _mm_load_ss(&b);
	
	__m128 result = _mm_min_ss(m_a, m_b);
	
	return _mm_cvtss_f32(result);
}

f32 f32_max(f32 a, f32 b) {
	__m128 m_a = _mm_load_ss(&a);
	__m128 m_b = _mm_load_ss(&b);
	
	__m128 result = _mm_max_ss(m_a, m_b);
	
	return _mm_cvtss_f32(result);
}
