#include "general.h"
#include <Windows.h>
#include "memory.h"
#include "glad/glad.h"
#include <math.h>

// windowsx.h
#define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))

#define WGL_CONTEXT_MAJOR_VERSION_ARB           0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB           0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB             0x2093
#define WGL_CONTEXT_FLAGS_ARB                   0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB            0x9126

#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB          0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002

HGLRC (*wglCreateContextAttribsARB)(HDC hDC, HGLRC hshareContext, const int *attribList);

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static bool is_running = true;

u32 compute_program = 0;
u32 parameters_ubo = 0;

GLuint texture1 = 0;
GLuint texture2 = 0;
GLuint render_texture = 0;

u32 window_width = 1280, window_height = 720;
u32 compute_initializer_program = 0;
bool even_frame = 1;

struct cca_parameters {
	s32 range;
	u32 n_states;
	u32 threshold;
} cca_parameters;

void SizeWindow() {
	glViewport(0, 0, window_width, window_height);
	
	glDeleteTextures(1, &texture1);
	glDeleteTextures(1, &texture2);
	
	glCreateTextures(GL_TEXTURE_2D, 1, &texture1);
	glTextureParameteri(texture1, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(texture1, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(texture1, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureParameteri(texture1, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureStorage2D(texture1, 1, GL_RGBA32F, window_width / 2, window_height / 2);
	
	glCreateTextures(GL_TEXTURE_2D, 1, &texture2);
	glTextureParameteri(texture2, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(texture2, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(texture2, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureParameteri(texture2, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureStorage2D(texture2, 1, GL_RGBA32F, window_width / 2, window_height / 2);
	
	glCreateTextures(GL_TEXTURE_2D, 1, &render_texture);
	glTextureParameteri(render_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(render_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(render_texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureParameteri(render_texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureStorage2D(render_texture, 1, GL_RGBA32F, window_width / 2, window_height / 2);
	
	// reset automata state
	even_frame = 1;
	glUseProgram(compute_initializer_program);
	glBindImageTexture(0, texture1, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
	glDispatchCompute(ceilf(window_width / 8.0f), ceilf(window_height / 4.0f), 1);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

u8 *ReadEntireFile(char *file_path, u32 *file_size) {
	HANDLE file_handle = CreateFile(file_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file_handle == INVALID_HANDLE_VALUE) return 0;
	
	*file_size = GetFileSize(file_handle, 0);
	if (*file_size == 0) return 0;
	
	u8 *buffer = BumpAlloc(*file_size + 1);
	
	DWORD bytes_read = 0;
	ReadFile(file_handle, buffer, *file_size, &bytes_read, NULL);
	CloseHandle(file_handle);
	if (bytes_read != *file_size) {
		*file_size = 0;
		BumpFree(buffer);
		return 0;
	};
	
	return buffer;
}

static u32 CreateComputeShader(char *compute_src) {
	u32 shader_program = glCreateProgram();
	
	u32 compute_shader = glCreateShader(GL_COMPUTE_SHADER);
	u32 file_size = 0;
	u8 *compute = ReadEntireFile(compute_src, &file_size);
	if (!compute) return 0;
	glShaderBinary(1, &compute_shader, GL_SHADER_BINARY_FORMAT_SPIR_V, compute, file_size);
	glSpecializeShader(compute_shader, "main", 0, 0, 0);
	BumpFree(compute);

	GLint compile_successful = GL_FALSE;
	glGetShaderiv(compute_shader, GL_COMPILE_STATUS, &compile_successful);
	if (compile_successful == GL_FALSE) {
		GLint max_length = 0;
		glGetShaderiv(compute_shader, GL_INFO_LOG_LENGTH, &max_length);
		
		char *info = BumpAlloc(max_length + 1);
		glGetShaderInfoLog(compute_shader, 1024, NULL, info);
		OutputDebugString(info);
		BumpFree(info);
		return 0;
	}
	
	glAttachShader(shader_program, compute_shader);
	glLinkProgram(shader_program);
	
	glDeleteShader(compute_shader);

	return shader_program;
}

static u32 CreateShader(char *vertex_src, char *fragment_src) {
	u32 shader_program = glCreateProgram();
	u32 file_size = 0;
	
	u32 vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	u8 *vertex = ReadEntireFile(vertex_src, &file_size);
	if (!vertex) return 0;
	glShaderBinary(1, &vertex_shader, GL_SHADER_BINARY_FORMAT_SPIR_V, vertex, file_size);
	glSpecializeShader(vertex_shader, "main", 0, 0, 0);
	BumpFree(vertex);

	GLint compile_successful = GL_FALSE;
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &compile_successful);
	if (compile_successful == GL_FALSE) {
		GLint max_length = 0;
		glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &max_length);
		
		char *info = BumpAlloc(max_length + 1);
		glGetShaderInfoLog(vertex_shader, 1024, NULL, info);
		OutputDebugString(info);
		BumpFree(info);
		return 0;
	}
	
	u32 fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	u8 *fragment = ReadEntireFile(fragment_src, &file_size);
	if (!fragment) return 0;
	glShaderBinary(1, &fragment_shader, GL_SHADER_BINARY_FORMAT_SPIR_V, fragment, file_size);
	glSpecializeShader(fragment_shader, "main", 0, 0, 0);
	BumpFree(fragment);
	
	compile_successful = GL_FALSE;
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &compile_successful);
	if (compile_successful == GL_FALSE) {
		GLint max_length = 0;
		glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &max_length);
		
		char *info = BumpAlloc(max_length + 1);
		glGetShaderInfoLog(fragment_shader, 1024, NULL, info);
		OutputDebugString(info);
		BumpFree(info);
		return 0;
	}

	glAttachShader(shader_program, vertex_shader);
	glAttachShader(shader_program, fragment_shader);
	glLinkProgram(shader_program);
	
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	return shader_program;
}


f32 random() {
	static f32 seed = 0.0f;
	f32 result = sinf(seed * 123.3814) * 345.9628;
	seed += 0.322918f;
	if (result < 0.0f) result *= -1.0f;
	return result - (u32)result;
}

u32 randint(u32 min, u32 max) {
	f32 r = random();
	r *= max - min;
	r += min;
	return (u32)r;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
  
	const char CLASS_NAME[]  = "Physics Engine";	
	WNDCLASS wc = {0};
	wc.lpfnWndProc   = WindowProc;
	wc.hInstance	 = hInstance;
	wc.lpszClassName = CLASS_NAME;
	RegisterClass(&wc);

	HWND hwnd = CreateWindowEx(
		0,
		CLASS_NAME,
		"Physics Engine",
		WS_OVERLAPPEDWINDOW | CS_OWNDC,
		CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720,
		NULL, NULL, hInstance, NULL
	);
	if (!hwnd) return 1;

	BumpInit();

	PIXELFORMATDESCRIPTOR pfd = {
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
	
	HDC hdc = GetDC(hwnd);
	int pixel_format = ChoosePixelFormat(hdc, &pfd);
	SetPixelFormat(hdc, pixel_format, &pfd);

	HGLRC dummy_context = wglCreateContext(hdc);
	if (!dummy_context) return 1;
	if (!wglMakeCurrent(hdc, dummy_context)) return 1;
	
	wglCreateContextAttribsARB = (void *)wglGetProcAddress("wglCreateContextAttribsARB");
	
	if (wglCreateContextAttribsARB) {
		int attribs[] = {
			WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
			WGL_CONTEXT_MINOR_VERSION_ARB, 6,
			WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
			0
		};
	
		HGLRC gl_context = wglCreateContextAttribsARB(hdc, 0, attribs);
		if (gl_context && wglMakeCurrent(hdc, gl_context)) {
			wglDeleteContext(dummy_context);
		}
	}
	
	if (!gladLoadGL()) return 1;
	
	compute_program = CreateComputeShader("./shaders/compute.spv");
	if (!compute_program) return 1;
	
	compute_initializer_program = CreateComputeShader("./shaders/compute_initialize.spv");
	if (!compute_initializer_program) return 1;

	u32 render_program = CreateShader("./shaders/vertex.spv", "./shaders/fragment.spv");
	if (!render_program) return 1;
	
	f32 vertices[] = {
		-1.0f, -1.0f , 0.0f, 0.0f, 0.0f,
		-1.0f,  1.0f , 0.0f, 0.0f, 1.0f,
		 1.0f,  1.0f , 0.0f, 1.0f, 1.0f,
		 1.0f, -1.0f , 0.0f, 1.0f, 0.0f,
	};

	u32 indices[] = {
		0, 2, 1,
		0, 3, 2
	};
	
	u32 vao = 0, vbo = 0, ebo = 0;
	glCreateVertexArrays(1, &vao);
	glCreateBuffers(1, &vbo);
	glCreateBuffers(1, &ebo);
	
	glNamedBufferData(vbo, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glNamedBufferData(ebo, sizeof(vertices), indices, GL_STATIC_DRAW);
	
	glEnableVertexArrayAttrib(vao, 0);
	glVertexArrayAttribBinding(vao, 0, 0);
	glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
	
	glEnableVertexArrayAttrib(vao, 1);
	glVertexArrayAttribBinding(vao, 1, 0);
	glVertexArrayAttribFormat(vao, 1, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(f32));
	
	glVertexArrayVertexBuffer(vao, 0, vbo, 0, 5 * sizeof(f32));
	glVertexArrayElementBuffer(vao, ebo);

	cca_parameters.n_states = 14;
	cca_parameters.range = 1;
	cca_parameters.threshold = 1;

	parameters_ubo = 0;
	glCreateBuffers(1, &parameters_ubo);
	glNamedBufferData(parameters_ubo, sizeof(cca_parameters), &cca_parameters, GL_DYNAMIC_DRAW);

	RECT client_rect = {0};
	GetClientRect(hwnd, &client_rect);
	window_width = client_rect.right - client_rect.left;
	window_height = client_rect.bottom - client_rect.top;
	SizeWindow();
	ShowWindow(hwnd, nCmdShow);
	
	LARGE_INTEGER start = {0};
	LARGE_INTEGER performance_frequency = {0};
	f64 accumulated_time = 0.0;
	f64 tick_rate = 1000.0 / 30.0;
	QueryPerformanceFrequency(&performance_frequency);	
	QueryPerformanceCounter(&start);
	
	while (is_running) {
		static MSG msg;
		while (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE)) {
			DispatchMessage(&msg);
		}
		
		LARGE_INTEGER now = {0};
		QueryPerformanceCounter(&now);
		f64 delta = (f64)(now.QuadPart - start.QuadPart) / (f64)performance_frequency.QuadPart * 1000.0;		
		accumulated_time += delta;
		start = now;

		if (accumulated_time >= tick_rate) {
			glUseProgram(compute_program);
			glBindImageTexture(2, render_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
			glBindBufferBase(GL_UNIFORM_BUFFER, 0, parameters_ubo);
			glUniformBlockBinding(compute_program, 0, 0);
			do {
				accumulated_time -= tick_rate;
				glBindImageTexture(0, (even_frame) ? texture1 : texture2, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
				glBindImageTexture(1, (even_frame) ? texture2 : texture1, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
				glDispatchCompute(ceilf(window_width / 8.0f), ceilf(window_height / 4.0f), 1);
				glMemoryBarrier(GL_ALL_BARRIER_BITS);
				even_frame = !even_frame;
			} while (accumulated_time >= tick_rate);
			glUseProgram(render_program);
			glBindBufferBase(GL_UNIFORM_BUFFER, 0, parameters_ubo);
			glUniformBlockBinding(compute_program, 0, 0);
			glBindTextureUnit(0, render_texture);
			glUniform1i(0, 0);
			glBindVertexArray(vao);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			SwapBuffers(hdc);
		}
	}
	
	ExitProcess(0);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static bool up;
	static bool down;
	static bool left;
	static bool right;
	
	switch (uMsg)
	{
		case WM_DESTROY: {
			PostQuitMessage(0);
			is_running = false;
			return 0;
		}
		case WM_SIZE: {
			window_width  = LOWORD(lParam);
			window_height = HIWORD(lParam);
			SizeWindow();
			return 0;
		}
		case WM_KEYDOWN: {
			if (wParam == 'R') {
				SizeWindow();
				return 0;
			}
			if (wParam == ' ') {
				cca_parameters.n_states = randint(5, 15);
				cca_parameters.range = randint(1, 5);
				cca_parameters.threshold = randint(1, 5);
				glNamedBufferSubData(parameters_ubo, 0, sizeof(cca_parameters), &cca_parameters);
				return 0;
			}
		}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
