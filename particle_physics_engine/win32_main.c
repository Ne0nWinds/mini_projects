#include "general.h"
#include <Windows.h>
#include "memory.h"
#include "glad/glad.h"
#include "game.h"
#include "renderer.h"
#include "benchmark.h"

// windowsx.h
#define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))

#define _AMD64_
#include <winuser.h>
#include <fileapi.h>

static v2 keyboard_vector = {0};

static u32 window_width = 1280, window_height = 720;
static bool is_running = true;
static game_input input = {0};

#define WGL_CONTEXT_MAJOR_VERSION_ARB           0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB           0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB             0x2093
#define WGL_CONTEXT_FLAGS_ARB                   0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB            0x9126

#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB          0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002

HGLRC (*wglCreateContextAttribsARB)(HDC hDC, HGLRC hshareContext, const int *attribList);

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

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
		CW_USEDEFAULT, CW_USEDEFAULT, window_width, window_height,
		NULL, NULL, hInstance, NULL
	);
	if (!hwnd) return 1;

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
	
	HDC hdc = GetDC(hwnd);
	int pixel_format = ChoosePixelFormat(hdc, &pfd);
	SetPixelFormat(hdc, pixel_format, &pfd);

	HGLRC dummy_context = wglCreateContext(hdc);
	if (!dummy_context) return 1;
	if (!wglMakeCurrent(hdc, dummy_context)) return 1;
	
	wglCreateContextAttribsARB = wglGetProcAddress("wglCreateContextAttribsARB");
	
	if (wglCreateContextAttribsARB) {
		int attribs[] = {
			WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
			WGL_CONTEXT_MINOR_VERSION_ARB, 3,
			WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
			0
		};
	
		HGLRC gl_context = wglCreateContextAttribsARB(hdc, 0, attribs);
		if (gl_context && wglMakeCurrent(hdc, gl_context)) {
			wglDeleteContext(dummy_context);
		}
	}
	
	if (!gladLoadGL()) return 1;
	if (!InitRenderer()) return 1;
	
	ResizeWindow(window_width, window_height);
	ShowWindow(hwnd, nCmdShow);
	
	glEnable(GL_BLEND);  	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	LARGE_INTEGER start = {0};
	QueryPerformanceCounter(&start);
	
	LARGE_INTEGER performance_frequency = {0};
	QueryPerformanceFrequency(&performance_frequency);
	
	f64 tick_rate = 1000.0 / 120.0;
	
	BenchmarkInit();
	GameInit();
	f64 accumulated_time = 0.0;
	
	MSG msg = { };
	while (is_running) {
		
		while (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE)) {
			DispatchMessage(&msg);
		}
		
		LARGE_INTEGER now = {0};
		QueryPerformanceCounter(&now);
		f64 delta = (f64)(now.QuadPart - start.QuadPart) / (f64)performance_frequency.QuadPart * 1000.0;		
		accumulated_time += delta;
		start = now;

		input.movement = keyboard_vector;

		if (accumulated_time >= tick_rate) {
			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			BenchmarkStart();
			do {
				accumulated_time -= tick_rate;
				GameMain(tick_rate, input);
			} while (accumulated_time >= tick_rate);
			BenchmarkEnd();
			GameRender();
			SwapBuffers(hdc);
#ifdef DEBUG
			// OutputDebugString(BenchmarkInfoToString());
#endif
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
			ResizeWindow(window_width, window_height);
			return 0;
		}
		case WM_KEYDOWN: {
			if (wParam == 'W') up = true;
			if (wParam == 'A') left = true;
			if (wParam == 'S') down = true;
			if (wParam == 'D') right = true;
			
			keyboard_vector = (v2){
				right - left, up - down
			};
			keyboard_vector = v2_normalize(keyboard_vector);
			
			return 0;
		}
		case WM_KEYUP: {
			if (wParam == 'W') up = false;
			if (wParam == 'A') left = false;
			if (wParam == 'S') down = false;
			if (wParam == 'D') right = false;
			
			keyboard_vector = (v2){
				right - left, up - down
			};
			keyboard_vector = v2_normalize(keyboard_vector);
			
			return 0;
		}
		case WM_LBUTTONDOWN: {
			input.buttons |= LMOUSE;

			v2 mouse_pos = {
				.x = GET_X_LPARAM(lParam),
				.y = GET_Y_LPARAM(lParam)
			};
			
			ScreenToWorld(&mouse_pos, window_width, window_height);
			
			input.mouse_pos = mouse_pos;
			
			return 0;
		}
		case WM_MOUSEMOVE: {
			v2 mouse_pos = {
				.x = GET_X_LPARAM(lParam),
				.y = GET_Y_LPARAM(lParam)
			};
			
			ScreenToWorld(&mouse_pos, window_width, window_height);
			
			input.mouse_pos = mouse_pos;
			
			return 0;
		}
		case WM_LBUTTONUP: {
			input.buttons &= ~LMOUSE;
			return 0;
		}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
