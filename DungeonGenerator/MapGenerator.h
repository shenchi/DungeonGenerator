#pragma once

#include <random>
#include <vector>

struct Cell
{
	int		width;
	int		height;
	int		x;
	int		y;
	bool	room;
	bool	discard;

	inline float cx() const { return x + width * 0.5f; }
	inline float cy() const { return y + height * 0.5f; }
};

struct Corridor
{
	int startX;
	int startY;
	int endX;
	int endY;
	int width;
};

class MapGenerator
{
public:
	MapGenerator();
	MapGenerator(int seed);
	~MapGenerator();

	void SetSeed(unsigned int seed);

	void Start(int cellCount, int randomRadius, int minSideLength, int maxSideLength);

	void Update();

	bool IsFinished() const { return state == Finished; }

	const std::vector<Cell>& GetCells() const { return cells; }
	const std::vector<bool>& GetConnections() const { return connections; }
	const std::vector<Corridor>& GetCorridors() const { return corridors;  }

	int Left() const { return left; }
	int Top() const { return top; }
	int Right() const { return right; }
	int Bottom() const { return bottom; }

private:

	void Expand();
	void Connect();
	void AddCorridor(int startX, int startY, int endX, int endY, int width);

	static bool SeparatingSteering(const Cell& a, const Cell& b, int& fx, int& fy);

private:

	enum State
	{
		Empty,
		Started,
		Expanding,
		Connecting,
		Finished
	};

	int							left;
	int							top;
	int							right;
	int							bottom;

	State						state;

	std::vector<Cell>			cells;
	std::vector<bool>			connections;
	std::vector<Corridor>		corridors;

	std::default_random_engine	generator;
};

