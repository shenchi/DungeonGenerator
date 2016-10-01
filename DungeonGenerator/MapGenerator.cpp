#include "MapGenerator.h"

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

		if (i == 0)
		{
			left = cells[i].x;
			top = cells[i].y;
			right = left + cells[i].width;
			bottom = top + cells[i].height;
		}
		else
		{
			if (cells[i].x < left) left = cells[i].x;
			if (cells[i].y < top) left = cells[i].y;
			if (cells[i].x + cells[i].width > right) right = cells[i].x + cells[i].width;
			if (cells[i].y + cells[i].height > bottom) bottom = cells[i].y + cells[i].height;
		}
	}

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

		if (cells[i].x < left) left = cells[i].x;
		if (cells[i].y < top) left = cells[i].y;
		if (cells[i].x + cells[i].width > right) right = cells[i].x + cells[i].width;
		if (cells[i].y + cells[i].height > bottom) bottom = cells[i].y + cells[i].height;
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
		int startX = cells[i].cx();
		int startY = cells[i].cy();

		for (size_t j = i + 1; j < len; ++j)
		{
			if (!connections[i * len + j]) continue;
			int endX = cells[j].cx();
			int endY = cells[j].cy();

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
