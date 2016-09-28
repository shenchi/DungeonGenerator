#pragma once

#include <vector>

struct Cell
{

};

class MapGenerator
{
public:
	MapGenerator();
	~MapGenerator();

	void Start(int roomCount);

	void Update();

	bool IsFinished() const;

	const std::vector<Cell>& GetCells() const { return cells; }

	const std::vector<int>& GetRooms() const { return rooms; }

private:
	std::vector<Cell>	cells;
	std::vector<int>	rooms;

};

