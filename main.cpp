#include <windows.h>
#include <cstdint>
#include <cmath>

#define internal_func static 
#define global_variable static 
#define local_persist static

global_variable bool Running;

global_variable BITMAPINFO BitMapInfo;
global_variable void* BitmapMemory;
global_variable int BitmapWidth;
global_variable int BitmapHeight;
global_variable int BytesPerPixel = 4;

internal_func void Render(int xOffset, int yOffset) {
	int Width = BitmapWidth;
	int Height = BitmapHeight;
	int Pitch = BitmapWidth * BytesPerPixel;
	uint8_t* Row = (uint8_t*)BitmapMemory;

	//Row by row
	for (int y = 0; y < Height; y++) {
		uint32_t* Pixel = (uint32_t*)Row;
		for (int x = 0; x < Width; x++) {
			uint8_t B = (uint8_t)(x + xOffset);
			uint8_t G = (uint8_t)(y + yOffset);
			uint8_t R = 0;

			*Pixel++ = ((G << 8) | B);
		}
		Row += Pitch;
	}
}

internal_func void
Win32ResizeDIBSection(int width, int height)
{
	if (BitmapMemory) {
		VirtualFree(BitmapMemory, 0, MEM_RELEASE);
	}

	BitmapWidth = width;
	BitmapHeight = height;

	BitMapInfo.bmiHeader.biSize = sizeof(BitMapInfo.bmiHeader);
	BitMapInfo.bmiHeader.biWidth = BitmapWidth;
	BitMapInfo.bmiHeader.biHeight = -BitmapHeight;
	BitMapInfo.bmiHeader.biPlanes = 1;
	BitMapInfo.bmiHeader.biBitCount = 32;
	BitMapInfo.bmiHeader.biCompression = BI_RGB;

	int BitmapMemorySize = BytesPerPixel * (BitmapWidth * BitmapHeight);

	BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

internal_func void
Win32UpdateWindow(RECT* WindowRect, HDC DeviceContest, int x, int y, int width, int height)
{
	int WindowWidth = WindowRect->right - WindowRect->left;
	int WindowHeight = WindowRect->bottom - WindowRect->top;

	StretchDIBits(DeviceContest,
		0, 0, BitmapWidth, BitmapHeight, 0, 0, WindowWidth, WindowHeight,
		BitmapMemory, &BitMapInfo, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK Win32WinprocCallback(HWND Window, UINT Message, WPARAM wParam, LPARAM LParam)
{
	LRESULT Result = 0;

	switch (Message) {
	case WM_SIZE:
	{
		RECT ClientRect;
		GetClientRect(Window, &ClientRect);
		LONG Width = ClientRect.right - ClientRect.left;
		LONG Height = ClientRect.bottom - ClientRect.top;
		Win32ResizeDIBSection(Width, Height);
	}break;

	case WM_DESTROY:
	{
		//TODO: Destroy windows as error occures and recreate it?
		Running = false;
	} break;
	case WM_CLOSE:
	{
		//TODO: Handle this with a message to user?
		Running = false;
	} break;

	case WM_ACTIVATEAPP:
	{
		OutputDebugStringA("WM_ACTIVATEAPP\n");
	} break;

	case WM_PAINT:
	{
		PAINTSTRUCT Paint;
		RECT ClientRect;

		HDC DeviceContext = BeginPaint(Window, &Paint);

		LONG Width = Paint.rcPaint.right - Paint.rcPaint.left;
		LONG Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
		LONG x = Paint.rcPaint.left;
		LONG y = Paint.rcPaint.top;

		GetClientRect(Window, &ClientRect);

		Win32UpdateWindow(&ClientRect, DeviceContext, x, y, Width, Height);
		EndPaint(Window, &Paint);
	}break;

	default:
	{
		//OutputDebugStringA("default\n");
		Result = DefWindowProc(Window, Message, wParam, LParam);
	} break;
	}

	return Result;
}

int CALLBACK main(
	HINSTANCE Instance,
	HINSTANCE PrevInstance,
	LPSTR CommandLine,
	int ShowCode)
{
	WNDCLASS WindowClass = {};
	WindowClass.style = CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = Win32WinprocCallback;
	WindowClass.hInstance = Instance;
	// WindowClass.hIcon;
	WindowClass.lpszClassName = L"HandmadeHeroWindowClass";

	if (RegisterClass(&WindowClass)) {
		HWND WindowHandle = CreateWindowEx(0, WindowClass.lpszClassName,
			L"HandmadeHero",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			0, 0, Instance, 0);

		if (WindowHandle) {
			Running = true;

			int xOffset = 0;
			int yOffset = 0;

			while (Running) {
				MSG Message;

				while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE)) {
					if (Message.message == WM_QUIT) {
						Running = false;
					}

					TranslateMessage(&Message);
					DispatchMessageA(&Message);
				}
				Render(xOffset, yOffset);

				HDC DeviceContext = GetDC(WindowHandle);
				RECT ClientRect;

				GetClientRect(WindowHandle, &ClientRect);
				int WindowWidth = ClientRect.right - ClientRect.left;
				int WindowHeight = ClientRect.bottom - ClientRect.top;

				Win32UpdateWindow(&ClientRect, DeviceContext, 0, 0, WindowWidth, WindowHeight);
				++xOffset;

				ReleaseDC(WindowHandle, DeviceContext);
			}
		}
		else {
			//TODO: Logging
		}
	}
	else {
		//TODO: Logging
	}


	return 0;
}