// TileMap.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//
#pragma once
#define GRID_SIZE (16.0 * zoomFactor)
#define GRID_WIDTH 100
#define GRID_HEIGHT 50
#define RAND_SEED 3200

#include <windowsx.h>
#include "framework.h"
#include "TileMap.h"
#include <time.h>
#include <vector>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus")

#include "PathFinder.h"
#include "ButtonDefine.h"
#include "CellularAutomata.h"

using namespace Gdiplus;

ULONG_PTR g_GdiplusToken;

PathFinder g_pf{ GRID_WIDTH ,GRID_HEIGHT };
HBRUSH g_hTileBrush;
HPEN g_hGridPen;

// Execute시에 해당 정보가 TileMap의 List로 들어감.
char g_Tile[GRID_HEIGHT][GRID_WIDTH];

HBRUSH g_hOpenListBrush;
HBRUSH g_hCloseListBrush;
HBRUSH g_hNowPathBrush;
HBRUSH g_hNowSearchedBrush;

HBRUSH g_hStartPointBrush;
HBRUSH g_hEndPointBrush;

vector<HBRUSH> g_BrushList;
list<PathFinder::Node*> g_CloseListForRender;
// 벽 Edit
bool g_bErase = false;
bool g_bDrag = false;

// TEMP : 시작, 도착점 (윈도우 생성시 초기화할때만 사용)
pair<int, int> g_StartPoint;
pair<int, int> g_EndPoint;

// 조작
// Edit Mode <-> Execute Mode
bool g_bIsProcessing = false;

bool g_bEndPointEdit = false;
bool g_bStartPointEdit = false;
bool g_bIsSimulating = false;

// 줌
double zoomFactor = 1.0;

// 패닝
bool isDragging = false;
POINT dragStart = { 0, 0 };
int viewOffsetX = 0, viewOffsetY = 0;

void RenderGrid(HDC hdc, Graphics& g)
{
    float x = 0;
    float y = 0;
    // PEN
    HPEN h_oldpen = (HPEN)SelectObject(hdc, g_hGridPen);
    Pen pen(Color(255, 191, 191, 191), 2.5f);
    
    for (int i = 0;i <= GRID_WIDTH;i++)
    {
        //MoveToEx(hdc, x, 0, NULL);
        g.DrawLine(&pen, (REAL)x, (REAL)0, (REAL)x, (REAL)GRID_HEIGHT * GRID_SIZE);
        //LineTo(hdc, x, GRID_HEIGHT * GRID_SIZE);
        x += GRID_SIZE;
    }
    for (int i = 0;i <= GRID_HEIGHT;i++)
    {
        //MoveToEx(hdc, 0, y, NULL);
        g.DrawLine(&pen, (REAL)0, (REAL)y, (REAL)GRID_WIDTH * GRID_SIZE, (REAL)y);
        //LineTo(hdc, GRID_WIDTH * GRID_SIZE, y);
        y += GRID_SIZE;
    }
    SelectObject(hdc, h_oldpen);
}

void RenderObstacle(HDC hdc, Graphics& g)
{
    int x = 0;
    int y = 0;
    SolidBrush b(Color(255, 191, 191, 191));
    HBRUSH h_oldbrush = (HBRUSH)SelectObject(hdc, g_hTileBrush);
    SelectObject(hdc, GetStockObject(NULL_PEN));
    for (int w = 0;w < GRID_WIDTH;w++)
    {
        for (int h = 0;h < GRID_HEIGHT;h++)
        {
            if (g_Tile[h][w])
            {
                // 타일의 좌상단 좌표 (x,y)
                x = w * GRID_SIZE;
                y = h * GRID_SIZE;
                g.FillRectangle(&b, x, y, GRID_SIZE + 2, GRID_SIZE + 2);
            }
        }
    }
    SelectObject(hdc, h_oldbrush);
}

void PaintTile(HDC hdc, int posX, int posY, HBRUSH& brush, Graphics& g, PathFinder::Node* node = nullptr)
{
    int x = 0;
    int y = 0;

    LOGBRUSH lb;
    GetObject(brush, sizeof(LOGBRUSH), &lb);
    COLORREF color = lb.lbColor;
    HBRUSH h_oldbrush = (HBRUSH)SelectObject(hdc, brush);
    SelectObject(hdc, GetStockObject(NULL_PEN));
    
    SolidBrush b(Color(255, GetRValue(color), GetGValue(color), GetBValue(color)));
    // 타일의 좌상단 좌표 (x,y)
    x = posX * GRID_SIZE;
    y = posY * GRID_SIZE;
    //Rectangle(hdc, x, y, x + GRID_SIZE, y + GRID_SIZE);
    g.FillRectangle(&b, x, y, GRID_SIZE, GRID_SIZE);
    if (zoomFactor > 5 && node != nullptr)
    {
        SolidBrush brush(Color(255, 10, 10, 10));

        // 2. 폰트 설정
        FontFamily fontFamily(L"Segoe UI");
        Font font(&fontFamily, 13, FontStyleRegular, UnitPixel);  

        // 3. 출력 위치
        wchar_t buffer[50];
        PointF point(x, y);

        swprintf(buffer, 50, L"(%d, %d)", node->_x, node->_y);
        g.DrawString(buffer, -1, &font, point, &brush);
        point.Y += 13;

        swprintf(buffer, 50, L"G : %lf", node->_g);
        g.DrawString(buffer, -1, &font, point, &brush);
        point.Y += 13;

        swprintf(buffer, 50, L"H : %lf", node->_h);
        g.DrawString(buffer, -1, &font, point, &brush);
        point.Y += 13;

        swprintf(buffer, 50, L"F : %lf", node->_f);
        g.DrawString(buffer, -1, &font, point, &brush);
    }

    SelectObject(hdc, h_oldbrush);
}

void PaintNowStatus(HDC hdc, Graphics& g)
{
    auto now_pen_index = 0;
    if (g_BrushList.size() <= now_pen_index)
    {
        g_BrushList.emplace_back(CreateSolidBrush(RGB(rand() % 255, rand() % 255, rand() % 255)));
        g_BrushList.emplace_back(CreateSolidBrush(RGB(rand() % 255, rand() % 255, rand() % 255)));
        g_BrushList.emplace_back(CreateSolidBrush(RGB(rand() % 255, rand() % 255, rand() % 255)));
        g_BrushList.emplace_back(CreateSolidBrush(RGB(rand() % 255, rand() % 255, rand() % 255)));
        g_BrushList.emplace_back(CreateSolidBrush(RGB(rand() % 255, rand() % 255, rand() % 255)));
    }

    // 시작점
    PaintTile(hdc, g_pf._src.first, g_pf._src.second, g_hStartPointBrush,g);

    // 끝 점
    PaintTile(hdc, g_pf._dst.first, g_pf._dst.second, g_hEndPointBrush, g);

    // arrow pen
    Pen pen(Color(180, 255, 0, 0), 2.0f);
    AdjustableArrowCap* arrowCap = new AdjustableArrowCap(3.f, 3.f);
    pen.SetCustomEndCap(arrowCap);

    auto* now = g_pf._now;

    // closeList 렌더
    for (auto p : g_CloseListForRender)
    {
        PaintTile(hdc, p->_x, p->_y, g_hCloseListBrush,g, p);

        //UP
        if (p->_direction[0])
        {
            PointF start(p->_x * GRID_SIZE + GRID_SIZE / 2.0,
                p->_y * GRID_SIZE + GRID_SIZE / 2.0); // 방향

            PointF end(p->_x * GRID_SIZE + GRID_SIZE / 2.0,
                p->_y * GRID_SIZE);

            g.DrawLine(&pen, start, end);
        }
        //UP-RIGHT
        if (p->_direction[1])
        {
            PointF start(p->_x * GRID_SIZE + GRID_SIZE / 2.0,
                p->_y * GRID_SIZE + GRID_SIZE / 2.0); // 방향

            PointF end(p->_x * GRID_SIZE + GRID_SIZE,
                p->_y * GRID_SIZE);

            g.DrawLine(&pen, start, end);
        }
        //RIGHT
        if (p->_direction[2])
        {
            PointF start(p->_x * GRID_SIZE + GRID_SIZE / 2.0,
                p->_y * GRID_SIZE + GRID_SIZE / 2.0); // 방향

            PointF end(p->_x * GRID_SIZE + GRID_SIZE,
                p->_y * GRID_SIZE + GRID_SIZE / 2.0);

            g.DrawLine(&pen, start, end);
        }
        //DOWN-RIGHT
        if (p->_direction[3])
        {
            PointF start(p->_x * GRID_SIZE + GRID_SIZE / 2.0,
                p->_y * GRID_SIZE + GRID_SIZE / 2.0); // 방향

            PointF end(p->_x * GRID_SIZE + GRID_SIZE,
                p->_y * GRID_SIZE + GRID_SIZE);

            g.DrawLine(&pen, start, end);
        }
        //DOWN
        if (p->_direction[4])
        {
            PointF start(p->_x * GRID_SIZE + GRID_SIZE / 2.0,
                p->_y * GRID_SIZE + GRID_SIZE / 2.0); // 방향

            PointF end(p->_x * GRID_SIZE + GRID_SIZE / 2.0,
                p->_y * GRID_SIZE + GRID_SIZE);

            g.DrawLine(&pen, start, end);
        }
        //DOWN-LEFT
        if (p->_direction[5])
        {
            PointF start(p->_x * GRID_SIZE + GRID_SIZE / 2.0,
                p->_y * GRID_SIZE + GRID_SIZE / 2.0); // 방향

            PointF end(p->_x * GRID_SIZE,
                p->_y * GRID_SIZE + GRID_SIZE);

            g.DrawLine(&pen, start, end);
        }
        //LEFT
        if (p->_direction[6])
        {
            PointF start(p->_x * GRID_SIZE + GRID_SIZE / 2.0,
                p->_y * GRID_SIZE + GRID_SIZE / 2.0); // 방향

            PointF end(p->_x * GRID_SIZE,
                p->_y * GRID_SIZE + GRID_SIZE / 2.0);

            g.DrawLine(&pen, start, end);
        }
        //UP-LEFT
        if (p->_direction[7])
        {
            PointF start(p->_x * GRID_SIZE + GRID_SIZE / 2.0,
                p->_y * GRID_SIZE + GRID_SIZE / 2.0); // 방향

            PointF end(p->_x * GRID_SIZE,
                p->_y * GRID_SIZE);

            g.DrawLine(&pen, start, end);
        }

        if (g_BrushList.size() <= now_pen_index)
        {
            g_BrushList.emplace_back(CreateSolidBrush(RGB(rand() % 255, rand() % 255, rand() % 255)));
            g_BrushList.emplace_back(CreateSolidBrush(RGB(rand() % 255, rand() % 255, rand() % 255)));
            g_BrushList.emplace_back(CreateSolidBrush(RGB(rand() % 255, rand() % 255, rand() % 255)));
            g_BrushList.emplace_back(CreateSolidBrush(RGB(rand() % 255, rand() % 255, rand() % 255)));
            g_BrushList.emplace_back(CreateSolidBrush(RGB(rand() % 255, rand() % 255, rand() % 255)));
        }
        auto pen_for_once = g_BrushList[now_pen_index];
        for (auto& i : p->_searched)
        {
            PaintTile(hdc, i.first, i.second, pen_for_once, g);
        }
        now_pen_index++;
    }

    // 현재 노드 렌더
    if (now != nullptr)
    {
        PaintTile(hdc, now->_x, now->_y, g_hNowPathBrush, g, now);

        for (auto& i : now->_searched)
        {
            PaintTile(hdc, i.first, i.second, g_hNowSearchedBrush, g);
        }
        now_pen_index++;
        //UP
        if (now->_direction[0])
        {
            PointF start(now->_x * GRID_SIZE + GRID_SIZE / 2.0,
                now->_y * GRID_SIZE + GRID_SIZE / 2.0); // 방향

            PointF end(now->_x * GRID_SIZE + GRID_SIZE / 2.0,
                now->_y * GRID_SIZE);

            g.DrawLine(&pen, start, end);
        }
        //UP-RIGHT
        if (now->_direction[1])
        {
            PointF start(now->_x * GRID_SIZE + GRID_SIZE / 2.0,
                now->_y * GRID_SIZE + GRID_SIZE / 2.0); // 방향

            PointF end(now->_x * GRID_SIZE + GRID_SIZE,
                now->_y * GRID_SIZE);

            g.DrawLine(&pen, start, end);
        }
        //RIGHT
        if (now->_direction[2])
        {
            PointF start(now->_x * GRID_SIZE + GRID_SIZE / 2.0,
                now->_y * GRID_SIZE + GRID_SIZE / 2.0); // 방향

            PointF end(now->_x * GRID_SIZE + GRID_SIZE,
                now->_y * GRID_SIZE + GRID_SIZE / 2.0);

            g.DrawLine(&pen, start, end);
        }
        //DOWN-RIGHT
        if (now->_direction[3])
        {
            PointF start(now->_x * GRID_SIZE + GRID_SIZE / 2.0,
                now->_y * GRID_SIZE + GRID_SIZE / 2.0); // 방향

            PointF end(now->_x * GRID_SIZE + GRID_SIZE,
                now->_y * GRID_SIZE + GRID_SIZE);

            g.DrawLine(&pen, start, end);
        }
        //DOWN
        if (now->_direction[4])
        {
            PointF start(now->_x * GRID_SIZE + GRID_SIZE / 2.0,
                now->_y * GRID_SIZE + GRID_SIZE / 2.0); // 방향

            PointF end(now->_x * GRID_SIZE + GRID_SIZE / 2.0,
                now->_y * GRID_SIZE + GRID_SIZE);

            g.DrawLine(&pen, start, end);
        }
        //DOWN-LEFT
        if (now->_direction[5])
        {
            PointF start(now->_x * GRID_SIZE + GRID_SIZE / 2.0,
                now->_y * GRID_SIZE + GRID_SIZE / 2.0); // 방향

            PointF end(now->_x * GRID_SIZE,
                now->_y * GRID_SIZE + GRID_SIZE);

            g.DrawLine(&pen, start, end);
        }
        //LEFT
        if (now->_direction[6])
        {
            PointF start(now->_x * GRID_SIZE + GRID_SIZE / 2.0,
                now->_y * GRID_SIZE + GRID_SIZE / 2.0); // 방향

            PointF end(now->_x * GRID_SIZE,
                now->_y * GRID_SIZE + GRID_SIZE / 2.0);

            g.DrawLine(&pen, start, end);
        }
        //UP-LEFT
        if (now->_direction[7])
        {
            PointF start(now->_x * GRID_SIZE + GRID_SIZE / 2.0,
                now->_y * GRID_SIZE + GRID_SIZE / 2.0); // 방향

            PointF end(now->_x * GRID_SIZE,
                now->_y * GRID_SIZE);

            g.DrawLine(&pen, start, end);
        }
    }

    // openlist 렌더
    int ordering = 1;
    for (auto p : g_pf._openList)
    {
        PaintTile(hdc, p.second->_x, p.second->_y, g_hOpenListBrush, g, p.second);

        //UP
        if (p.second->_direction[0])
        {
            PointF start(p.second->_x * GRID_SIZE + GRID_SIZE / 2.0,
                p.second->_y * GRID_SIZE + GRID_SIZE / 2.0); // 방향

            PointF end(p.second->_x * GRID_SIZE + GRID_SIZE / 2.0,
                p.second->_y * GRID_SIZE);

            g.DrawLine(&pen, start, end);
        }
        //UP-RIGHT
        if (p.second->_direction[1])
        {
            PointF start(p.second->_x * GRID_SIZE + GRID_SIZE / 2.0,
                p.second->_y * GRID_SIZE + GRID_SIZE / 2.0); // 방향

            PointF end(p.second->_x * GRID_SIZE + GRID_SIZE,
                p.second->_y * GRID_SIZE);

            g.DrawLine(&pen, start, end);
        }
        //RIGHT
        if (p.second->_direction[2])
        {
            PointF start(p.second->_x * GRID_SIZE + GRID_SIZE / 2.0,
                p.second->_y * GRID_SIZE + GRID_SIZE / 2.0); // 방향

            PointF end(p.second->_x * GRID_SIZE + GRID_SIZE,
                p.second->_y * GRID_SIZE + GRID_SIZE / 2.0);

            g.DrawLine(&pen, start, end);
        }
        //DOWN-RIGHT
        if (p.second->_direction[3])
        {
            PointF start(p.second->_x * GRID_SIZE + GRID_SIZE / 2.0,
                p.second->_y * GRID_SIZE + GRID_SIZE / 2.0); // 방향

            PointF end(p.second->_x * GRID_SIZE + GRID_SIZE,
                p.second->_y * GRID_SIZE + GRID_SIZE);

            g.DrawLine(&pen, start, end);
        }
        //DOWN
        if (p.second->_direction[4])
        {
            PointF start(p.second->_x * GRID_SIZE + GRID_SIZE / 2.0,
                p.second->_y * GRID_SIZE + GRID_SIZE / 2.0); // 방향

            PointF end(p.second->_x * GRID_SIZE + GRID_SIZE / 2.0,
                p.second->_y * GRID_SIZE + GRID_SIZE);

            g.DrawLine(&pen, start, end);
        }
        //DOWN-LEFT
        if (p.second->_direction[5])
        {
            PointF start(p.second->_x * GRID_SIZE + GRID_SIZE / 2.0,
                p.second->_y * GRID_SIZE + GRID_SIZE / 2.0); // 방향

            PointF end(p.second->_x * GRID_SIZE,
                p.second->_y * GRID_SIZE + GRID_SIZE);

            g.DrawLine(&pen, start, end);
        }
        //LEFT
        if (p.second->_direction[6])
        {
            PointF start(p.second->_x * GRID_SIZE + GRID_SIZE / 2.0,
                p.second->_y * GRID_SIZE + GRID_SIZE / 2.0); // 방향

            PointF end(p.second->_x * GRID_SIZE,
                p.second->_y * GRID_SIZE + GRID_SIZE / 2.0);

            g.DrawLine(&pen, start, end);
        }
        //UP-LEFT
        if (p.second->_direction[7])
        {
            PointF start(p.second->_x * GRID_SIZE + GRID_SIZE / 2.0,
                p.second->_y * GRID_SIZE + GRID_SIZE / 2.0); // 방향

            PointF end(p.second->_x * GRID_SIZE,
                p.second->_y * GRID_SIZE);

            g.DrawLine(&pen, start, end);
        }
        //zoomFactor <= 5
        SolidBrush brush(Color(255, 1, 1, 1));

        // 2. 폰트 설정
        FontFamily fontFamily(L"Segoe UI");
        Font font(&fontFamily, 11, FontStyleBoldItalic, UnitPixel);

        // 3. 출력 위치
        wchar_t buffer[10];
        PointF point((p.second->_x + 0.4)* GRID_SIZE, p.second->_y* GRID_SIZE);

        swprintf(buffer, 50, L"%d", ordering);
        g.DrawString(buffer, -1, &font, point, &brush);
        ordering++;
    }

    // 출력용 노드 순번
    // 순서에 맞춰 땅을 덮게 의도
    if(g_pf._now != nullptr)
        g_CloseListForRender.push_back(g_pf._now);

    // 목적지 찾으면 링킹
    while (now != nullptr && now->_parent != nullptr && g_pf.FoundDst())
    {
        //PaintTile(hdc, now->_x, now->_y, g_hNowPathBrush, g, now);
        Pen pen(Color(180, 38, 36, 38), 2.0f);
        PointF start(now->_x * GRID_SIZE + GRID_SIZE / 2.0,
            now->_y* GRID_SIZE + GRID_SIZE / 2.0); // 방향

        PointF end(now->_parent->_x * GRID_SIZE + GRID_SIZE / 2.0,
            now->_parent->_y * GRID_SIZE + GRID_SIZE / 2.0);

        g.DrawLine(&pen, start, end);
        now = now->_parent;
    }

    /////////////////////////////////////////////////////////
    // 상태창
    /////////////////////////////////////////////////////////
    
    // 출력 좌표 고정을 위해
    g.ResetTransform();

    SolidBrush brush(Color(255, 10, 10, 10));

    // 2. 폰트 설정
    FontFamily fontFamily(L"Segoe UI");
    Font font(&fontFamily, 13, FontStyleRegular, UnitPixel);  

    // 3. 출력 위치
    wchar_t buffer[100];
    PointF point(800, 800);

    if (g_bIsProcessing)
    {
        swprintf(buffer, 50, L"실행 모드입니다.");
        g.DrawString(buffer, -1, &font, point, &brush);
        point.Y += 15;

        swprintf(buffer, 50, L"[NEXT] : 다음 한칸을 찾습니다.");
        g.DrawString(buffer, -1, &font, point, &brush);
        point.Y += 15;

        swprintf(buffer, 50, L"[FIND_DST] : 목적지까지 길을 탐색합니다.");
        g.DrawString(buffer, -1, &font, point, &brush);
        point.Y += 15;

        swprintf(buffer, 50, L"[Stop] : FIND_DST 동작을 멈춥니다.");
        g.DrawString(buffer, -1, &font, point, &brush);
        point.Y += 15;

        if(g_pf.Unreachable())
            swprintf(buffer, 50, L"[경과] : 목적지 도달 불가.");
        else if(g_pf.FoundDst())
            swprintf(buffer, 50, L"[경과] : 목적지 도착 ");
        else
            swprintf(buffer, 50, L"[경과] : 탐색중 ");
        g.DrawString(buffer, -1, &font, point, &brush);
    }
    else
    {
        swprintf(buffer, 50, L"편집 모드입니다.");
        g.DrawString(buffer, -1, &font, point, &brush);
        point.Y += 15;

        swprintf(buffer, 100, L"패닝 기능으로 인해 Edit 위치가 이상해졌다면, Edit Mode 버튼을 다시 눌러주세요.");
        g.DrawString(buffer, -1, &font, point, &brush);
        point.Y += 15;

        if (g_pf._mode_g == PathFinder::Euclid)
            swprintf(buffer, 50, L"[G] : EUCLID");
        else
            swprintf(buffer, 50, L"[G] : MANHATTAN ");
        g.DrawString(buffer, -1, &font, point, &brush);
        point.Y += 15;

        if (g_pf._mode_h == PathFinder::Euclid)
            swprintf(buffer, 50, L"[H] : EUCLID");
        else
            swprintf(buffer, 50, L"[H] : MANHATTAN ");
        g.DrawString(buffer, -1, &font, point, &brush);
    }

    delete arrowCap;
}

void CreateButtons(HWND hWnd)
{
    // BTN
    CreateWindowW(L"BUTTON", L"EDIT_WALL",
        WS_VISIBLE | WS_CHILD,
        50, 810, 150, 30,      // 위치와 크기
        hWnd, (HMENU)SET_WALL,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);
    CreateWindowW(L"BUTTON", L"EDIT_STARTPOINT",
        WS_VISIBLE | WS_CHILD,
        50, 840, 150, 30,      // 위치와 크기
        hWnd, (HMENU)SET_STARTPOINT,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);
    CreateWindowW(L"BUTTON", L"EDIT_ENDPOINT",
        WS_VISIBLE | WS_CHILD,
        50, 870, 150, 30,      // 위치와 크기
        hWnd, (HMENU)SET_ENDPOINT,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);
    CreateWindowW(L"BUTTON", L"EDIT_RandWall",
        WS_VISIBLE | WS_CHILD,
        50, 900, 150, 30,      // 위치와 크기
        hWnd, (HMENU)RAND_WALL,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);
    CreateWindowW(L"BUTTON", L"G_EUCLID",
        WS_VISIBLE | WS_CHILD,
        200, 810, 150, 30,      // 위치와 크기
        hWnd, (HMENU)G_EUCLID,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);
    CreateWindowW(L"BUTTON", L"G_MANHATTAN",
        WS_VISIBLE | WS_CHILD,
        200, 840, 150, 30,      // 위치와 크기
        hWnd, (HMENU)G_MANHATTAN,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);
    CreateWindowW(L"BUTTON", L"EraseWalls",
        WS_VISIBLE | WS_CHILD,
        200, 870, 150, 30,      // 위치와 크기
        hWnd, (HMENU)ERASE_WALLS,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);
    CreateWindowW(L"BUTTON", L"H_EUCLID",
        WS_VISIBLE | WS_CHILD,
        350, 810, 150, 30,      // 위치와 크기
        hWnd, (HMENU)H_EUCLID,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);
    CreateWindowW(L"BUTTON", L"H_MANHATTAN",
        WS_VISIBLE | WS_CHILD,
        350, 840, 150, 30,      // 위치와 크기
        hWnd, (HMENU)H_MANHATTAN,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);
    CreateWindowW(L"BUTTON", L"EXECUTE Mode",
        WS_VISIBLE | WS_CHILD,
        500, 810, 150, 30,      // 위치와 크기
        hWnd, (HMENU)EXECUTE,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);
    CreateWindowW(L"BUTTON", L"Edit Mode",
        WS_VISIBLE | WS_CHILD,
        500, 840, 150, 30,      // 위치와 크기
        hWnd, (HMENU)STOP,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);
    CreateWindowW(L"BUTTON", L"NEXT (key n)",
        WS_VISIBLE | WS_CHILD,
        650, 810, 150, 30,      // 위치와 크기
        hWnd, (HMENU)NEXT,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);
    CreateWindowW(L"BUTTON", L"FIND_DST",
        WS_VISIBLE | WS_CHILD,
        650, 840, 150, 30,      // 위치와 크기
        hWnd, (HMENU)FIND_DST,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);
    CreateWindowW(L"BUTTON", L"Stop",
        WS_VISIBLE | WS_CHILD,
        650, 870, 150, 30,      // 위치와 크기
        hWnd, (HMENU)STOP_FIND_DST,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);
    CreateWindowW(L"BUTTON", L"SIMULATION",
        WS_VISIBLE | WS_CHILD,
        650, 900, 150, 30,      // 위치와 크기
        hWnd, (HMENU)SIMULATION,
        (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
        NULL);
}

void InitWalls()
{
    for (int w = 0;w < GRID_WIDTH;w++)
    {
        for (int h = 0;h < GRID_HEIGHT;h++)
        {
            if (g_Tile[h][w])
            {
                // 타일의 좌상단 좌표 (x,y)
                g_pf.AddWall(w, h);
            }
        }
    }
}
void MakeCave()
{
    CellularAutomata cavemaker(g_pf._src.first, g_pf._src.second, g_pf._dst.first, g_pf._dst.second);
    cavemaker.InitMap();
    cavemaker.MakeNoise(57);
    cavemaker.GenerateMap();
    cavemaker.GenerateMap();
    
    memcpy(g_Tile, g_bMap, sizeof(g_Tile));
}


// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{

    // TODO: 여기에 코드를 입력합니다.
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&g_GdiplusToken, &gdiplusStartupInput, nullptr);

    // 전역 문자열을 초기화합니다.
    MyRegisterClass(hInstance);

    // 애플리케이션 초기화를 수행합니다:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }


    MSG msg;

    // 기본 메시지 루프입니다:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    GdiplusShutdown(g_GdiplusToken);

    return (int) msg.wParam;
}



//
//  함수: MyRegisterClass()
//
//  용도: 창 클래스를 등록합니다.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TILEMAP));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_TILEMAP);
    wcex.lpszClassName  = L"PathFinder";
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   함수: InitInstance(HINSTANCE, int)
//
//   용도: 인스턴스 핸들을 저장하고 주 창을 만듭니다.
//
//   주석:
//
//        이 함수를 통해 인스턴스 핸들을 전역 변수에 저장하고
//        주 프로그램 창을 만든 다음 표시합니다.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

   HWND hWnd = CreateWindowW(L"PathFinder", L"PathFind", WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, 1650, 1000, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  함수: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  용도: 주 창의 메시지를 처리합니다.
//
//  WM_COMMAND  - 애플리케이션 메뉴를 처리합니다.
//  WM_PAINT    - 주 창을 그립니다.
//  WM_DESTROY  - 종료 메시지를 게시하고 반환합니다.
//
//

// 클라이언트 크기의 BITMAP
HBITMAP g_hMemDCBitmap;
HBITMAP g_hMemDCBitmap_old;

// 메모리 접근 DC (실제 도화지)
HDC g_hMemDC;

// Memory상의 도화지 크기를 정의한다. 클라이언트 크기에 따라 갱신된다.
RECT g_MemDCRect;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_RBUTTONDOWN:
    {
        isDragging = true;
        dragStart.x = GET_X_LPARAM(lParam);
        dragStart.y = GET_Y_LPARAM(lParam);
        SetCapture(hWnd);
        break;
    }
    case WM_RBUTTONUP:
    {
        isDragging = false;
        ReleaseCapture();
        break;
    }
    case WM_LBUTTONDOWN:
        g_bDrag = true;
        
        if(g_bIsProcessing == false)
        {
            int mouse_x = GET_X_LPARAM(lParam);
            int mouse_y = GET_Y_LPARAM(lParam);
            int tile_x = mouse_x / GRID_SIZE;
            int tile_y = mouse_y / GRID_SIZE;

            if (g_bStartPointEdit)
            {
                if (tile_x >= 0 && tile_x < GRID_WIDTH &&
                    tile_y >= 0 && tile_y < GRID_HEIGHT)
                {
                    g_pf._src.first = tile_x;
                    g_pf._src.second = tile_y;
                    InvalidateRect(hWnd, NULL, false);
                }
            }
            else if (g_bEndPointEdit)
            {
                if (tile_x >= 0 && tile_x < GRID_WIDTH &&
                    tile_y >= 0 && tile_y < GRID_HEIGHT)
                {
                    g_pf._dst.first = tile_x;
                    g_pf._dst.second = tile_y;
                    InvalidateRect(hWnd, NULL, false);
                }
            }
            else
            {
                // 벽
                int mouse_x = GET_X_LPARAM(lParam);
                int mouse_y = GET_Y_LPARAM(lParam);
                int tile_x = mouse_x / GRID_SIZE;
                int tile_y = mouse_y / GRID_SIZE;

                if (g_Tile[tile_y][tile_x] == 1)
                    g_bErase = true;
                else
                    g_bErase = false;
            }
        }
        break;
    case WM_LBUTTONUP:
        g_bDrag = false;

        break;
    case WM_MOUSEMOVE:
        // 화면 조절
        if (isDragging) {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);

            // 드래그 방향에 따라 전체 오프셋 조절
            viewOffsetX += (x - dragStart.x);
            viewOffsetY += (y - dragStart.y);

            dragStart.x = x;
            dragStart.y = y;

            InvalidateRect(hWnd, NULL, false);
        }
        // Edit Mode에서 장애물 설치 및 제거
        if (g_bDrag && !g_bIsProcessing && !g_bStartPointEdit && !g_bEndPointEdit)
        {
            int mouse_x = GET_X_LPARAM(lParam);
            int mouse_y = GET_Y_LPARAM(lParam);
            int tile_x = mouse_x / GRID_SIZE;
            int tile_y = mouse_y / GRID_SIZE;

            if (tile_x >= 0 && tile_x < GRID_WIDTH &&
                tile_y >= 0 && tile_y < GRID_HEIGHT)
            {
                g_Tile[tile_y][tile_x] = !g_bErase;
            }
            InvalidateRect(hWnd, NULL, false);
        }
        break;
    case WM_KEYDOWN:
        if ((wParam == 'N'))
        {
            if (g_bIsProcessing)
            {
                g_pf.Next();
                InvalidateRect(hWnd, NULL, false);
            }
        }
        break;
    case WM_CREATE:
    {
        srand(RAND_SEED);
        g_hGridPen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
        g_hTileBrush = CreateSolidBrush(RGB(100, 100, 100));

        // GREEN
        g_hOpenListBrush = CreateSolidBrush(RGB(38, 209, 75));

        // PURPLE
        g_hCloseListBrush = CreateSolidBrush(RGB(187, 132, 245));

        // YELLOW
        g_hNowPathBrush = CreateSolidBrush(RGB(251, 255, 0));

        // R
        g_hStartPointBrush = CreateSolidBrush(RGB(255, 0, 0));
        // B
        g_hEndPointBrush = CreateSolidBrush(RGB(0, 0, 255));

        g_hNowSearchedBrush = CreateSolidBrush(RGB(38, 36, 38));
        HDC hdc = GetDC(hWnd);

        // 클라이언트 영역
        GetClientRect(hWnd, &g_MemDCRect);

        // 메모리에 덮어쓸 비트맵 DC (클라이언트 영역을 바탕으로 크기 결정)
        g_hMemDCBitmap = CreateCompatibleBitmap(hdc, g_MemDCRect.right, g_MemDCRect.bottom);

        // 메모리 DC
        g_hMemDC = CreateCompatibleDC(hdc);
        ReleaseDC(hWnd, hdc);
        g_hMemDCBitmap_old = (HBITMAP)SelectObject(g_hMemDC, g_hMemDCBitmap);

        // BTN
        CreateButtons(hWnd);

        // default start, end point
        g_StartPoint.first = 1;
        g_StartPoint.second = 1;
        g_EndPoint.first = 80;
        g_EndPoint.second = 49;
        //g_EndPoint.first = GRID_WIDTH - 1;
        //g_EndPoint.second = GRID_HEIGHT - 1;
        // init
        g_pf.Init(g_StartPoint, g_EndPoint);
    }
    break;
    case WM_COMMAND:
    {            
        auto btn_val = LOWORD(wParam);
        switch (btn_val)
        {
        case SET_WALL:
            if (HIWORD(wParam) == BN_CLICKED &&
                g_bIsProcessing == false) {
                //TODO
                g_bStartPointEdit = false;
                g_bEndPointEdit = false;
            }
            break;
        case SET_STARTPOINT:
            if (HIWORD(wParam) == BN_CLICKED &&
                g_bIsProcessing == false) {
                //TODO
                g_bStartPointEdit = true;
                g_bEndPointEdit = false;
            }
            break;
        case SET_ENDPOINT:
            if (HIWORD(wParam) == BN_CLICKED &&
                g_bIsProcessing == false) {
                //TODO
                g_bStartPointEdit = false;
                g_bEndPointEdit = true;
            }
            break;
        case G_EUCLID:
            if (HIWORD(wParam) == BN_CLICKED &&
                g_bIsProcessing == false) {
                g_pf._mode_g = PathFinder::Euclid;
                InvalidateRect(hWnd, NULL, false);
            }
            break;
        case G_MANHATTAN:
            if (HIWORD(wParam) == BN_CLICKED &&
                g_bIsProcessing == false) {
                g_pf._mode_g = PathFinder::Manhattan;
                InvalidateRect(hWnd, NULL, false);
            }
            break;
        case H_EUCLID:
            if (HIWORD(wParam) == BN_CLICKED &&
                g_bIsProcessing == false) {
                g_pf._mode_h = PathFinder::Euclid;
                InvalidateRect(hWnd, NULL, false);
            }           
            break;
        case H_MANHATTAN:
            if (HIWORD(wParam) == BN_CLICKED &&
                g_bIsProcessing == false) {
                g_pf._mode_h = PathFinder::Manhattan;
                InvalidateRect(hWnd, NULL, false);
            }
            break;
        case EXECUTE:
            if (HIWORD(wParam) == BN_CLICKED) {
                g_bIsProcessing = true;                
                g_pf.Init();
                g_pf.ClearWall();
                InitWalls();
                g_CloseListForRender.clear();
                InvalidateRect(hWnd, NULL, false);

                KillTimer(hWnd, TIMER_FIND_DST);
                KillTimer(hWnd, TIMER_SIMULATION);
            }         
            break;
            // EDIT MODE
        case STOP:
            if (HIWORD(wParam) == BN_CLICKED) {
                g_bIsProcessing = false;
                g_pf.Init();
            
                KillTimer(hWnd, TIMER_SIMULATION);
                KillTimer(hWnd, TIMER_FIND_DST);
                // 줌 초기화
                zoomFactor = 1.0;
                // 패닝 초기화
                dragStart = { 0, 0 };
                viewOffsetX = 0, viewOffsetY = 0;

                g_CloseListForRender.clear();
                InvalidateRect(hWnd, NULL, false);
            }
            break;
        case NEXT:
            if (HIWORD(wParam) == BN_CLICKED && 
                g_bIsProcessing == true) {
                g_pf.Next();  // 버튼 클릭 시 func1 호출
                InvalidateRect(hWnd, NULL, false);
            }
            break;
        case FIND_DST:
            if (HIWORD(wParam) == BN_CLICKED &&
                g_bIsProcessing == true) {
                SetTimer(hWnd, TIMER_FIND_DST, 1, NULL);
            }
            break;
        case STOP_FIND_DST:
            if (HIWORD(wParam) == BN_CLICKED &&
                g_bIsProcessing == true) {
                KillTimer(hWnd, TIMER_FIND_DST);
                KillTimer(hWnd, TIMER_SIMULATION);
                g_bIsSimulating = false;
            }
            break;
        case RAND_WALL:
            if (HIWORD(wParam) == BN_CLICKED &&
                g_bIsProcessing == false) {
                //TODO
                MakeCave();
                InvalidateRect(hWnd, NULL, false);
            }
            break;
        case SIMULATION:
            if (HIWORD(wParam) == BN_CLICKED &&
                g_bIsProcessing == true) {
                g_bIsSimulating = false;
                SetTimer(hWnd, TIMER_SIMULATION, 1, NULL);
            }
            break;
        case ERASE_WALLS:
            if (HIWORD(wParam) == BN_CLICKED &&
                g_bIsProcessing == false) {

                memset(g_Tile, 0, sizeof(g_Tile));
                InvalidateRect(hWnd, NULL, false);
            }
            break;
        }
    }
        break;
    case WM_TIMER:
        if (wParam == TIMER_FIND_DST)
        {
            bool b = g_pf.Next();
            InvalidateRect(hWnd, NULL, false);
            if (b == false)
                KillTimer(hWnd, TIMER_FIND_DST);
        }
        else if (wParam == TIMER_SIMULATION)
        {
            if (g_bIsSimulating == false)
            {
                g_bIsSimulating = true;
                MakeCave();
                g_pf.Init();
                g_pf.ClearWall();
                InitWalls();
                g_CloseListForRender.clear();
                InvalidateRect(hWnd, NULL, false);
            }
            else
            {
                bool b = g_pf.Next();
                InvalidateRect(hWnd, NULL, false);
                if (b == false)
                {
                    g_bIsSimulating = false;
                    if (g_pf.Unreachable())
                    {
                        KillTimer(hWnd, TIMER_SIMULATION);
                        g_bIsSimulating = false;
                    }
                }
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // 메모리에 
            Graphics g(g_hMemDC);
            g.SetSmoothingMode(SmoothingModeAntiAlias);
            g.TranslateTransform((REAL)viewOffsetX, (REAL)viewOffsetY);

            PatBlt(g_hMemDC, 0, 0, g_MemDCRect.right, g_MemDCRect.bottom, WHITENESS);
            RenderObstacle(g_hMemDC, g);
            RenderGrid(g_hMemDC,g);

            PaintNowStatus(g_hMemDC, g);
            BitBlt(hdc, 0, 0, g_MemDCRect.right, g_MemDCRect.bottom, g_hMemDC, 0, 0, SRCCOPY);
            // TODO: 여기에 hdc를 사용하는 그리기 코드를 추가합니다...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_MOUSEWHEEL:
    {
        short delta = GET_WHEEL_DELTA_WPARAM(wParam);
        if (delta > 0)
            zoomFactor *= 1.1;
        else
            zoomFactor /= 1.1;
        InvalidateRect(hWnd, NULL, false);
    }
    break;
    case WM_SIZE:
    {
        SelectObject(g_hMemDC, g_hMemDCBitmap_old);
        DeleteObject(g_hMemDC);
        DeleteObject(g_hMemDCBitmap);

        HDC hdc = GetDC(hWnd);

        // 클라이언트 영역
        GetClientRect(hWnd, &g_MemDCRect);

        // 메모리에 덮어쓸 비트맵 DC (클라이언트 영역을 바탕으로 크기 결정)
        g_hMemDCBitmap = CreateCompatibleBitmap(hdc, g_MemDCRect.right, g_MemDCRect.bottom);

        // 메모리 DC
        g_hMemDC = CreateCompatibleDC(hdc);
        ReleaseDC(hWnd, hdc);
        g_hMemDCBitmap_old = (HBITMAP)SelectObject(g_hMemDC, g_hMemDCBitmap);
        break;
    }
    case WM_DESTROY:
    {
        SelectObject(g_hMemDC, g_hMemDCBitmap_old);
        DeleteObject(g_hMemDC);
        DeleteObject(g_hMemDCBitmap);

        DeleteObject(g_hTileBrush);
        DeleteObject(g_hGridPen);
        DeleteObject(g_hOpenListBrush);
        DeleteObject(g_hCloseListBrush);
        DeleteObject(g_hNowPathBrush);
        DeleteObject(g_hStartPointBrush);
        DeleteObject(g_hEndPointBrush);
        DeleteObject(g_hNowSearchedBrush);

        for (auto i : g_BrushList)
        {
            DeleteObject(i);
        }

        KillTimer(hWnd, TIMER_FIND_DST);

        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// 정보 대화 상자의 메시지 처리기입니다.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
