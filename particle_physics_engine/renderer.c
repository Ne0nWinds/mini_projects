#include "general.h"
#include "vector.h"
#include "memory.h"
#include "renderer.h"
#include "fileio.h"

#include "glad/glad.h"

#define _AMD64_
#include <debugapi.h>

struct draw_elements_indirect_command {
	u32 count;
	u32 instance_count;
	u32 first_index;
	u32 base_vertex;
	u32 base_instance;
};
typedef struct draw_elements_indirect_command draw_elements_indirect_command;

static f32 quad_vertices[] = {
	-0.5f, 0.5f,
	0.5f, -0.5f,
	-0.5f, -0.5f,

	-0.5f, 0.5f,
	0.5f, 0.5f,
	0.5f, -0.5f
};

static u32 quad_vbo = 0;

static u32 circle_vao = 0;
static u32 circle_shader = 0;
static u32 circle_pos_buffer = 0;
static u32 circle_width_buffer = 0;

static u32 convex_shape_vao = 0;
static u32 convex_shape_vbo = 0;
static u32 convex_shape_ebo = 0;
static u32 convex_shape_shader = 0;
static u32 convex_shape_pos_buffer = 0;
static u32 convex_shape_size_buffer = 0;
static u32 convex_shape_indirect_buffer = 0;

static u32 CreateShader(char *vertex_src, char *fragment_src) {
	u32 shader_program = glCreateProgram();
	
	u32 vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	u32 vertex_file_size = 0;
	u8 *vertex = ReadEntireFile(vertex_src, &vertex_file_size);
	if (!vertex) return 0;
	glShaderSource(vertex_shader, 1, &vertex, (GLint *)&vertex_file_size);
	glCompileShader(vertex_shader);
	BumpFree(vertex);

	GLint compile_successful = 0;
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &compile_successful);
	if (compile_successful == GL_FALSE) {
#if DEBUG
		char *info = BumpAlloc(1024);
		glGetShaderInfoLog(vertex_shader, 1024, NULL, info);
		OutputDebugString(info);
		BumpFree(info);
#endif
		return 0;
	}
	
	u32 fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	u32 fragment_file_size = 0;
	u8 *fragment = ReadEntireFile(fragment_src, &fragment_file_size);
	if (!fragment) return 0;
	glShaderSource(fragment_shader, 1, &fragment, (GLint *)&fragment_file_size);
	glCompileShader(fragment_shader);
	BumpFree(fragment);

	compile_successful = 0;
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &compile_successful);
	if (compile_successful == GL_FALSE) {
#if DEBUG
		char *info = BumpAlloc(1024);
		glGetShaderInfoLog(fragment_shader, 1024, NULL, info);
		OutputDebugString(info);
		BumpFree(info);
#endif
		return 0;
	}
	
	glAttachShader(shader_program, vertex_shader);
	glAttachShader(shader_program, fragment_shader);
	glLinkProgram(shader_program);
	
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	return shader_program;
}

bool InitRenderer() {
	glGenVertexArrays(1, &circle_vao);
	glBindVertexArray(circle_vao);
	glGenBuffers(1, &quad_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	
	glGenBuffers(1, &circle_pos_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, circle_pos_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(v2) * 1024 * 1024, 0, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glVertexAttribDivisor(1, 1);
	
	glGenBuffers(1, &circle_width_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, circle_width_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(f32) * 1024 * 1024, 0, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 0, 0);
	glVertexAttribDivisor(2, 1);
	
	circle_shader = CreateShader("./shaders/circle_vertex.glsl", "./shaders/circle_fragment.glsl");
	if (!circle_shader) return false;
	
	f32 vertices[] = {
		// rectangle
		-0.5f, 0.5f,
		0.5f,  0.5f,
		0.5f,  -0.5f,
		-0.5f, -0.5f,
		
		// triangle
		0.0f,  0.5f,
		0.5f, -0.5f,
		-0.5f, -0.5f,
	};
	
	u32 indices[] = {
		0, 1, 2,
		0, 2, 3,

		0, 1, 2,
	};
	
	glGenVertexArrays(1, &convex_shape_vao);
	glBindVertexArray(convex_shape_vao);
	
	glGenBuffers(1, &convex_shape_indirect_buffer);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, convex_shape_indirect_buffer);
	glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(draw_elements_indirect_command) * 2, 0, GL_STATIC_DRAW);
	
	glGenBuffers(1, &convex_shape_ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, convex_shape_ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_DYNAMIC_DRAW);
	
	glGenBuffers(1, &convex_shape_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, convex_shape_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	
	glGenBuffers(1, &convex_shape_pos_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, convex_shape_pos_buffer);
	glBufferData(GL_ARRAY_BUFFER, 1024 * 1024, 0, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glVertexAttribDivisor(1, 1);

	glGenBuffers(1, &convex_shape_size_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, convex_shape_size_buffer);
	glBufferData(GL_ARRAY_BUFFER, 1024 * 1024, 0, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glVertexAttribDivisor(2, 1);
	
	convex_shape_shader = CreateShader("./shaders/shape_vertex.glsl", "./shaders/shape_fragment.glsl");
	if (!convex_shape_shader) return false;
	
	return true;
}

m3 camera;
v2 renderer_coordinates = {0};

void ResizeWindow(u32 window_width, u32 window_height) {
	glViewport(0, 0, window_width, window_height);
	
	renderer_coordinates = (v2){
		window_width / (f32)window_height,
		1.0f
	};
	
	camera = m3_identity();
	m3_camera(renderer_coordinates.x, renderer_coordinates.y, 1.0f, &camera);
}

void ScreenToWorld(v2 *mouse_pos, u32 window_width, u32 window_height) {
	mouse_pos->x = (mouse_pos->x - window_width / 2.0f) / window_width * 2.0f * renderer_coordinates.x;
	mouse_pos->y = (window_height / 2.0f - mouse_pos->y) / window_height * 2.0f * renderer_coordinates.y;
}

static u32 total_vertices = 0;
static u32 total_indices = 0;

draw_elements_indirect_command commands[3] = {
	{
		.count = 6,
		.instance_count = 0,
		.first_index = 0,
		.base_vertex = 0,
		.base_instance = 0,
	},
	{
		.count = 3,
		.instance_count = 0,
		.first_index = 6,
		.base_vertex = 4,
		.base_instance = 0,
	},
	{0}
};

static u32 total_shapes = 0;

void AddNewShape(enum shape_id shape_id, u32 count, v2 *sizes) {
	commands[shape_id].instance_count += count;
	commands[shape_id + 1].base_instance += count;

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, convex_shape_indirect_buffer);
	glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0, sizeof(draw_elements_indirect_command) * 2, commands);

	glBindBuffer(GL_ARRAY_BUFFER, convex_shape_size_buffer);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(v2) * total_shapes, sizeof(v2) * count, sizes);
	
	total_shapes += count;
}

void RemoveShapes(u32 index, u32 count) {
	
}

void RenderShapes(v2 *positions, u32 count) {
	glBindBuffer(GL_ARRAY_BUFFER, convex_shape_pos_buffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(f32) * 3 * count, positions);

	glUseProgram(convex_shape_shader);
	glBindVertexArray(convex_shape_vao);
	
	glUniformMatrix3fv(glGetUniformLocation(convex_shape_shader, "camera"), 1, GL_FALSE, &camera);
	glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0, 2, 0);
}

static u32 total_circles = 0;

void RenderCircles(v2 *circle_positions, u32 count) {
	glBindBuffer(GL_ARRAY_BUFFER, circle_pos_buffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(f32) * 3 * count, circle_positions);
	
	glUseProgram(circle_shader);
	glBindVertexArray(circle_vao);
	
	glUniformMatrix3fv(glGetUniformLocation(circle_shader, "camera"), 1, GL_FALSE, &camera);
	glDrawArraysInstanced(GL_TRIANGLES, 0, 6, total_circles);
}

void AddNewCircles(f32 *widths, u32 count) {
	glBindBuffer(GL_ARRAY_BUFFER, circle_width_buffer);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(f32) * total_circles, sizeof(f32) * count, widths);
	total_circles += count;
}

void RemoveCircles(u32 index, u32 count) {
	
}

