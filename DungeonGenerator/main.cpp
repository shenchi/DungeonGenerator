#include <Windows.h>

#define CHECK(x) if (!(x)) { DWORD ret = GetLastError(); return ret; }

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	const wchar_t* pClassName = L"DungeonGenerator";

	WNDCLASS wndCls{};
	wndCls.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
	wndCls.hInstance = hInstance;
	wndCls.lpfnWndProc = WndProc;
	wndCls.lpszClassName = pClassName;
	wndCls.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;

	CHECK(RegisterClass(&wndCls));

	HWND hWnd = CreateWindow(pClassName, pClassName, WS_TILEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, nullptr, nullptr, hInstance, nullptr);
	CHECK(hWnd);

	ShowWindow(hWnd, SW_SHOW);

	bool running = true;
	MSG msg;
	while (running)
	{
		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				running = false;
				break;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	CHECK(UnregisterClass(pClassName, hInstance));

	return 0;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}