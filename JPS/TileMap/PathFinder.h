#pragma once
#include <map>
#include <list>
#include <math.h>

// temp
#define MAP_WIDTH 100
#define MAP_HEIGHT 50

using namespace std;

class PathFinder
{
public:
	// Distance 계산 모드
	enum DistanceMode
	{
		Euclid, Manhattan
	};
	enum Direction
	{
		UP = 0, UP_RIGHT, RIGHT, DOWN_RIGHT, DOWN, DOWN_LEFT, LEFT, UP_LEFT
	};
	struct Node
	{
		int _x;
		int _y;
		Node* _parent;
		bool _direction[8];
		double _g;
		double _h;
		double _f;
		list<pair<int, int>> _searched;
	};
	PathFinder(int mapSizeX, int mapSizeY) : 
		_mapSizeX(mapSizeX), 
		_mapSizeY(mapSizeY),
		_now(nullptr),
		_src(pair<int,int>{0,0}),
		_dst(pair<int, int>{0, 0}),
		_mode_g(Euclid),
		_mode_h(Manhattan),
		_foundDst(false),
		_is_unreachable(false)
	{

	}
	~PathFinder()
	{
		for (auto& n : _openList)
		{
			DeleteNode(n.second);
		}

		for (auto& n : _closeList)
		{
			DeleteNode(n.second);
		}

	}

	// 초기화
	void Init()
	{
		InitOpenList();
		InitCloseList();

		// now는 closelist에 존재함.
		_now = nullptr;

		// 벽 초기화는 수동.
		// map 초기화는 생성자에서만.

		// 모드 변경은 수동
		_foundDst = false;
		_is_unreachable = false;
	}
	void Init(pair<int, int> src, pair<int, int> dst)
	{
		InitOpenList();
		InitCloseList();

		// now는 closelist에 존재함.
		_now = nullptr;

		// 벽 초기화는 수동.
		// map 초기화는 생성자에서만.

		_src = src;
		_dst = dst;

		// 모드 변경은 수동
		_foundDst = false;	
		_is_unreachable = false;
	}
	void InitOpenList()
	{
		for (auto& n : _openList)
		{
			delete n.second;
		}
		_openList.clear();
	}
	void InitCloseList()
	{
		for (auto& n : _closeList)
		{
			delete n.second;
		}
		_closeList.clear();
	}

	// 수동
	void InitWallList()
	{
		for (auto& n : _wallList)
		{
			delete n.second;
		}
		_wallList.clear();
	}

	bool Next()
	{
		if (_now == nullptr)
		{
			bool direction[8] = { true, true, true, true, true, true, true, true };
			// push src
			Node* src_node = CreateNode(_src.first, _src.second, nullptr,direction);
			_openList.insert(pair<double, Node*>{src_node->_f, src_node});
		}
		if (FoundDst())
			return false;
		if (_openList.size() == 0)
		{
			_is_unreachable = true;
			return false;
		}

		// 1. 가장 낮은 key를 가진 iterator를 가져온다.
		auto now =  _openList.begin();

		// 2. 방문
		_closeList.insert(*now);
		_now = (*now).second;

		// 3. 해당 노드가 목적지면 return
		if ((*now).second->_x == _dst.first && (*now).second->_y == _dst.second)
		{
			// 중요, 꼭 openlist에서 제거하고 나와야함.
			_openList.erase(now);
			_foundDst = true;
			return true;
		}

		if(_now->_direction[0])
			CheckLine_UP(_now);
		if(_now->_direction[1])
			CheckAcross_UP_RIGHT(_now);
		if(_now->_direction[2])
			CheckLine_RIGHT(_now);
		if (_now->_direction[3])
			CheckAcross_DOWN_RIGHT(_now);
		if (_now->_direction[4])
			CheckLine_DOWN(_now);
		if (_now->_direction[5])
			CheckAcross_DOWN_LEFT(_now);
		if (_now->_direction[6])
			CheckLine_LEFT(_now);
		if (_now->_direction[7])
			CheckAcross_UP_LEFT(_now);
		/*// 4. 시계 방향으로 조사
		if (isValidPos(_now->_x, _now->_y - 1, _now))
			AddToOpenList(_now->_x, _now->_y - 1,_now);
		if (isValidPos(_now->_x + 1, _now->_y - 1, _now))
			AddToOpenList(_now->_x + 1, _now->_y - 1, _now);
		if (isValidPos(_now->_x + 1, _now->_y, _now))
			AddToOpenList(_now->_x + 1, _now->_y, _now);
		if (isValidPos(_now->_x + 1, _now->_y + 1, _now))
			AddToOpenList(_now->_x + 1, _now->_y + 1, _now);
		if (isValidPos(_now->_x, _now->_y + 1, _now))
			AddToOpenList(_now->_x, _now->_y + 1, _now);
		if (isValidPos(_now->_x - 1, _now->_y + 1, _now))
			AddToOpenList(_now->_x - 1, _now->_y + 1, _now);
		if (isValidPos(_now->_x - 1, _now->_y, _now))
			AddToOpenList(_now->_x - 1, _now->_y, _now);
		if (isValidPos(_now->_x - 1, _now->_y - 1, _now))
			AddToOpenList(_now->_x - 1, _now->_y - 1, _now);*/

		// STL에서는 OpenList에서 사라졌을때, 해당 항목을 가지고있는 iterator를 end로 만든다.
		_openList.erase(now);
		return true;
	}

	// 적어도 x,y가 멀쩡한 좌표다.
	// TODO : 대각선에서는 탐색만하고 나와야됨.

	bool CheckLine_UP(Node* me, bool bSearchOnly = false)
	{
		int now_x = me->_x;
		int now_y = me->_y - 1;
		bool direction[8] = {true, false, false, false, false, false, false, false};
		bool bCreate_node = false;

		// 0. 탐색가능 위치? + 1. 벽?
		while (isValidPos(now_x, now_y))
		{
			me->_searched.push_back(pair<int, int>(now_x, now_y));

			// 2. 목적지 인가?
			if (_dst.first == now_x && _dst.second == now_y)
			{
				// 해당 위치에 노드 생성 후 탈출
				if (!bSearchOnly)
				{
					AddToOpenList(now_x, now_y, me, direction);
				}

				return true;
			}

			// 3. 노드 생성 조건
			 
			// UP_LEFT FLAG
			// O 
			// X ME
			if (!isValidPos(now_x - 1, now_y) && isValidPos(now_x - 1, now_y - 1))
			{	
				direction[UP_LEFT] = true;
				// 해당 위치에 노드 생성 요청
				bCreate_node = true;
			}

			// UP_RIGHT FLAG
			//    O
			// ME X
			if (!isValidPos(now_x + 1, now_y) && isValidPos(now_x + 1, now_y - 1))
			{
				direction[UP_RIGHT] = true;
				// 해당 위치에 노드 생성 요청
				bCreate_node = true;
			}

			// 4. 노드 생성 요청이 들어왔으면, 생성 후 탈출
			if (bCreate_node)
			{
				if (bSearchOnly)
				{
					// 1. closelist
					for (auto& i : _closeList)
					{
						if (i.second->_x == now_x && i.second->_y == now_y)
						{
							return false;
						}
					}
				}
				if (!bSearchOnly)
					AddToOpenList(now_x, now_y, me, direction);

				return true;
			}

			now_y--;
		}
		return false;
	}

	void CheckAcross_UP_RIGHT(Node* me)
	{
		int now_x = me->_x + 1;
		int now_y = me->_y - 1;

		// FOR CONDITION 1
		bool bCreate_node = false;
		bool direction[8] = { false, false, false, false, false, false, false, false };

		// 0. 탐색가능 위치? + 1. 벽?
		while (isValidPos(now_x, now_y))
		{
			me->_searched.push_back(pair<int, int>(now_x, now_y));

			// 2. 목적지 인가?
			if (_dst.first == now_x && _dst.second == now_y)
			{
				// 해당 위치에 노드 생성 후 탈출
				AddToOpenList(now_x, now_y, me, direction);
				return;
			}

			// 3. 노드 생성
			
			// 조건 1.  
			// O
			// X NOW
			//    X  O			 
			if (!isValidPos(now_x - 1, now_y) && isValidPos(now_x - 1, now_y - 1))
			{
				bCreate_node = true;
				direction[UP_LEFT] = true;
			}

			if (!isValidPos(now_x, now_y + 1) && isValidPos(now_x + 1, now_y + 1))
			{
				bCreate_node = true;
				direction[DOWN_RIGHT] = true;
			}

			// 조건 2
			int temp_x = me->_x;
			int temp_y = me->_y;
			me->_x = now_x;
			me->_y = now_y;
			if (CheckLine_UP(me, true))
			{
				direction[UP] = true;
				bCreate_node = true;
			}

			if (CheckLine_RIGHT(me, true))
			{
				direction[RIGHT] = true;
				bCreate_node = true;
			}

			me->_x = temp_x;
			me->_y = temp_y;
			
			// 노드 생성 요청이 들어왔으면, 생성 판단 후 탈출
			if (bCreate_node)
			{
				direction[UP_RIGHT] = true;
				AddToOpenList(now_x, now_y, me, direction);

				return;
			}			

			now_x++;
			now_y--;
		}
		return;
	}

	bool CheckLine_RIGHT(Node* me, bool bSearchOnly = false)
	{
		int now_x = me->_x + 1;
		int now_y = me->_y;
		bool direction[8] = { false, false, true, false, false, false, false, false };
		bool bCreate_node = false;

		// 0. 탐색가능 위치? + 1. 벽?
		while (isValidPos(now_x, now_y))
		{
			me->_searched.push_back(pair<int, int>(now_x, now_y));

			// 2. 목적지 인가?
			if (_dst.first == now_x && _dst.second == now_y)
			{
				// 해당 위치에 노드 생성 후 탈출
				if (!bSearchOnly)
					AddToOpenList(now_x, now_y, me, direction);

				return true;
			}

			// 3. 노드 생성 조건

			// UP_RIGHT FLAG
			// X  O 
			// ME
			if (!isValidPos(now_x, now_y - 1) && isValidPos(now_x + 1, now_y - 1))
			{
				direction[UP_RIGHT] = true;
				// 해당 위치에 노드 생성 요청
				bCreate_node = true;
			}

			// DOWN_RIGHT FLAG
			// ME   
			// X  O
			if (!isValidPos(now_x, now_y + 1) && isValidPos(now_x + 1, now_y + 1))
			{
				direction[DOWN_RIGHT] = true;
				// 해당 위치에 노드 생성 요청
				bCreate_node = true;
			}

			// 4. 노드 생성 요청이 들어왔으면, 생성 후 탈출
			if (bCreate_node)
			{
				if (bSearchOnly)
				{
					// 1. closelist
					for (auto& i : _closeList)
					{
						if (i.second->_x == now_x && i.second->_y == now_y)
						{
							return false;
						}
					}
				}
				if (!bSearchOnly)
					AddToOpenList(now_x, now_y, me, direction);

				return true;
			}
			now_x++;
		}
		return false;
	}

	void CheckAcross_DOWN_RIGHT(Node* me)
	{
		int now_x = me->_x + 1;
		int now_y = me->_y + 1;

		// FOR CONDITION 1
		bool bCreate_node = false;
		bool direction[8] = { false, false, false, false, false, false, false, false };

		// 0. 탐색가능 위치? + 1. 벽?
		while (isValidPos(now_x, now_y))
		{
			me->_searched.push_back(pair<int, int>(now_x, now_y));

			// 2. 목적지 인가?
			if (_dst.first == now_x && _dst.second == now_y)
			{
				// 해당 위치에 노드 생성 후 탈출
				AddToOpenList(now_x, now_y, me, direction);

				return;
			}

			// 3. 노드 생성

			// 조건 1.  
			//    X  O
			// X NOW
			// O  		 
			if (!isValidPos(now_x, now_y - 1) && isValidPos(now_x + 1, now_y - 1))
			{
				bCreate_node = true;
				direction[UP_RIGHT] = true;
			}

			if (!isValidPos(now_x - 1, now_y) && isValidPos(now_x - 1, now_y + 1))
			{
				bCreate_node = true;
				direction[DOWN_LEFT] = true;
			}

			// 조건 2
			int temp_x = me->_x;
			int temp_y = me->_y;
			me->_x = now_x;
			me->_y = now_y;
			if (CheckLine_RIGHT(me, true))
			{
				direction[RIGHT] = true;
				bCreate_node = true;
			}

			if (CheckLine_DOWN(me, true))
			{
				direction[DOWN] = true;
				bCreate_node = true;
			}
			me->_x = temp_x;
			me->_y = temp_y;

			// 노드 생성 요청이 들어왔으면, 생성 판단 후 탈출
			if (bCreate_node)
			{
				direction[DOWN_RIGHT] = true;
				AddToOpenList(now_x, now_y, me, direction);

				return;
			}
			
			now_x++;
			now_y++;
		}
		return;
	}

	bool CheckLine_DOWN(Node* me, bool bSearchOnly = false)
	{
		int now_x = me->_x;
		int now_y = me->_y + 1;
		bool direction[8] = { false, false, false, false, true, false, false, false };
		bool bCreate_node = false;

		// 0. 탐색가능 위치? + 1. 벽?
		while (isValidPos(now_x, now_y))
		{
			me->_searched.push_back(pair<int, int>(now_x, now_y));

			// 2. 목적지 인가?
			if (_dst.first == now_x && _dst.second == now_y)
			{
				// 해당 위치에 노드 생성 후 탈출
				if (!bSearchOnly)
					AddToOpenList(now_x, now_y, me, direction);

				return true;
			}

			// 3. 노드 생성 조건

			// DOWN_RIGHT FLAG
			// ME X 
			//    O
			if (!isValidPos(now_x + 1, now_y) && isValidPos(now_x + 1, now_y + 1))
			{
				direction[DOWN_RIGHT] = true;
				// 해당 위치에 노드 생성 요청
				bCreate_node = true;
			}

			// DOWN_LEFT FLAG
			// X ME   
			// O  
			if (!isValidPos(now_x - 1, now_y) && isValidPos(now_x - 1, now_y + 1))
			{
				direction[DOWN_LEFT] = true;
				// 해당 위치에 노드 생성 요청
				bCreate_node = true;
			}

			// 4. 노드 생성 요청이 들어왔으면, 생성 후 탈출
			if (bCreate_node)
			{
				if (bSearchOnly)
				{
					// 1. closelist
					for (auto& i : _closeList)
					{
						if (i.second->_x == now_x && i.second->_y == now_y)
						{
							return false;
						}
					}
				}
				if (!bSearchOnly)
					AddToOpenList(now_x, now_y, me, direction);

				return true;
			}

			now_y++;
		}
		return false;
	}

	void CheckAcross_DOWN_LEFT(Node* me)
	{
		int now_x = me->_x - 1;
		int now_y = me->_y + 1;

		// FOR CONDITION 1
		bool bCreate_node = false;
		bool direction[8] = { false, false, false, false, false, false, false, false };

		// 0. 탐색가능 위치? + 1. 벽?
		while (isValidPos(now_x, now_y))
		{
			me->_searched.push_back(pair<int, int>(now_x, now_y));

			// 2. 목적지 인가?
			if (_dst.first == now_x && _dst.second == now_y)
			{
				// 해당 위치에 노드 생성 후 탈출
				AddToOpenList(now_x, now_y, me, direction);

				return;
			}

			// 3. 노드 생성

			// 조건 1.  
			// O  X  
			//   NOW X
			//    	 O 
			if (!isValidPos(now_x, now_y - 1) && isValidPos(now_x - 1, now_y - 1))
			{
				bCreate_node = true;
				direction[UP_LEFT] = true;
			}

			if (!isValidPos(now_x + 1, now_y) && isValidPos(now_x + 1, now_y + 1))
			{
				bCreate_node = true;
				direction[DOWN_RIGHT] = true;
			}

			// 조건 2
			int temp_x = me->_x;
			int temp_y = me->_y;
			me->_x = now_x;
			me->_y = now_y;
			if (CheckLine_LEFT(me, true))
			{
				direction[LEFT] = true;
				bCreate_node = true;
			}

			if (CheckLine_DOWN(me, true))
			{
				direction[DOWN] = true;
				bCreate_node = true;
			}

			me->_x = temp_x;
			me->_y = temp_y;

			// 노드 생성 요청이 들어왔으면, 생성 판단 후 탈출
			if (bCreate_node)
			{
				direction[DOWN_LEFT] = true;
				AddToOpenList(now_x, now_y, me, direction);

				return;
			}
			now_x--;
			now_y++;
		}
		return;
	}

	bool CheckLine_LEFT(Node* me, bool bSearchOnly = false)
	{
		int now_x = me->_x - 1;
		int now_y = me->_y;
		bool direction[8] = { false, false, false, false, false, false, true, false };
		bool bCreate_node = false;

		// 0. 탐색가능 위치? + 1. 벽?
		while (isValidPos(now_x, now_y))
		{
			me->_searched.push_back(pair<int, int>(now_x, now_y));

			// 2. 목적지 인가?
			if (_dst.first == now_x && _dst.second == now_y)
			{
				// 해당 위치에 노드 생성 후 탈출
				if (!bSearchOnly)
					AddToOpenList(now_x, now_y, me, direction);

				return true;
			}

			// 3. 노드 생성 조건

			// DOWN_LEFT FLAG
			//   ME 
			// O X
			if (!isValidPos(now_x, now_y + 1) && isValidPos(now_x - 1, now_y + 1))
			{
				direction[DOWN_LEFT] = true;
				// 해당 위치에 노드 생성 요청
				bCreate_node = true;
			}

			// UP_LEFT FLAG
			// O X 
			//   ME 
			if (!isValidPos(now_x, now_y - 1) && isValidPos(now_x - 1, now_y - 1))
			{
				direction[UP_LEFT] = true;
				// 해당 위치에 노드 생성 요청
				bCreate_node = true;
			}

			// 4. 노드 생성 요청이 들어왔으면, 생성 후 탈출
			if (bCreate_node)
			{
				if (bSearchOnly)
				{
					// 1. closelist
					for (auto& i : _closeList)
					{
						if (i.second->_x == now_x && i.second->_y == now_y)
						{
							return false;
						}
					}
				}
				if (!bSearchOnly)
					AddToOpenList(now_x, now_y, me, direction);

				return true;
			}
			now_x--;
		}
		return false;
	}

	void CheckAcross_UP_LEFT(Node* me)
	{
		int now_x = me->_x - 1;
		int now_y = me->_y - 1;

		// FOR CONDITION 1
		bool bCreate_node = false;
		bool direction[8] = { false, false, false, false, false, false, false, false };

		// 0. 탐색가능 위치? + 1. 벽?
		while (isValidPos(now_x, now_y))
		{
			me->_searched.push_back(pair<int, int>(now_x, now_y));

			// 2. 목적지 인가?
			if (_dst.first == now_x && _dst.second == now_y)
			{
				// 해당 위치에 노드 생성 후 탈출
				AddToOpenList(now_x, now_y, me, direction);

				return;
			}

			// 3. 노드 생성

			// 조건 1.  
			//       O
			//   NOW X
			// O  X 	 
			if (!isValidPos(now_x + 1, now_y) && isValidPos(now_x + 1, now_y - 1))
			{
				bCreate_node = true;
				direction[UP_RIGHT] = true;
			}

			if (!isValidPos(now_x, now_y + 1) && isValidPos(now_x - 1, now_y + 1))
			{
				bCreate_node = true;
				direction[DOWN_LEFT] = true;
			}

			// 조건 2
			int temp_x = me->_x;
			int temp_y = me->_y;
			me->_x = now_x;
			me->_y = now_y;
			if (CheckLine_UP(me, true))
			{
				direction[UP] = true;
				bCreate_node = true;
			}

			if (CheckLine_LEFT(me, true))
			{
				direction[LEFT] = true;
				bCreate_node = true;
			}

			me->_x = temp_x;
			me->_y = temp_y;

			// 노드 생성 요청이 들어왔으면, 생성 판단 후 탈출
			if (bCreate_node)
			{
				direction[UP_LEFT] = true;
				AddToOpenList(now_x, now_y, me, direction);

				return;
			}
			now_x--;
			now_y--;
		}
		return;
	}

	

	// 노드 생성
	Node* CreateNode(int x, int y, Node* parent, bool direction[8])
	{
		double g = GetG(x,y,parent);
		double h = GetH(x,y);

		Node* temp = new Node;
		temp->_x = x;
		temp->_y = y;
		temp->_parent = parent;
		for (int i = 0;i < 8;i++)
			temp->_direction[i] = direction[i];

		temp->_g = g;
		temp->_h = h;
		temp->_f = g + h;
		return temp;
	} const
	void DeleteNode(Node* n)
	{
		delete n;
	} const
	double GetG(int x, int y, Node* parent)
	{
		if (parent == nullptr)
			return 0;
		double val = parent->_g;

		int relative_x = x - parent->_x;
		int relative_y = y - parent->_y;

		if (_mode_g == Euclid)
		{
			// UP
			if (relative_x == 0 && relative_y < 0)
			{
				val += 1.0 * abs(relative_y);
			}
			// UP-RIGHT
			else if (relative_x > 0 && relative_y < 0)
			{
				val += sqrt(2.0) * relative_x;
			}
			// RIGHT
			else if (relative_x > 0 && relative_y == 0)
			{
				val += 1.0 * relative_x;
			}
			// DOWN-RIGHT
			else if (relative_x > 0 && relative_y > 0)
			{
				val += sqrt(2.0) * relative_x;
			}
			// DOWN
			else if (relative_x == 0 && relative_y > 0)
			{
				val += 1.0 * relative_y;
			}
			// DOWN-LEFT
			else if (relative_x < 0 && relative_y > 0)
			{
				val += sqrt(2.0) * relative_y;
			}
			// LEFT
			else if (relative_x < 0 && relative_y == 0)
			{
				val += 1.0 * abs(relative_x);
			}
			// UP-LEFT
			else if (relative_x < 0 && relative_y < 0)
			{
				val += sqrt(2.0) * abs(relative_y);
			}
		}
		else
		{
			// UP
			if (relative_x == 0 && relative_y < 0)
			{
				val += 1.0;
			}
			// UP-RIGHT
			else if (relative_x > 0 && relative_y < 0)
			{
				val += 2.0;
			}
			// RIGHT
			else if (relative_x > 0 && relative_y == 0)
			{
				val += 1.0;
			}
			// DOWN-RIGHT
			else if (relative_x > 0 && relative_y > 0)
			{
				val += 2.0;
			}
			// DOWN
			else if (relative_x == 0 && relative_y > 0)
			{
				val += 1.0;
			}
			// DOWN-LEFT
			else if (relative_x < 0 && relative_y > 0)
			{
				val += 2.0;
			}
			// LEFT
			else if (relative_x < 0 && relative_y == 0)
			{
				val += 1.0;
			}
			// UP-LEFT
			else if (relative_x < 0 && relative_y < 0)
			{
				val += 2.0;
			}
		}

		return val;
	} const
	double GetH(int x, int y)
	{
		double val;
		if (_mode_h == Euclid)
		{
			val = sqrt(pow(_dst.first - x, 2) + pow(_dst.second - y, 2));
		}
		else
		{
			val = fabs(_dst.first - x) + fabs(_dst.second - y);
		}

		return val;
	} const

	// 벽 추가
	void AddWall(int x, int y)
	{
		bool direction[8] = { false, false, false, false, false, false, false, false };
		Node* n = CreateNode(x, y, nullptr, direction);
		_wallList.insert(pair<double, Node*>{n->_f, n});
	}
	void ClearWall()
	{
		_wallList.clear();
	}

	void AddToOpenList(int x, int y, Node* parent, bool direction[8])
	{
		const double g = GetG(x, y, parent);
		const double h = GetH(x, y);

		// 1. closelist
		for (auto& i : _closeList)
		{
			if (i.second->_x == x && i.second->_y == y)
			{
				/*if (g < i.second->_g)
				{
					return;
				}*/
				return;
			}

		}
		// 2. openlist 갱신
		pair<double, Node*> temp;
		for (auto i = _openList.begin();i != _openList.end();i++)
		{
			if ((*i).second->_x == x && (*i).second->_y == y)
			{
				// openlist 경로 갱신이 필요하면
				if (g < (*i).second->_g)
				{
					temp = *i;
					_openList.erase(i);
					temp.second->_parent = parent;
					temp.second->_g = g;
					temp.second->_f = temp.second->_g + temp.second->_h;
					temp.first = temp.second->_f;
					_openList.insert(temp);
				}

				return;
			}
		}

		// 새로 추가
		Node* n = CreateNode(x, y, parent, direction);
		_openList.insert(pair<double, Node*>{n->_f, n});
	}

	// 기타
	bool FoundDst() const
	{
		return _foundDst;
	}
	// 목적지에
	bool Unreachable() const
	{
		return _is_unreachable;
	}

	///////////////////////////////////////////////////////
	// map 범위 내에있는지, wall인지
	///////////////////////////////////////////////////////
	bool isValidPos(int x, int y) const
	{
		if (x >= 0 && x < _mapSizeX &&
			y >= 0 && y < _mapSizeY)
		{
			for (auto pair : _wallList)
			{
				if (pair.second->_x == x && pair.second->_y == y)
					return false;
			}

			return true;
		}
		else
			return false;
	}


public:
	// path find
	multimap<double, Node*> _openList;
	multimap<double, Node*> _closeList;
	Node* _now;

	// MAP
	multimap<double, Node*> _wallList;
	int _mapSizeX;
	int _mapSizeY;	
	// x, y
	pair<int, int> _src;
	pair<int, int> _dst;

	// Class State
	// 목적지 도착?
	bool _foundDst;
	// 목적지 도달 불가?
	bool _is_unreachable;

public:
	DistanceMode _mode_g;
	DistanceMode _mode_h;
};