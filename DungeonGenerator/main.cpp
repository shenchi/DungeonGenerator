#include <Windows.h>
#include <chrono>
#include <ctime>
#include "MapGenerator.h"

using namespace std;

#define CHECK(x) if (!(x)) { DWORD ret = GetLastError(); return ret; }

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

namespace
{
	MapGenerator mapGen;
	constexpr char tileTable[NumTileType] = { ' ', '.', '#' };
	char* map = nullptr;
	size_t mapWidth = 0;
	size_t mapHeight = 0;
	int centerX = 0;
	int centerY = 0;
	bool drawGridMap = false;
	RECT clientRect;
	int gridStep;
}

float g_DPIScaleX, g_DPIScaleY, g_DrawingScale;

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	const wchar_t* pClassName = L"DungeonGenerator";
	const wchar_t* pTitleName = L"Dungeon Generator (press Q to toggle grid view)";

	WNDCLASS wndCls{};
	wndCls.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
	wndCls.hInstance = hInstance;
	wndCls.lpfnWndProc = WndProc;
	wndCls.lpszClassName = pClassName;
	wndCls.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;

	CHECK(RegisterClass(&wndCls));

	HWND hWnd = CreateWindow(pClassName, pTitleName, WS_TILEDWINDOW & ~(WS_MAXIMIZEBOX | WS_SIZEBOX), CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, nullptr, nullptr, hInstance, nullptr);
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

	GetClientRect(hWnd, &clientRect);
	centerX = (clientRect.left + clientRect.right) / 2;
	centerY = (clientRect.top + clientRect.bottom) / 2;
	g_DrawingScale = min((clientRect.right - clientRect.left), (clientRect.bottom - clientRect.top)) / (float)(genRadius * 10 + genMaxLength);

	chrono::milliseconds interval{ 33 };
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
		else if (nullptr == map)
		{
			size_t w = mapWidth = mapGen.Right() - mapGen.Left() + 1;
			size_t h = mapHeight = mapGen.Bottom() - mapGen.Top() + 1;
			map = new char[mapWidth * mapHeight];
			gridStep = min(
				(clientRect.right - clientRect.left + 1) / mapWidth,
				(clientRect.bottom - clientRect.top + 1) / mapHeight);
			mapGen.Gen2DArrayMap(map, w, h, tileTable);
		}
	}

	delete[] map;

	CHECK(UnregisterClass(pClassName, hInstance));

	return 0;
}

void DrawLine(HDC hdc, int x1, int y1, int x2, int y2)
{
	MoveToEx(hdc,
		int(centerX + x1 * g_DPIScaleX * g_DrawingScale),
		int(centerY + y1 * g_DPIScaleY * g_DrawingScale),
		nullptr);

	LineTo(hdc,
		int(centerX + x2 * g_DPIScaleX * g_DrawingScale),
		int(centerY + y2 * g_DPIScaleY * g_DrawingScale));
}

void DrawBox(HDC hdc, int left, int top, int right, int bottom)
{
	Rectangle(hdc,
		int(centerX + left * g_DPIScaleX * g_DrawingScale),
		int(centerY + top * g_DPIScaleY * g_DrawingScale),
		int(centerX + (right + 1) * g_DPIScaleX * g_DrawingScale - 1),
		int(centerY + (bottom + 1) * g_DPIScaleY * g_DrawingScale - 1));
}

void DrawVectorMap(HWND hWnd)
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

	SelectObject(ps.hdc, lightRedPen);
	{
		int left = mapGen.Left();
		int top = mapGen.Top();
		int right = mapGen.Right();
		int bottom = mapGen.Bottom();

		DrawLine(ps.hdc, left, top, right, top);
		DrawLine(ps.hdc, right, top, right, bottom);
		DrawLine(ps.hdc, right, bottom, left, bottom);
		DrawLine(ps.hdc, left, bottom, left, top);
	}

	SelectObject(ps.hdc, bluePen);
	SelectObject(ps.hdc, blueBrush);
	auto corridors = mapGen.GetCorridors();
	for (auto c = corridors.begin(); c != corridors.end(); ++c)
	{
		int sx, sy, tx, ty;
		c->rect(sx, sy, tx, ty);

		DrawBox(ps.hdc, sx, sy, tx, ty);
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

		DrawBox(ps.hdc, i->x, i->y, i->x + i->width - 1, i->y + i->height - 1);
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
				DrawLine(ps.hdc, (int)cells[i].cx(), (int)cells[i].cy(), (int)cells[i].cx(), (int)cells[i].cy());
			}
		}
	}

	SelectObject(ps.hdc, originalPen);
	SelectObject(ps.hdc, originalBrush);
	DeleteObject(blackPen);
	DeleteObject(redPen);
	DeleteObject(lightRedPen);
	DeleteObject(lightGrayPen);
	DeleteObject(bluePen);
	DeleteObject(lightBluePen);
	DeleteObject(blueBrush);
	DeleteObject(lightBlueBrush);
	DeleteObject(lightRedBrush);
	EndPaint(hWnd, &ps);
}

void DrawGridBox(HDC hdc, int x, int y)
{
	Rectangle(hdc, clientRect.left + x * gridStep, 
		clientRect.top + y * gridStep,
		clientRect.left + (x + 1) * gridStep ,
		clientRect.top + (y + 1) * gridStep);
}

void DrawGridMap(HWND hWnd)
{
	PAINTSTRUCT ps;
	BeginPaint(hWnd, &ps);
	HPEN blackPen = CreatePen(PS_SOLID, 1, 0);
	HBRUSH lightBlueBrush = CreateSolidBrush(RGB(200, 200, 255));
	HBRUSH lightRedBrush = CreateSolidBrush(RGB(255, 200, 200));
	HGDIOBJ originalPen = SelectObject(ps.hdc, blackPen);
	HGDIOBJ originalBrush = SelectObject(ps.hdc, lightBlueBrush);


	for (size_t y = 0; y < mapHeight; ++y)
	{
		for (size_t x = 0; x < mapWidth; ++x)
		{
			switch (map[y * mapWidth + x])
			{
			case tileTable[Walkable]:
				SelectObject(ps.hdc, lightBlueBrush);
				break;
			case tileTable[Wall]:
				SelectObject(ps.hdc, lightRedBrush);
				break;
			case tileTable[Void]:
			default:
				SelectObject(ps.hdc, originalBrush);
				break;
			}
			DrawGridBox(ps.hdc, x, y);
		}
	}

	SelectObject(ps.hdc, originalPen);
	SelectObject(ps.hdc, originalBrush);
	DeleteObject(blackPen);
	DeleteObject(lightBlueBrush);
	DeleteObject(lightRedBrush);
	EndPaint(hWnd, &ps);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_KEYDOWN:
		if ((int)wParam == int('Q'))
		{
			drawGridMap = !drawGridMap;
			InvalidateRect(hWnd, nullptr, TRUE);
		}
		break;
	case WM_PAINT:
		if (drawGridMap && nullptr != map)
		{
			DrawGridMap(hWnd);
		}
		else
		{
			DrawVectorMap(hWnd);
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