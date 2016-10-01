#include <Windows.h>
#include <chrono>
#include <ctime>
#include "MapGenerator.h"

using namespace std;

#define CHECK(x) if (!(x)) { DWORD ret = GetLastError(); return ret; }

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

MapGenerator mapGen;
float g_DPIScaleX, g_DPIScaleY, g_DrawingScale;

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

	const int genCount = 50;
	const int genRadius = 10;
	const int genMinLength = 3;
	const int genMaxLength = 10;

	mapGen.SetSeed((int)std::time(NULL));

	mapGen.Start(genCount, genRadius, genMinLength, genMaxLength);

	HDC hdc = GetDC(hWnd);
	g_DPIScaleX = GetDeviceCaps(hdc, LOGPIXELSX) / 96.0f;
	g_DPIScaleY = GetDeviceCaps(hdc, LOGPIXELSY) / 96.0f;
	ReleaseDC(hWnd, hdc);

	RECT clientRect;
	GetClientRect(hWnd, &clientRect);
	g_DrawingScale = min((clientRect.right - clientRect.left), (clientRect.bottom - clientRect.top)) / (float)(genRadius * 10 + genMaxLength);

	chrono::milliseconds interval{33};
	auto last_frame_time = chrono::high_resolution_clock::now();
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

		auto now_time = chrono::high_resolution_clock::now();
		if (now_time - last_frame_time < interval)
			continue;

		last_frame_time = now_time;

		if (!mapGen.IsFinished())
		{
			mapGen.Update();
			InvalidateRect(hWnd, nullptr, TRUE);
		}
	}

	CHECK(UnregisterClass(pClassName, hInstance));

	return 0;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			BeginPaint(hWnd, &ps);
			HPEN blackPen = CreatePen(PS_SOLID, 1, 0);
			HPEN redPen = CreatePen(PS_SOLID, 2, RGB(255, 0, 0));
			HPEN lightRedPen = CreatePen(PS_DOT, 1, RGB(255, 200, 200));
			HPEN lightGrayPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
			HPEN bluePen = CreatePen(PS_SOLID, 1, RGB(100, 100, 255));
			HPEN lightBluePen = CreatePen(PS_SOLID, 1, RGB(200, 200, 255));
			HBRUSH blueBrush = CreateSolidBrush(RGB(100, 100, 255));
			HBRUSH lightBlueBrush = CreateSolidBrush(RGB(200, 200, 255));
			HBRUSH lightRedBrush = CreateSolidBrush(RGB(255, 200, 200));
			HGDIOBJ originalPen = SelectObject(ps.hdc, blackPen);
			HGDIOBJ originalBrush = SelectObject(ps.hdc, lightBlueBrush);

			RECT rc;
			GetClientRect(hWnd, &rc);
			int centerX = (rc.left + rc.right) / 2;
			int centerY = (rc.top + rc.bottom) / 2;

			SelectObject(ps.hdc, bluePen);
			SelectObject(ps.hdc, blueBrush);
			auto corridors = mapGen.GetCorridors();
			for (auto c = corridors.begin(); c != corridors.end(); ++c)
			{
				int sx = c->startX, sy = c->startY;
				int tx = c->endX, ty = c->endY;
				if (tx < sx) swap(sx, tx);
				if (ty < sy) swap(sy, ty);
				if (sx == tx)
				{
					sx -= c->width / 2;
					tx += (c->width - 1) / 2;
				}
				else
				{
					sy -= c->width / 2;
					ty += (c->width - 1) / 2;
				}

				Rectangle(ps.hdc,
					centerX + sx * g_DPIScaleX * g_DrawingScale,
					centerY + sy * g_DPIScaleY * g_DrawingScale,
					centerX + (tx + 1) * g_DPIScaleX * g_DrawingScale - 1,
					centerY + (ty + 1) * g_DPIScaleY * g_DrawingScale - 1);
			}

			auto cells = mapGen.GetCells();
			for (auto i = cells.begin(); i != cells.end(); ++i)
			{
				if (i->discard) continue;
				if (i->room)
				{
					SelectObject(ps.hdc, redPen);
					SelectObject(ps.hdc, lightRedBrush);
				}
				else if (i->discard)
				{
					SelectObject(ps.hdc, originalBrush);
					SelectObject(ps.hdc, lightGrayPen);
				}
				else
				{
					SelectObject(ps.hdc, lightBlueBrush);
					SelectObject(ps.hdc, bluePen);
				}
				Rectangle(ps.hdc,
					centerX + (i->x) * g_DPIScaleX * g_DrawingScale,
					centerY + (i->y) * g_DPIScaleY * g_DrawingScale,
					centerX + (i->x + i->width) * g_DPIScaleX * g_DrawingScale,
					centerY + (i->y + i->height) * g_DPIScaleY * g_DrawingScale);
			}

			SelectObject(ps.hdc, lightRedPen);
			auto connections = mapGen.GetConnections();
			size_t roomCount = cells.size();
			for (size_t i = 0; i < roomCount; ++i)
			{
				for (size_t j = i + 1; j < roomCount; ++j)
				{
					if (connections[i * roomCount + j])
					{
						MoveToEx(ps.hdc,
							centerX + cells[i].cx() * g_DPIScaleX * g_DrawingScale,
							centerY + cells[i].cy() * g_DPIScaleY * g_DrawingScale,
							nullptr);

						LineTo(ps.hdc,
							centerX + cells[j].cx() * g_DPIScaleX * g_DrawingScale,
							centerY + cells[j].cy() * g_DPIScaleY * g_DrawingScale);
					}
				}
			}

			SelectObject(ps.hdc, originalPen);
			SelectObject(ps.hdc, originalBrush);
			DeleteObject(blackPen);
			EndPaint(hWnd, &ps);
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}