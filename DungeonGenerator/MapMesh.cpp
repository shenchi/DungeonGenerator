#include "MapMesh.h"
#include <unordered_map>
#include <functional>

//#define OUTPUT_TO_FILE
#ifdef OUTPUT_TO_FILE
#include <cstdio>
#endif 

using namespace std;

MapMesh::MapMesh()
{
}


MapMesh::~MapMesh()
{
}

void MapMesh::CreateFromGridMap(const char* map, int width, int height, const char* tileTypes, int nTileTypes, int defaultDensity)
{
	walls.clear();

	unordered_map<char, size_t> density;
	typedef unordered_map<char, size_t>::iterator iter_type;

	for (size_t i = 0; i < nTileTypes; ++i)
	{
		density.insert({ tileTypes[i], i });
	}

	auto density_func = [map, width, height, defaultDensity, density](int x, int y) -> size_t
	{
		if (((x) < 0 || (x) >= width || (y) < 0 || (y) >= height))
			return defaultDensity;

		auto iter = density.find(map[y * width + x]);
		if (iter != density.end())
			return iter->second;

		return defaultDensity;
	};

	auto func = [](int& width, int& height, function<size_t(int, int)> density_func, function<void(int, int, int, int, int)> add_line)
	{
		for (int y = 0; y <= height; ++y)
		{
			int start = -1;
			int dir = 0;
			size_t last_den_l = -1;
			size_t last_den_r = -1;

			for (int x = 0; x <= width; ++x)
			{
				iter_type iter;
				size_t den_l = density_func(x, y - 1);
				size_t den_r = density_func(x, y);
				int den_delta = (int)den_l - (int)den_r;
				if (den_delta < 0)
				{
					den_delta = -1;
				}
				else if (den_delta > 0)
				{
					den_delta = 1;
				}

				if (start < 0 && den_delta != 0)
				{
					start = x;
					dir = den_delta;
				}
				else if (start >= 0 && ((dir < 0 && (den_r != last_den_r || dir != den_delta)) || (dir > 0 && (den_l != last_den_l || dir != den_delta))))
				{
					add_line(start, x, y, (dir < 0 ? (int)den_r : (int)den_l), dir);

					if (den_delta != 0)
					{
						start = x;
						dir = den_delta;
					}
					else
					{
						start = -1;
						dir = 0;
					}
				}

				last_den_l = den_l;
				last_den_r = den_r;
			}
		}
	};

	func(width, height,
		[density_func](int x, int y) { return density_func(x, y); },
		[this](int start, int x, int y, int label, int dir) { walls.push_back({ start, y, x, y, label, dir > 0 }); });

	func(height, width,
		[density_func](int x, int y) { return density_func(y, x); },
		[this](int start, int x, int y, int label, int dir) { walls.push_back({ y, start, y, x, label, dir < 0 }); });

}

void MapMesh::GenerateMesh(float stepSize, float height, float* colorList)
{
	vertices.clear();
	indices.clear();

	float white[] = { 1.0f, 1.0f, 1.0f };

	for (auto w = walls.begin(); w != walls.end(); ++w)
	{
		int sx = w->sx, sy = w->sy, tx = w->tx, ty = w->ty;
		if (!w->faceRight)
		{
			int t = sx; sx = tx; tx = t;
			t = sy; sy = ty; ty = t;
		}

		float* color = white;
		if (nullptr != colorList)
		{
			color = &(colorList[w->label * 3]);
		}

		float pos[] = {
			sx * stepSize,      0, sy * stepSize,
			tx * stepSize,      0, ty * stepSize,
			sx * stepSize, height, sy * stepSize,
			tx * stepSize, height, ty * stepSize,
		};

		float vec1[] = { pos[3] - pos[0], pos[4] - pos[1], pos[5] - pos[2] };
		float vec2[] = { pos[6] - pos[0], pos[7] - pos[1], pos[8] - pos[2] };
		float dist1 = sqrt(vec1[0] * vec1[0] + vec1[1] * vec1[1] + vec1[2] * vec1[2]);
		float dist2 = sqrt(vec2[0] * vec2[0] + vec2[1] * vec2[1] + vec2[2] * vec2[2]);

		vec1[0] /= dist1;
		vec1[1] /= dist1;
		vec1[2] /= dist1;
		vec2[0] /= dist2;
		vec2[1] /= dist2;
		vec2[2] /= dist2;

		float normal[] = {
			vec1[1] * vec2[2] - vec1[2] * vec2[1],
			vec1[2] * vec2[0] - vec1[0] * vec2[2],
			vec1[0] * vec2[1] - vec1[1] * vec2[0],
		};

		int startIdx = (int)vertices.size();

		for (size_t i = 0; i < 4; i++)
		{
			vertices.push_back({ pos[i * 3], pos[i * 3 + 1], pos[i * 3 + 2], normal[0], normal[1], normal[2], color[0], color[1], color[2] });
		}

		indices.push_back(startIdx + 0);
		indices.push_back(startIdx + 1);
		indices.push_back(startIdx + 2);
		indices.push_back(startIdx + 2);
		indices.push_back(startIdx + 1);
		indices.push_back(startIdx + 3);
	}


#ifdef OUTPUT_TO_FILE

	FILE *fp = fopen("mesh.obj", "w");

	for (auto i = vertices.begin(); i != vertices.end(); ++i)
	{
		fprintf(fp, "v %f %f %f\n", i->x, i->y, i->z);
	}

	for (auto i = vertices.begin(); i != vertices.end(); ++i)
	{
		fprintf(fp, "vn %f %f %f\n", i->nx, i->ny, i->nz);
	}

	for (size_t i = 0; i < indices.size(); i += 3)
	{
		fprintf(fp, "f %d %d %d\n", indices[i] + 1, indices[i + 1] + 1, indices[i + 2] + 1);
	}

	fclose(fp);

#endif

}

