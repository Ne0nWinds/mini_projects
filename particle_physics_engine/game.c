#include "general.h"
#include "game.h"
#include "vector.h"
#include "renderer.h"
#include "memory.h"

#if DEBUG
#include <stdio.h>
#endif

#define _AMD64_
#include <debugapi.h>
#include "benchmark.h"

#define MAX_PARTICLES 1024 * 1024

struct circle {
	v2 velocity;
	f32 angular_velocity;
	f32 inv_mass;
	f32 width;
};

struct box {
	v2 velocity;
	f32 angular_velocity;
	f32 inv_mass;
	f32 width;
	f32 height;
};

struct shape {
	v2 velocity;
	f32 angular_velocity;
	f32 inv_mass;
	v2 size;
};

v2 triangle_vertices[] = {
	{ 0.0f, 0.5f },
	{ 0.5f, -0.5f },
	{ -0.5f, -0.5f }
};

typedef struct circle circle;
typedef struct box box;
typedef struct shape shape;

struct renderer_data {
	v2 pos;
	f32 rotation;
};
typedef struct renderer_data renderer_data;

renderer_data circles_rdata[MAX_PARTICLES] = {0};
circle circles[MAX_PARTICLES] = {0};

box boxes[MAX_PARTICLES] = {0};
shape triangles[MAX_PARTICLES] = {0};

renderer_data shape_rdata[MAX_PARTICLES * 2] = {0};

static u32 circle_count;
static u32 box_count;
static u32 triangle_count;

v2 v2_drag_force(v2 velocity, f32 k) {
	
	v2 drag_direction = v2_scale(v2_normalize(velocity), -1.0f);
	
	f32 drag_magnitude = k * v2_dot(velocity, velocity);
	
	return v2_scale(drag_direction, drag_magnitude);
}

v2 v2_friction_force(v2 velocity, f32 k) {
	return v2_scale(v2_normalize(velocity), -k);
}

s32 random_seed = 1.0;

f32 random() {
	f32 n = __builtin_sinf(random_seed * 6789.12f) * 546.901f;
	random_seed += 1.125f;
	n = n - (s64)n;
	return f32_abs(n);
}

void GameInit() {
	circles_rdata[0].pos.x = 0.1f;
	circles_rdata[0].rotation = 0.2f;
	circles[0].angular_velocity = 0.2f;
	circles[0].width = 0.1f;
	circles[0].inv_mass = 1.0 / 1.0f;
	circles[1].inv_mass = 0.0f;
	circles_rdata[1].pos.x = -0.2f;
	circles_rdata[1].rotation = 1.0f;
	circles[1].width = 0.3f;
	
	f32 widths[] = { 0.1f, 0.3f };
	AddNewCircles(widths, 2);
	circle_count = 2;
	
	shape_rdata[0].pos.x = 0.35f;
	shape_rdata[0].pos.y = 0.25f;
	triangles[0].inv_mass = 1.0f;
	shape_rdata[0].rotation = 0.2f;

	shape_rdata[1].pos.x = -0.25f;
	shape_rdata[1].pos.y = -0.25f;
	triangles[1].angular_velocity = 0.1f;
	triangles[1].inv_mass = 1.0f / 4.0f;
	
	v2 tri_sizes[] = {
		(v2){ 0.1f, 0.1f },
		(v2){ 0.25f, 0.25f },
	};
	
	for (u32 i = 0; i < len(tri_sizes); ++i) {
		triangles[i].size = tri_sizes[i];
	}
}

struct separation {
	f32 separation;
	v2 axis;
	v2 point;
};
typedef struct separation separation;

separation FindMinSeparation(v2 *vertices_a, u32 count_a, v2 *vertices_b, u32 count_b) {
	
	separation result = {
		.separation = -FLT_MAX
	};
	
	for (u32 i = 0; i < count_a; ++i) {
		v2 va = vertices_a[i];
		v2 axis = vertices_a[(i + 1) % 3];
		v2 normal = v2_normal(v2_sub(axis, va));
		
		f32 min_sep = FLT_MAX;
		v2 min_vertex;
		
		for (u32 j = 0; j < count_b; ++j) {
			v2 vb = vertices_b[j];
			f32 projection = v2_dot(v2_sub(va, vb), normal);
			if (projection < min_sep) {
				min_sep = projection;
				min_vertex = vb;
			}
		}
		
		if (min_sep > result.separation) {
			result.separation = min_sep;
			result.axis = axis;
			result.point = min_vertex;
		}
	}
	
	return result;
}

bool VerticesColliding(v2 *vertices_a, u32 count_a, v2 *vertices_b, u32 count_b, separation *c) {
	
	separation c_a = FindMinSeparation(vertices_a, count_a, vertices_b, count_b);
	
	if (c_a.separation >= 0.0f)
		return false;
	
	separation c_b = FindMinSeparation(vertices_b, count_b, vertices_a, count_a);
	
	if (c_b.separation >= 0.0f)
		return false;
	
	if (c_a.separation > c_b.separation) {
		c_a.separation = f32_negate(c_a.separation);
		c_a.point = v2_normal(c_a.point);
		*c = c_a;
	} else {
		c_b.separation = f32_negate(c_b.separation);
		c_b.point = v2_negate(v2_normal(c_a.point));
		*c = c_b;
	}

	return true;
}

#define ONE_PIXEL (1.0 / 1024.0)

static bool created_particle;

void GameMain(f64 delta, game_input input) {
	
	if (input.buttons & LMOUSE && !created_particle && circle_count < MAX_PARTICLES) {
		created_particle = true;
		
		f32 random_number = random();
		random_number /= 8.0f;
		random_number += 0.04f;
		
		// if (random() < 0.5f) {
		
			circles[circle_count] = (circle){
				.inv_mass = 1.0f / random_number,
				.width = random_number
			};
			circles_rdata[circle_count].pos = input.mouse_pos;
			AddNewCircles(&circles[circle_count].width, 1);
			circle_count += 1;

		// } else {
			
		// 	shape_rdata[triangle_count].pos = input.mouse_pos;
		// 	triangles[triangle_count].inv_mass = 1.0f / random_number;
			
		// 	v2 tri_size = {
		// 		.x = random_number,
		// 		.y = random_number
		// 	};
		// 	triangles[triangle_count].size = tri_size;
	
		// 	AddNewShape(SHAPE_TRIANGLE, 1, &tri_size);
			
		// 	triangle_count += 1;
		// }
	}
	
	if (!(input.buttons & LMOUSE)) {
		created_particle = false;
	}
	
	delta /= 128.0f;
	
	v2 movement = input.movement;

	movement = v2_scale(movement, ONE_PIXEL * 9.8f / 2.0f);
	
	for (u32 i = 0; i < circle_count; ++i) {
		
		// kinematics
		v2 velocity = circles[i].velocity;
		v2 position = circles_rdata[i].pos;
		
		v2 drag = v2_friction_force(velocity, 1.0 / 1024.0);
		v2 acceleration = v2_add(movement, drag);
		acceleration = v2_scale(acceleration, circles[i].inv_mass);
		
		velocity = v2_add(velocity, v2_scale(acceleration, delta));
		position = v2_add(position, v2_scale(velocity, delta));
		
		circles[i].velocity = velocity;
		circles_rdata[i].pos = position;
		
		f32 angular_velocity = circles[i].angular_velocity * delta;
		circles_rdata[i].rotation += angular_velocity;
		
		f32 width = circles[i].width / 2.0f;

		// collision detection
		if (circles_rdata[i].pos.y < -renderer_coordinates.y + width) {
			circles[i].velocity.y *= -0.5f;
			circles_rdata[i].pos.y = -renderer_coordinates.y + width;
		}

		if (circles_rdata[i].pos.y > renderer_coordinates.y - width) {
			circles[i].velocity.y *= -0.5f;
			circles_rdata[i].pos.y = renderer_coordinates.y - width;
		}

		if (circles_rdata[i].pos.x > renderer_coordinates.x - width) {
			circles[i].velocity.x *= -0.5f;
			circles_rdata[i].pos.x = renderer_coordinates.x - width;
		}

		if (circles_rdata[i].pos.x < -renderer_coordinates.x + width) {
			circles[i].velocity.x *= -0.5f;
			circles_rdata[i].pos.x = -renderer_coordinates.x + width;
		}

		for (u32 j = i + 1; j < circle_count; ++j) {
			
			f32 a_radius = circles[i].width / 2.0;
			f32 b_radius = circles[j].width / 2.0;
			v2 a_pos = circles_rdata[i].pos;
			v2 b_pos = circles_rdata[j].pos;
			v2 ab = v2_sub(a_pos, b_pos);
			bool is_colliding = v2_length(ab) <= (a_radius + b_radius);
			
			// remove this branch?
			if (!is_colliding) continue;
			
			v2 ab_normal = v2_normalize(ab);
			v2 contact_start = v2_sub(a_pos, v2_scale(ab_normal, a_radius));
			v2 contact_end = v2_add(b_pos, v2_scale(ab_normal, b_radius));
			f32 depth = v2_length(v2_sub(contact_end, contact_start));
			
			// remove this branch?
			if (v2_length(ab_normal) <= FLT_EPSILON) {
				depth = a_radius + b_radius;
				ab_normal = (v2){ 1.0f, 0.0f };
			}
	
			f32 total_mass = circles[i].inv_mass + circles[j].inv_mass;
			f32 da = f32_safe_divide(depth, total_mass) * circles[i].inv_mass;
			f32 db = f32_safe_divide(depth, total_mass) * circles[j].inv_mass;
			
			circles_rdata[i].pos = v2_add(a_pos, v2_scale(ab_normal, da));
			circles_rdata[j].pos = v2_sub(b_pos, v2_scale(ab_normal, db));
			
			f32 e = 0.125f;
			v2 v_rel = v2_sub(circles[i].velocity, circles[j].velocity);
			f32 dot_normal = v2_dot(v_rel, ab_normal);
			f32 impulse_magnitude = -(1 + e) * dot_normal / total_mass;
			v2 impulse = v2_scale(ab_normal, impulse_magnitude);
			
			circles[i].velocity = v2_add(circles[i].velocity, v2_scale(impulse, circles[i].inv_mass));
			circles[j].velocity = v2_sub(circles[j].velocity, v2_scale(impulse, circles[j].inv_mass));
		}
	}
	
	for (u32 i = 0; i < triangle_count; ++i) {
		v2 velocity = triangles[i].velocity;
		v2 position = shape_rdata[i].pos;
		
		v2 drag = v2_friction_force(velocity, 1.0 / 1024.0);
		v2 acceleration = v2_add(movement, drag);
		acceleration = v2_scale(acceleration, triangles[i].inv_mass);
		
		velocity = v2_add(velocity, v2_scale(acceleration, delta));
		position = v2_add(position, v2_scale(velocity, delta));
		
		triangles[i].velocity = velocity;
		shape_rdata[i].pos = position;

		f32 angular_velocity = triangles[i].angular_velocity * delta;
		shape_rdata[i].rotation += angular_velocity;
		
		if (shape_rdata[i].pos.y < -renderer_coordinates.y) {
			// triangles[i].velocity.y *= -0.5f;
			shape_rdata[i].pos.y = -renderer_coordinates.y;
		}

		if (shape_rdata[i].pos.y > renderer_coordinates.y) {
			// triangles[i].velocity.y *= -0.5f;
			shape_rdata[i].pos.y = renderer_coordinates.y;
		}

		if (shape_rdata[i].pos.x > renderer_coordinates.x) {
			// triangles[i].velocity.x *= -0.5f;
			shape_rdata[i].pos.x = renderer_coordinates.x;
		}

		if (shape_rdata[i].pos.x < -renderer_coordinates.x) {
			// triangles[i].velocity.x *= -0.5f;
			shape_rdata[i].pos.x = -renderer_coordinates.x;
		}
	}
	
	v2 *vertices_a = BumpAlloc(sizeof(v2) * 3);
	v2 *vertices_b = BumpAlloc(sizeof(v2) * 3);

	for (u32 i = 0; i < triangle_count; ++i) {
		
		for (u32 v = 0; v < 3; ++v) {
			v2 t = triangle_vertices[v];
			t = v2_rotate(t, shape_rdata[i].rotation);
			t = v2_mul(t, triangles[i].size);
			t = v2_add(t, shape_rdata[i].pos);
			vertices_a[v] = t;
		}

		for (u32 j = i + 1; j < triangle_count; ++j) {
			for (u32 v = 0; v < 3; ++v) {
				v2 t = triangle_vertices[v];
				t = v2_rotate(t, shape_rdata[j].rotation);
				t = v2_mul(t, triangles[j].size);
				t = v2_add(t, shape_rdata[j].pos);
				vertices_b[v] = t;
			}
			separation c = {0};
			bool colliding = VerticesColliding(vertices_a, 3, vertices_b, 3, &c);
			if (!colliding) continue;
		}
	}

	BumpFree(vertices_a);
}

void GameRender() {
	RenderCircles(circles_rdata, circle_count);
	RenderShapes(shape_rdata, box_count + triangle_count);
}
