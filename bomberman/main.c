#include "general.h"
#include "gl_math.h"
#include "memory.h"
#include "glad/glad.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#include "stb_image.h"

#undef STBTT_malloc
#undef STBTT_free
#define STBTT_malloc(x, u) ((void)(u),BumpAlloc(x))
#define STBTT_free(x, u)

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

static u32 window_width = 1280;
static u32 window_height = 720;

static u32 seed = 0;

f32 random() {
	
	f64 r2 = cosf(seed * 2039.0) * 1201.0;
	
	seed += 49;
	
	r2 = r2 - (long)r2;
	return (r2 < 0) ? -r2 : r2;
}

f32 smoothstep(float edge0, float edge1, float x) {
   if (x < edge0)
      return 0;

   if (x >= edge1)
      return 1;

   // Scale/bias into [0..1] range
   x = (x - edge0) / (edge1 - edge0);

   return x * x * (3 - 2 * x);
}

f32 sign(f32 x) {
		return (x < 0.0f) ? -1.0f : 1.0f;
}

f32 absf(f32 x) {
		return (x > 0.0f) ? x : x * -1.0f;
}

static u32 CreateShaderProgram(char *vertex_path, char *fragment_path) {

	LARGE_INTEGER file_size = {0};
	DWORD bytes_read = 0;
	HANDLE file_handle = 0;

	// vertex shader
	file_handle = CreateFile(vertex_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!file_handle) return 0;

	if (!GetFileSizeEx(file_handle, &file_size)) return 0;
	char *vertex_src = BumpAlloc(file_size.QuadPart + 1);

	ReadFile(file_handle, vertex_src, file_size.QuadPart, &bytes_read, NULL);
	CloseHandle(file_handle);

	u32 vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_src, &bytes_read);
	glCompileShader(vertex_shader);
	BumpReset();

	GLint compile_successful = 0;
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &compile_successful);
	if (compile_successful == GL_FALSE) {
		char *info = BumpAlloc(1024);
		glGetShaderInfoLog(vertex_shader, 1024, NULL, info);
		OutputDebugString(info);
		BumpReset();
		return 0;
	}

	if (bytes_read != file_size.QuadPart) return 0;

	// fragment shader
	file_size = (LARGE_INTEGER){0};
	bytes_read = 0;
	file_handle = 0;

	file_handle = CreateFile(fragment_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!file_handle) return 0;

	if (!GetFileSizeEx(file_handle, &file_size)) return 0;
	char *fragment_src = BumpAlloc(file_size.QuadPart + 1);

	ReadFile(file_handle, fragment_src, file_size.QuadPart, &bytes_read, NULL);
	CloseHandle(file_handle);

	u32 fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_src, &bytes_read);
	glCompileShader(fragment_shader);
	BumpReset();

	compile_successful = 0;
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &compile_successful);
	if (compile_successful == GL_FALSE) {
		char *info = BumpAlloc(1024);
		glGetShaderInfoLog(fragment_shader, 1024, NULL, info);
		OutputDebugString(info);
		BumpReset();
		return 0;
	}

	u32 shader_program = glCreateProgram();
	glAttachShader(shader_program, vertex_shader);
	glAttachShader(shader_program, fragment_shader);
	glLinkProgram(shader_program);

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	return shader_program;
}

static bool is_running = true;
static v2 keyboard_vector;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	const char CLASS_NAME[]  = "Sample Window Class";	
	WNDCLASS wc = {0};
	wc.lpfnWndProc   = WindowProc;
	wc.hInstance	 = hInstance;
	wc.lpszClassName = CLASS_NAME;
	RegisterClass(&wc);

	HWND hwnd = CreateWindowEx(
		0,
		CLASS_NAME,
		"Learn OpenGL",
		WS_OVERLAPPEDWINDOW | CS_OWNDC,
		CW_USEDEFAULT, CW_USEDEFAULT, window_width, window_height,
		NULL, NULL, hInstance, NULL
	);
	if (!hwnd) return 0;

	BumpInit();

	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,// Flags
		PFD_TYPE_RGBA,    // The kind of framebuffer. RGBA or palette.
		32,               // Colordepth of the framebuffer.
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		24,               // Number of bits for the depthbuffer
		8,                // Number of bits for the stencilbuffer
		0,                // Number of Aux buffers in the framebuffer.
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};
	
	{
		SYSTEMTIME system_time = {0};
		GetSystemTime(&system_time);
		seed = system_time.wMilliseconds + system_time.wSecond;
	}

	HDC hdc = GetDC(hwnd);
	int pixel_format = ChoosePixelFormat(hdc, &pfd);
	SetPixelFormat(hdc, pixel_format, &pfd);
	HGLRC gl_context = wglCreateContext(hdc);
	if (!gl_context) return 0;
	if (!wglMakeCurrent(hdc, gl_context)) return 0;
	if (!gladLoadGL()) return 0;

	glClearColor(0.1, 0.1, 0.1f, 1.0f);

	LARGE_INTEGER start = {0};
	QueryPerformanceCounter(&start);

	u32 shader_program = CreateShaderProgram("./shaders/vertex.glsl", "./shaders/fragment.glsl");
	if (!shader_program) return 0;

	u32 player_shader_program = CreateShaderProgram("./shaders/player_vertex.glsl", "./shaders/player_fragment.glsl");
	if (!player_shader_program) return 0;
	
	u32 font_shader_program = CreateShaderProgram("./shaders/font_vertex.glsl", "./shaders/font_fragment.glsl");
	if (!font_shader_program) return 0;

	u32 texture_atlas = 0;
	{
		HANDLE file_handle = CreateFile("./textures/texture_atlas.png", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		DWORD file_size = GetFileSize(file_handle, 0);
		HANDLE file_mapping = CreateFileMapping(file_handle, NULL, PAGE_READONLY, 0, file_size, NULL);
		u8 *data = MapViewOfFile(file_mapping, FILE_MAP_READ, 0, 0, 0);
		stbi_set_flip_vertically_on_load(true);
		int x = 0, y = 0, nr_channels = 0;
		u8 *png_file_data = stbi_load_from_memory(data, file_size, &x, &y, &nr_channels, 0);
		if (!png_file_data) return 0;
		glGenTextures(1, &texture_atlas);
		glBindTexture(GL_TEXTURE_2D, texture_atlas);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, png_file_data);
	}
	
	u32 font_atlas = 0;
	{
		// HANDLE file_handle = CreateFile("./fonts/Franchise.ttf", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		// HANDLE file_handle = CreateFile("c:\\Windows\\Fonts\\Arial.ttf", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		// if (file_handle == INVALID_HANDLE_VALUE) return 0;
		
		// DWORD file_size = GetFileSize(file_handle, 0);
		// char *ttf_file = BumpAlloc(file_size);
		
		// DWORD bytes_read = 0;
		// ReadFile(file_handle, ttf_file, file_size, &bytes_read, NULL);
		// CloseHandle(file_handle);
		
		// stbtt_fontinfo font_info = {0};
		// stbtt_InitFont(&font_info, (u8 *)ttf_file, 0);
		
		// // u32 bitmap_width = 256;
		// // u32 bitmap_height = bitmap_width / 2;
		// f32 scale = stbtt_ScaleForPixelHeight(&font_info, 96);
		
		// s32 ascent = 0, baseline = 0, descent = 0;
		// stbtt_GetFontVMetrics(&font_info, &ascent, &descent, 0);
		
		// ascent *= scale;
		
		// wchar_t *text[] = {
		// 	L"Bomberman",
		// 	L"Start",
		// 	L"Configuración",
		// 	L"Exit",
		// };
		
		// s32 total_width = 0;
		
		// u8 *bitmap = BumpAlloc(bitmap_width * bitmap_height);
		
		// s32 y_offset = 0;
		
		// for (u32 i = 0; i < len(text); ++i) {
		// 	f32 x = 16;
		// 	for (u32 j = 0; j < wcslen(text[i]); ++j) {
		// 		s32 x0 = 0, y0 = 0, x1 = 0, y1 = 0;
		// 		f32 x_shift = x - (f32)floor(x);
		// 		stbtt_GetCodepointBitmapBoxSubpixel(&font_info, text[i][j], scale, scale, x_shift, 0, &x0, &y0, &x1, &y1);
				
		// 		s32 y = ascent + y0 + y_offset;
			
		// 		s32 byte_offset = (u32)roundf(x) + y * bitmap_width;
		// 		stbtt_MakeCodepointBitmapSubpixel(&font_info, bitmap + byte_offset, x1 - x0, y1 - y0, bitmap_width, scale, scale, x_shift, 0, text[i][j]);
			
		// 		s32 advance_width = 0;
		// 		s32 lsb = 0;
		// 		stbtt_GetCodepointHMetrics(&font_info, text[i][j], &advance_width, &lsb);
		// 		s32 kern = stbtt_GetCodepointKernAdvance(&font_info, text[i][j], text[i][j + 1]);
				
		// 		x += (x1 - x0) + lsb * scale;
		// 	}
		// 	y_offset += bitmap_height / 4;
		// }
		
		// s32 width = 0, height = 0;
		// s32 x_offset = 0, y_offset = 0;
		// u8 *bitmap = stbtt_GetCodepointSDF(&font_info, scale, 'a', 4, 128, 32, &width, &height, &x_offset, &y_offset);

		// glGenTextures(1, &font_atlas);
		// glBindTexture(GL_TEXTURE_2D, font_atlas);
		// glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap);
		
		// BumpReset();
	}

	u32 quad_vao = 0, quad_vbo = 0, texture_index_vbo = 0;
	glGenVertexArrays(1, &quad_vao);
	glBindVertexArray(quad_vao);

	f32 vertices[] = {
		-0.5f, 0.5f, 0.0f, 1.0f,
		0.5f, -0.5f, 1.0f, 0.0f,
		-0.5f, -0.5f, 0.0f, 0.0f,

		-0.5f, 0.5f, 0.0f, 1.0f,
		0.5f, 0.5f, 1.0f, 1.0f,
		0.5f, -0.5f, 1.0f, 0.0f
	};

	glGenBuffers(1, &quad_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void *)0);
	
	v2i *spawn_points = BumpAlloc(0);
	u32 spawn_point_count = 0;

	u32 tile_map[121] = { 0 };
	for (u32 y = 0; y < 11; ++y) {
		for (u32 x = 0; x < 11; ++x) {
			u32 texture_index = y & x & 1;
			tile_map[y * 11 + x] = texture_index;
			if (texture_index == 0 && !(y & 1 ^ x & 1)) {
				spawn_points[spawn_point_count].x = x;
				spawn_points[spawn_point_count].y = y;
				spawn_point_count += 1;
			}
		}
	}

	v2 player_positions[2] = { };
	
	v2i spawn_point1 = spawn_points[(u32)(random() * spawn_point_count)];
	v2i spawn_point2 = spawn_points[(u32)(random() * spawn_point_count)];
	
	player_positions[0].x = spawn_point1.x * 64.0f;
	player_positions[0].y = spawn_point1.y * 64.0f;
	
	tile_map[spawn_point1.y * 11 + spawn_point1.x] = 2;
	
	if (spawn_point1.x < 10)
		tile_map[spawn_point1.y * 11 + spawn_point1.x + 1] = 2;
	if (spawn_point1.x > 0)
		tile_map[spawn_point1.y * 11 + spawn_point1.x - 1] = 2;
	if (spawn_point1.y < 10)
		tile_map[(spawn_point1.y + 1) * 11 + spawn_point1.x] = 2;
	if (spawn_point1.y > 0)
		tile_map[(spawn_point1.y - 1) * 11 + spawn_point1.x] = 2;
	
	player_positions[1].x = spawn_point2.x * 64.0f;
	player_positions[1].y = spawn_point2.y * 64.0f;
	
	if (spawn_point2.x < 10)
		tile_map[spawn_point2.y * 11 + spawn_point2.x + 1] = 2;
	if (spawn_point2.x > 0)
		tile_map[spawn_point2.y * 11 + spawn_point2.x - 1] = 2;
	if (spawn_point2.y < 10)
		tile_map[(spawn_point2.y + 1) * 11 + spawn_point2.x] = 2;
	if (spawn_point2.y > 0)
		tile_map[(spawn_point2.y - 1) * 11 + spawn_point2.x] = 2;
	
	tile_map[spawn_point2.y * 11 + spawn_point2.x] = 2;

	glGenBuffers(1, &texture_index_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, texture_index_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(tile_map), tile_map, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, sizeof(u32), (void *)0);
	glVertexAttribDivisor(1, 1);

	u32 player_vao, player_positions_vbo = 0;
	glGenVertexArrays(1, &player_vao);
	glBindVertexArray(player_vao);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void *)0);

	glGenBuffers(1, &player_positions_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, player_positions_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(player_positions), player_positions, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(f32) * 2, (void *)0);
	glVertexAttribDivisor(1, 1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	ShowWindow(hwnd, nCmdShow);
	glViewport(0, 0, window_width, window_height);
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	v2 camera_pos[2] = {0};
	const f64 tick = 1000.0 / 120.0;

	while (is_running) {
		MSG msg = {0};

		u32 i = 0;
		while (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE)) {
			DispatchMessage(&msg);
		}

		v2 controller_input[2] = {0};

		XINPUT_STATE controller_state = {0};

		for (u32 i = 0; i < 2; ++i) {
			DWORD xinput_result = XInputGetState(i, &controller_state);
			if (xinput_result == ERROR_SUCCESS) {
				XINPUT_GAMEPAD gamepad = controller_state.Gamepad;
				if (gamepad.sThumbLX < 0) gamepad.sThumbLX += 1;
				if (gamepad.sThumbLY < 0) gamepad.sThumbLY += 1;
				
				controller_input[i] = (v2){ 
					((gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) > 0) - ((gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) > 0),
					((gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) > 0) - ((gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) > 0),
				};

				if (v2_length(controller_input[i]) == 0.0f) {
					controller_input[i] = (v2){ (f32)gamepad.sThumbLX, (f32)gamepad.sThumbLY };
					controller_input[i] = v2_scale(controller_input[i], 1.0f / 32767.0f);
				}

				f32 length = v2_length(controller_input[i]);
				

				if (length < 0.24f)
					controller_input[i] = (v2){ 0.0f, 0.0f };
				else if (length > 1.0f)
					controller_input[i] = v2_normalize(controller_input[i]);

				controller_input[i] = v2_scale(controller_input[i], tick * (1.0f / 64.0f));
			}
		}

		static f64 accumulated_time = 0.0;
		LARGE_INTEGER now = {0};
		QueryPerformanceCounter(&now);
		f64 delta = (f64)(now.QuadPart - start.QuadPart) / 1000.0;
		accumulated_time += delta;
		start = now;
		while (accumulated_time >= tick) {
			accumulated_time -= tick;
			for (u32 i = 0; i < 2; ++i) {
				player_positions[i] = v2_add(player_positions[i], controller_input[i]);
				
				v2 pos = player_positions[i];
				
				s32 left_tile_x = (player_positions[i].x - 12.0f) / 64.0f + 0.5f;
				s32 right_tile_x = (player_positions[i].x + 12.0f) / 64.0f + 0.5f;
				s32 top_tile_y = (player_positions[i].y - 12.0f) / 64.0f + 0.5f;
				s32 bottom_tile_y = (player_positions[i].y + 12.0f) / 64.0f + 0.5f;
				
				if (player_positions[i].x < -20.0f) player_positions[i].x = -20.0f;
				if (player_positions[i].y < -20.0f) player_positions[i].y = -20.0f;
				
				for (s32 x = left_tile_x; x <= right_tile_x; ++x) {
					for (s32 y = top_tile_y; y <= bottom_tile_y; ++y) {
						s32 index =  y * 11 + x;
						if (tile_map[index] < 2) {
							v2 tile_pos = { (f32)x * 64, (f32)y * 64 };
							v2 penetration = v2_sub(player_positions[i], tile_pos);
							v2 normalized = v2_normalize(penetration);
							f32 distance_scalar = 0.0f;
							if (absf(penetration.x) > absf(penetration.y)) {
								distance_scalar = 44.0 / normalized.x * sign(penetration.x);
							} else {
								distance_scalar = 44.0 / normalized.y * sign(penetration.y);
							}
							v2 final_relative_pos = v2_scale(normalized, distance_scalar);
							v2 final_pos = v2_add(tile_pos, final_relative_pos);
							
							player_positions[i] = final_pos;
						}
					}
				}
			}
		}

		f32 distance_between_players = 0.0f;
		distance_between_players = v2_length(v2_sub(player_positions[0], player_positions[1]));

		v2 new_center = v2_scale(v2_add(player_positions[0], player_positions[1]), 0.5f);

		glClear(GL_COLOR_BUFFER_BIT);
		
		glBindTexture(GL_TEXTURE_2D, texture_atlas);
		glUseProgram(shader_program);
		glBindVertexArray(quad_vao);
		f32 zoom_value = (1.0f - smoothstep(200.0, 680.0, distance_between_players)) * 0.75f + 1.0f;
		m4 ortho = m4_ortho(window_width / (f64)window_height * 704.0, 704.0, zoom_value, new_center);
		glUniformMatrix4fv(glGetUniformLocation(shader_program, "ortho"), 1, GL_FALSE, &ortho);
		m4 model = m4_scale(64.0f);
		glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, &model);
		glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 121);

		glViewport(0, 0, window_width, window_height);
		glUseProgram(player_shader_program);

		glBindBuffer(GL_ARRAY_BUFFER, player_positions_vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(player_positions), player_positions);

		glBindVertexArray(player_vao);

		glUniformMatrix4fv(glGetUniformLocation(player_shader_program, "ortho"), 1, GL_FALSE, &ortho);
		model = m4_scale(24.0f);
		glUniformMatrix4fv(glGetUniformLocation(player_shader_program, "model"), 1, GL_FALSE, &model);
		glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 2);
		
		glBindTexture(GL_TEXTURE_2D, font_atlas);
		glUseProgram(font_shader_program);
		// ortho = m4_ortho(window_width, window_height * 2.0f, 720.0f, (v2){0.0f, 0.0f});
		ortho = m4_ortho(window_width / (f64)window_height, 1.0f, 1.0f, (v2){ 0.0f, 0.0f });
		glUniformMatrix4fv(glGetUniformLocation(font_shader_program, "ortho"), 1, GL_FALSE, &ortho);
		glBindVertexArray(quad_vao);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		

		SwapBuffers(hdc);
	}
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static bool keyboard_up;
	static bool keyboard_down;
	static bool keyboard_left;
	static bool keyboard_right;

	switch (uMsg)
	{
		case WM_DESTROY: {
			PostQuitMessage(0);
			is_running = false;
			return 0;
		}
		case WM_SIZE: {
			window_width = LOWORD(lParam);
			window_height = HIWORD(lParam);
			return 0;
		}

		// TODO: Replace with raw keyboard input
		case WM_KEYUP: {
			switch (wParam) {
				case 'W': keyboard_up = false; break;
				case 'A': keyboard_left = false; break;
				case 'S': keyboard_down = false; break;
				case 'D': keyboard_right = false; break;
			}
			keyboard_vector = (v2){
				(f32)keyboard_right - (f32)keyboard_left,
				(f32)keyboard_up - (f32)keyboard_down
			};
			return 0;
		}
		case WM_KEYDOWN: {
			switch (wParam) {
				case 'W': keyboard_up = true; break;
				case 'A': keyboard_left = true; break;
				case 'S': keyboard_down = true; break;
				case 'D': keyboard_right = true; break;
			}

			keyboard_vector = (v2){
				(f32)keyboard_right - (f32)keyboard_left,
				(f32)keyboard_up - (f32)keyboard_down
			};
			return 0;
		}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
