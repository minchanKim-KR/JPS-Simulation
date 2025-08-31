#pragma once

#define CA_WIDTH 100
#define CA_HEIGHT 50

#include <Windows.h>
#include <time.h>
#include <iostream>

using namespace std;

extern BYTE g_bMap[CA_HEIGHT][CA_WIDTH];
extern BYTE g_bMapTemp[CA_HEIGHT][CA_WIDTH];

class CellularAutomata
{
public:
	CellularAutomata(int srcX, int srcY, int dstX, int dstY) :
		_srcX(srcX), _srcY(srcY), _dstX(dstX), _dstY(dstY)
	{
		//srand(time(NULL));
	}

	// 1 ~ 100
	void MakeNoise(int density)
	{
		for (int i = 0;i < CA_HEIGHT;i++)
		{
			for (int j = 0;j < CA_WIDTH;j++)
			{
				if (rand() % 101 < density)
					g_bMap[i][j] = 1;
			}
		}

		// 출발지 보정
		for (int y = _srcY - 2;y <= _srcY + 2;y++)
		{
			for (int x = _srcX - 2;x <= _srcX + 2;x++)
			{
				if (y >= 0 && y < CA_HEIGHT &&
					x >= 0 && x < CA_WIDTH)
				{
					g_bMap[y][x] = 0;
				}
			}
		}
		// 목적지 보정
		for (int y = _dstY - 2;y <= _dstY + 2;y++)
		{
			for (int x = _dstX - 2;x <= _dstX + 2;x++)
			{
				if (y >= 0 && y < CA_HEIGHT &&
					x >= 0 && x < CA_WIDTH)
				{
					g_bMap[y][x] = 0;
				}
			}
		}
	}
	void InitMap()
	{
		memset(g_bMap, 0, sizeof(g_bMap));
	}
	void GenerateMap()
	{
		for (int y = 0;y < CA_HEIGHT;y++)
		{
			for (int x = 0;x < CA_WIDTH;x++)
			{
				g_bMapTemp[y][x] = MakeTileFromMap(x, y);
			}
		}
		for (int y = 0;y < CA_HEIGHT;y++)
		{
			for (int x = 0;x < CA_WIDTH;x++)
			{
				g_bMap[y][x] = MakeTileFromTemp(x, y);
			}
		}

		for (int i = 0;i < CA_WIDTH;i++)
			g_bMap[_srcY][i] = 0;
		for (int i = 0;i < CA_HEIGHT;i++)
			g_bMap[i][_dstX] = 0;
		//int startX;
		//int startY;
		//if()
		//for()
	}
	BYTE MakeTileFromMap(int x, int y)
	{
		int walls = 0;

		// UP-LEFT
		if (x - 1 >= 0 && x - 1 < CA_WIDTH &&
			y - 1 >= 0 && y - 1 < CA_HEIGHT)
		{
			if (g_bMap[y - 1][x - 1] == 1)
				walls++;
		}
		else
			walls++;
		// UP
		if (x >= 0 && x < CA_WIDTH &&
			y - 1 >= 0 && y - 1 < CA_HEIGHT)
		{
			if (g_bMap[y - 1][x] == 1)
				walls++;
		}
		else
			walls++;
		// UP-RIGHT
		if (x + 1 >= 0 && x + 1 < CA_WIDTH &&
			y - 1 >= 0 && y - 1 < CA_HEIGHT)
		{
			if (g_bMap[y - 1][x + 1] == 1)
				walls++;
		}
		else
			walls++;
		// RIGHT
		if (x + 1 >= 0 && x + 1 < CA_WIDTH &&
			y >= 0 && y < CA_HEIGHT)
		{
			if (g_bMap[y][x + 1] == 1)
				walls++;
		}
		else
			walls++;
		// DOWN-RIGHT
		if (x + 1 >= 0 && x + 1 < CA_WIDTH &&
			y + 1 >= 0 && y + 1 < CA_HEIGHT)
		{
			if (g_bMap[y + 1][x + 1] == 1)
				walls++;
		}
		else
			walls++;
		// DOWN
		if (x >= 0 && x < CA_WIDTH &&
			y + 1 >= 0 && y + 1 < CA_HEIGHT)
		{
			if (g_bMap[y + 1][x] == 1)
				walls++;
		}
		else
			walls++;
		// DOWN-LEFT
		if (x - 1 >= 0 && x - 1 < CA_WIDTH &&
			y + 1 >= 0 && y + 1 < CA_HEIGHT)
		{
			if (g_bMap[y + 1][x - 1] == 1)
				walls++;
		}
		else
			walls++;
		// LEFT
		if (x - 1 >= 0 && x - 1 < CA_WIDTH &&
			y >= 0 && y < CA_HEIGHT)
		{
			if (g_bMap[y][x - 1] == 1)
				walls++;
		}
		else
			walls++;

		if (walls > _criterion)
			return 1;
		else
			return 0;
	}
	BYTE MakeTileFromTemp(int x, int y)
	{
		int walls = 0;

		// UP-LEFT
		if (x - 1 >= 0 && x - 1 < CA_WIDTH &&
			y - 1 >= 0 && y - 1 < CA_HEIGHT)
		{
			if (g_bMapTemp[y - 1][x - 1] == 1)
				walls++;
		}
		else
			walls++;
		// UP
		if (x >= 0 && x < CA_WIDTH &&
			y - 1 >= 0 && y - 1 < CA_HEIGHT)
		{
			if (g_bMapTemp[y - 1][x] == 1)
				walls++;
		}
		else
			walls++;
		// UP-RIGHT
		if (x + 1 >= 0 && x + 1 < CA_WIDTH &&
			y - 1 >= 0 && y - 1 < CA_HEIGHT)
		{
			if (g_bMapTemp[y - 1][x + 1] == 1)
				walls++;
		}
		else
			walls++;
		// RIGHT
		if (x + 1 >= 0 && x + 1 < CA_WIDTH &&
			y >= 0 && y < CA_HEIGHT)
		{
			if (g_bMapTemp[y][x + 1] == 1)
				walls++;
		}
		else
			walls++;
		// DOWN-RIGHT
		if (x + 1 >= 0 && x + 1 < CA_WIDTH &&
			y + 1 >= 0 && y + 1 < CA_HEIGHT)
		{
			if (g_bMapTemp[y + 1][x + 1] == 1)
				walls++;
		}
		else
			walls++;
		// DOWN
		if (x >= 0 && x < CA_WIDTH &&
			y + 1 >= 0 && y + 1 < CA_HEIGHT)
		{
			if (g_bMapTemp[y + 1][x] == 1)
				walls++;
		}
		else
			walls++;
		// DOWN-LEFT
		if (x - 1 >= 0 && x - 1 < CA_WIDTH &&
			y + 1 >= 0 && y + 1 < CA_HEIGHT)
		{
			if (g_bMapTemp[y + 1][x - 1] == 1)
				walls++;
		}
		else
			walls++;
		// LEFT
		if (x - 1 >= 0 && x - 1 < CA_WIDTH &&
			y >= 0 && y < CA_HEIGHT)
		{
			if (g_bMapTemp[y][x - 1] == 1)
				walls++;
		}
		else
			walls++;

		if (walls > _criterion)
			return 1;
		else
			return 0;
	}

	void PrintMap()
	{
		for (int i = 0;i < CA_HEIGHT;i++)
		{
			for (int j = 0;j < CA_WIDTH;j++)
			{
				if (g_bMap[i][j] == 1)
				{
					cout << "■";
				}
				else
				{
					cout << "□";
				}
			}
			cout << endl;
		}
	}
private:
	// 기준 초과면 벽
	BYTE _criterion = 4;
	int _srcX;
	int _srcY;
	int _dstX;
	int _dstY;
};