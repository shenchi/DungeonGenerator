#include "MapGenerator.h"
#ifdef _DEBUG
#include <cassert>
#endif

using namespace std;

MapGenerator::MapGenerator()
{
}

MapGenerator::MapGenerator(int seed)
{
	generator.seed(seed);
}


MapGenerator::~MapGenerator()
{
}

void MapGenerator::SetSeed(unsigned int seed)
{
	generator.seed(seed);
}

void MapGenerator::Start(int cellCount, int randomRadius, int minSideLength, int maxSideLength)
{
	if (state != Empty || cellCount <= 0 || randomRadius <= 0 || minSideLength > maxSideLength || minSideLength <= 0)
	{
		return;
	}

	uniform_int_distribution<int> lengthDist(minSideLength, maxSideLength);
	uniform_int_distribution<int> posDist(-randomRadius, randomRadius - maxSideLength);

	int thresholdLength = minSideLength + int(0.75f * (maxSideLength - minSideLength));

	cells.resize(cellCount);
	connections.resize(cellCount * cellCount);
	for (int i = 0; i < cellCount; ++i)
	{
		cells[i].width = lengthDist(generator);
		cells[i].height = lengthDist(generator);
		cells[i].x = posDist(generator);
		cells[i].y = posDist(generator);
		cells[i].room = (cells[i].width * cells[i].height > thresholdLength * thresholdLength);
	}

	UpdateRect();

	state = Started;
}

void MapGenerator::Update()
{
	switch (state)
	{
	case MapGenerator::Empty:
		break;
	case MapGenerator::Started:
		state = Expanding;
	case MapGenerator::Expanding:
		Expand();
		break;
	case MapGenerator::Connecting:
		Connect();
		break;
	case MapGenerator::Finished:
	default:
		break;
	}
}

void MapGenerator::Gen2DArrayMap(char* map, size_t& width, size_t& height, const char tileTable[NumTileType]) const
{
	if (!IsFinished() || (right - left + 1 > (int)width) || (bottom - top + 1 > (int)height))
	{
		width = height = 0;
		return;
	}

	size_t w = right - left + 1;
	size_t h = bottom - top + 1;

	memset(map, tileTable[Void], width * height);

	for (auto i = cells.begin(); i != cells.end(); ++i)
	{
		if (i->discard) continue;

		size_t l = i->x - left;
		size_t t = i->y - top;
		size_t r = l + i->width - 1;
		size_t b = t + i->height - 1;

#ifdef _DEBUG
		assert(l >= 0 && t >= 0 && r < w && b < h);
#endif

		for (size_t y = t; y <= b; ++y)
		{
			for (size_t x = l; x <= r; ++x)
			{
				if (i->room && (y == t || y == b || x == l || x == r))
				{
					map[y * width + x] = tileTable[Wall];
				}
				else
				{
					map[y * width + x] = tileTable[Walkable];
				}
			}
		}
	}

	for (auto i = corridors.begin(); i != corridors.end(); ++i)
	{
		int l, t, r, b;
		i->rect(l, t, r, b);
		l -= left;
		r -= left;
		t -= top;
		b -= top;
#ifdef _DEBUG
		assert(l >= 0 && t >= 0 && r < (int)w && b < (int)h);
#endif
		for (int y = t; y <= b; ++y)
		{
			for (int x = l; x <= r; ++x)
			{
				map[y * width + x] = tileTable[Walkable];
			}
		}
	}

	width = w;
	height = h;

}

void MapGenerator::UpdateRect()
{
	const size_t len = cells.size();
	bool first = true;
	for (size_t i = 0; i < len; ++i)
	{
		if (cells[i].discard) continue;

		if (first)
		{
			left = cells[i].x;
			top = cells[i].y;
			right = left + cells[i].width;
			bottom = top + cells[i].height;

			first = false;
			continue;
		}

		if (cells[i].x < left) left = cells[i].x;
		if (cells[i].y < top) top = cells[i].y;
		if (cells[i].x + cells[i].width - 1 > right) right = cells[i].x + cells[i].width - 1;
		if (cells[i].y + cells[i].height - 1 > bottom) bottom = cells[i].y + cells[i].height - 1;
	}
}

void MapGenerator::Expand()
{
	const size_t len = cells.size();
	vector<int> forceX(len), forceY(len);
	int fx, fy;

	bool finished = true;

	for (size_t a = 0; a < len; ++a)
	{
		for (size_t b = a + 1; b < len; ++b)
		{
			if (!SeparatingSteering(cells[a], cells[b], fx, fy))
			{
				continue;
			}

			finished = false;

			if (fx > 0)
			{
				forceX[a]--;
				forceX[b]++;
			}
			else if (fx < 0)
			{
				forceX[a]++;
				forceX[b]--;
			}
			if (fy > 0)
			{
				forceY[a]--;
				forceY[b]++;
			}
			else if (fy < 0)
			{
				forceY[a]++;
				forceY[b]--;
			}
		}
	}

	if (finished)
	{
		UpdateRect();
		state = Connecting;
		return;
	}

	const static int stepLimit = 1;

	for (size_t i = 0; i < len; ++i)
	{
		if (forceX[i] < -stepLimit) forceX[i] = -stepLimit;
		if (forceX[i] > stepLimit) forceX[i] = stepLimit;
		if (forceY[i] < -stepLimit) forceY[i] = -stepLimit;
		if (forceY[i] > stepLimit) forceY[i] = stepLimit;
		cells[i].x += forceX[i];
		cells[i].y += forceY[i];
	}
}

void MapGenerator::Connect()
{
	const size_t len = cells.size();

	// calculating distances for rooms;
	vector<float> dists(len * len);
	for (size_t i = 0; i < len; ++i)
	{
		if (!cells[i].room) continue;
		for (size_t j = i + 1; j < len; ++j)
		{
			if (!cells[j].room) continue;
			float dist = sqrt((cells[i].cx() - cells[j].cx()) * (cells[i].cx() - cells[j].cx())
				+ (cells[i].cy() - cells[j].cy()) * (cells[i].cy() - cells[j].cy()));

			dists[i * len + j] = dist;
			dists[j * len + i] = dist;
		}
	}

	// generating connection graph
	for (size_t i = 0; i < len; ++i)
	{
		if (!cells[i].room) continue;
		for (size_t j = i + 1; j < len; ++j)
		{
			if (!cells[j].room) continue;
			bool connect = true;
			float dist_ij = dists[i * len + j];
			for (size_t k = 0; k < len; ++k)
			{
				if (!cells[k].room || k == i || k == j)
					continue;

				if (dists[i * len + k] < dist_ij && dists[j * len + k] < dist_ij)
				{
					connect = false;
					break;
				}
			}

			if (connect)
			{
				connections[i * len + j] = true;
				connections[j * len + i] = true;
			}
		}
	}

	// building corridor
	uniform_real_distribution<float> prob;
	uniform_int_distribution<int> widthDist(1, 3);

	for (auto c = cells.begin(); c != cells.end(); ++c)
	{
		c->discard = !c->room;
	}
	corridors.clear();

	for (size_t i = 0; i < len; ++i)
	{
		int startX = (int)cells[i].cx();
		int startY = (int)cells[i].cy();

		for (size_t j = i + 1; j < len; ++j)
		{
			if (!connections[i * len + j]) continue;
			int endX = (int)cells[j].cx();
			int endY = (int)cells[j].cy();

			int width = widthDist(generator);

			if (prob(generator) > 0.5f)
			{
				AddCorridor(startX, startY, startX, endY, width);
				AddCorridor(startX, endY, endX, endY, width);
			}
			else
			{
				AddCorridor(startX, startY, endX, startY, width);
				AddCorridor(endX, startY, endX, endY, width);
			}
		}
	}

	UpdateRect();

	state = Finished;
}

void MapGenerator::AddCorridor(int startX, int startY, int endX, int endY, int width)
{
	if (!((startX == endX) ^ (startY == endY)) || width <= 0)
	{
		return;
	}

	corridors.push_back({ startX, startY, endX, endY, width });

	int negHalf = width / 2;
	int posHalf = (width - 1) / 2;

	for (auto c = cells.begin(); c != cells.end(); ++c)
	{
		if (!c->discard) continue;
		if (startX == endX && startX + posHalf >= c->x && startX - negHalf < c->x + c->width)
		{
			if (endY < startY) swap(startY, endY);
			if (endY < c->y || startY >= c->y + c->height)
				continue;
		}
		else if (startY == endY && startY + posHalf >= c->y && startY - negHalf < c->y + c->height)
		{
			if (endX < startX) swap(startX, endX);
			if (endX < c->x || startX >= c->x + c->width)
				continue;
		}
		else
		{
			continue;
		}
		c->discard = false;
	}
}

bool MapGenerator::SeparatingSteering(const Cell& a, const Cell& b, int& fx, int& fy)
{
	int dxa = a.x + a.width - b.x;
	int dxb = a.x - b.x - b.width;
	int dya = a.y + a.height - b.y;
	int dyb = a.y - b.y - b.height;
	bool overlappedX = dxa > 0 && dxb < 0;
	bool overlappedY = dya > 0 && dyb < 0;
	fx = abs(dxa) < abs(dxb) ? dxa : dxb;
	fy = abs(dya) < abs(dyb) ? dya : dyb;
	return overlappedX && overlappedY;
}
