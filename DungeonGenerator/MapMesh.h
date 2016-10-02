#pragma once

#include <vector>

struct Vertex
{
	float x, y, z;
	float nx, ny, nz;
	float r, g, b;
};

struct LineWall
{
	int sx, sy, tx, ty;
	int label;
	bool faceRight;
};

class MapMesh
{
public:
	MapMesh();
	~MapMesh();

	void CreateFromGridMap(const char* map, int width, int height, const char* tileTypes, size_t nTileTypes, int defaultDensity);
	void GenerateMesh(float stepSize, float height, float* colorList = nullptr);

	const std::vector<LineWall>& GetWalls() const { return walls; }
	const std::vector<Vertex>& GetVertices() const { return vertices; }
	const std::vector<int>& GetIndices() const { return indices; }
	
private:
	std::vector<LineWall>	walls;
	std::vector<Vertex>		vertices;
	std::vector<int>		indices;
};

