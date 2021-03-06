#include <Field.h>

Field::Field(const glm::ivec4 size, const int lightDist, const int roomSize)
	: size(size), totalSize(size.x*size.y*size.z*size.w), lightDist(lightDist), roomSize(roomSize)
{
	map = new Cell_t[totalSize];
	lightMap = new Light_t[totalSize];
	winMap = new Cell_t[winMapSize.x*winMapSize.y*winMapSize.z*winMapSize.w];
	winLightMap = new Light_t[winMapSize.x*winMapSize.y*winMapSize.z*winMapSize.w];

	curMap = map;
	curLightMap = lightMap;
}

Field::~Field()
{
	delete[] map;
	delete[] lightMap;
	delete[] winMap;
	delete[] winLightMap;
}

void Field::Init(Maze* maze, Shader* shader)
{
	this->shader = shader;

	for (int i = 0; i < totalSize; i++)
	{
		curMap[i] = 0;
		curLightMap[i] = 0;
	}

	CreateBorders();
	CreateExit(maze);
	GenerateWalls(maze);
	GenerateLight(maze);

	this->LoadMazeToGL(shader);

	int memBytes = (sizeof(Cell_t) + sizeof(Light_t)) * (totalSize + winMapSize.x*winMapSize.y*winMapSize.z*winMapSize.z);
	Log("cubesCount: ", cubesCount, ", mem(map): ", memBytes / 1024.0f / 1024.0f, " Mb");
}

void Field::CreateCube(int x, int y, int z, int w)
{
	int index = GetIndex(x, y, z, w);
	if ((curMap[index] & WALL_BLOCK) == 0)
	{
		curMap[index] |= WALL_BLOCK;
		cubesCount++;
	}
};

void Field::CreateBorders()
{
	for (int x = 0; x < size.x; x++)
	{
		for (int y = 0; y < size.y; y++)
		{
			for (int z = 0; z < size.z; z++)
			{
				for (int w = 0; w < size.w; w++)
				{
					if (!(x > 0 && x < (size.x - 1) &&
						y > 0 && y < (size.y - 1) &&
						z > 0 && z < (size.z - 1) &&
						w > 0 && w < (size.w - 1)))
					{
						CreateCube(x, y, z, w);
					}
				}
			}
		}
	}
}

void Field::CreateExit(Maze* maze)
{
	Random* rnd = Random::GetInstance();

	auto createExit = [&](int edge)
	{
		glm::ivec4 start = glm::ivec4(
			(maze->size.x - 1)*roomSize + 1,
			(maze->size.y - 1)*roomSize + 1,
			(maze->size.z - 1)*roomSize + 1,
			(maze->size.w - 1)*roomSize + 1
		);
		glm::ivec4 end = glm::ivec4(
			(maze->size.x)*roomSize,
			(maze->size.y)*roomSize,
			(maze->size.z)*roomSize,
			(maze->size.w)*roomSize
		);
		switch (edge)
		{
		case 0: start.x = (maze->size.x)*roomSize; end.x = (maze->size.x)*roomSize + 1; break;
		case 1: start.y = (maze->size.y)*roomSize; end.y = (maze->size.y)*roomSize + 1; break;
		case 2: start.z = (maze->size.z)*roomSize; end.z = (maze->size.z)*roomSize + 1; break;
		case 3: start.w = (maze->size.w)*roomSize; end.w = (maze->size.w)*roomSize + 1; break;
		default: break;
		}
		for (int x = start.x; x < end.x; x++)
		{
			for (int y = start.y; y < end.y; y++)
			{
				for (int z = start.z; z < end.z; z++)
				{
					for (int w = start.w; w < end.w; w++)
					{
						int index = GetIndex(x, y, z, w);
						curMap[index] = WIN_BLOCK;
						cubesCount--;
					}
				}
			}
		}
	};

	std::vector<int> edgesForExit;
	if (maze->size.x > 1) edgesForExit.push_back(0);
	if (maze->size.y > 1) edgesForExit.push_back(1);
	if (maze->size.z > 1) edgesForExit.push_back(2);
	if (maze->size.w > 1) edgesForExit.push_back(3);
	if (edgesForExit.size() > 0)
		createExit(edgesForExit[rnd->GetInt(0, edgesForExit.size() - 1)]);
}

void Field::GenerateWalls(Maze* maze)
{
	auto createShape = [&](glm::ivec4 start, glm::ivec4 end)
	{
		end.x = glm::min(end.x, size.x - 1);
		end.y = glm::min(end.y, size.y - 1);
		end.z = glm::min(end.z, size.z - 1);
		end.w = glm::min(end.w, size.w - 1);

		for (int x = start.x; x < end.x; x++)
		{
			for (int y = start.y; y < end.y; y++)
			{
				for (int z = start.z; z < end.z; z++)
				{
					for (int w = start.w; w < end.w; w++)
					{
						CreateCube(x, y, z, w);
					}
				}
			}
		}
	};


	for (int x = 0; x < maze->size.x; x++)
	{
		for (int y = 0; y < maze->size.y; y++)
		{
			for (int z = 0; z < maze->size.z; z++)
			{
				for (int w = 0; w < maze->size.w; w++)
				{
					glm::ivec4 step = glm::ivec4(
						size.x / maze->size.x,
						size.y / maze->size.y,
						size.z / maze->size.z,
						size.w / maze->size.w);

					// Create walls (only on negative edges)
					if (maze->IsWallExist(NEG_X, glm::ivec4(x, y, z, w)))
					{
						createShape(glm::ivec4(x*step.x, y*step.y, z*step.z, w*step.w),
							glm::ivec4(x*step.x + 1, (y + 1)*step.y, (z + 1)*step.z, (w + 1)*step.w));
					}
					if (maze->IsWallExist(NEG_Y, glm::ivec4(x, y, z, w)))
					{
						createShape(glm::ivec4(x*step.x, y*step.y, z*step.z, w*step.w),
							glm::ivec4((x + 1)*step.x, y*step.y + 1, (z + 1)*step.z, (w + 1)*step.w));
					}
					if (maze->IsWallExist(NEG_Z, glm::ivec4(x, y, z, w)))
					{
						createShape(glm::ivec4(x*step.x, y*step.y, z*step.z, w*step.w),
							glm::ivec4((x + 1)*step.x, (y + 1)*step.y, z*step.z + 1, (w + 1)*step.w));
					}
					if (maze->IsWallExist(NEG_W, glm::ivec4(x, y, z, w)))
					{
						createShape(glm::ivec4(x*step.x, y*step.y, z*step.z, w*step.w),
							glm::ivec4((x + 1)*step.x, (y + 1)*step.y, (z + 1)*step.z, w*step.w + 1));
					}

					// Create corners (check positive edges in current room and create corners
					// on negative edges in adjoining rooms)
					if (maze->IsWallExist(POS_X, glm::ivec4(x, y, z, w)) && maze->IsWallExist(POS_Y, glm::ivec4(x, y, z, w)))
					{
						createShape(glm::ivec4((x + 1)*step.x, (y + 1)*step.y, z*step.z, w*step.w),
							glm::ivec4((x + 1)*step.x + 1, (y + 1)*step.y + 1, (z + 1)*step.z, (w + 1)*step.w));
					}
					if (maze->IsWallExist(POS_X, glm::ivec4(x, y, z, w)) && maze->IsWallExist(POS_Z, glm::ivec4(x, y, z, w)))
					{
						createShape(glm::ivec4((x + 1)*step.x, y*step.y, (z + 1)*step.z, w*step.w),
							glm::ivec4((x + 1)*step.x + 1, (y + 1)*step.y, (z + 1)*step.z + 1, (w + 1)*step.w));
					}
					if (maze->IsWallExist(POS_X, glm::ivec4(x, y, z, w)) && maze->IsWallExist(POS_W, glm::ivec4(x, y, z, w)))
					{
						createShape(glm::ivec4((x + 1)*step.x, y*step.y, z*step.z, (w + 1)*step.w),
							glm::ivec4((x + 1)*step.x + 1, (y + 1)*step.y, (z + 1)*step.z, (w + 1)*step.w + 1));
					}
					if (maze->IsWallExist(POS_Y, glm::ivec4(x, y, z, w)) && maze->IsWallExist(POS_Z, glm::ivec4(x, y, z, w)))
					{
						createShape(glm::ivec4(x*step.x, (y + 1)*step.y, (z + 1)*step.z, w*step.w),
							glm::ivec4((x + 1)*step.x, (y + 1)*step.y + 1, (z + 1)*step.z + 1, (w + 1)*step.w));
					}
					if (maze->IsWallExist(POS_Y, glm::ivec4(x, y, z, w)) && maze->IsWallExist(POS_W, glm::ivec4(x, y, z, w)))
					{
						createShape(glm::ivec4(x*step.x, (y + 1)*step.y, z*step.z, (w + 1)*step.w),
							glm::ivec4((x + 1)*step.x, (y + 1)*step.y + 1, (z + 1)*step.z, (w + 1)*step.w + 1));
					}
					if (maze->IsWallExist(POS_Z, glm::ivec4(x, y, z, w)) && maze->IsWallExist(POS_W, glm::ivec4(x, y, z, w)))
					{
						createShape(glm::ivec4(x*step.x, y*step.y, (z + 1)*step.z, (w + 1)*step.w),
							glm::ivec4((x + 1)*step.x, (y + 1)*step.y, (z + 1)*step.z + 1, (w + 1)*step.w + 1));
					}
					if (maze->IsWallExist(POS_X, glm::ivec4(x, y, z, w)) && maze->IsWallExist(POS_Y, glm::ivec4(x, y, z, w)) &&
						maze->IsWallExist(POS_Z, glm::ivec4(x, y, z, w)))
					{
						createShape(glm::ivec4((x + 1)*step.x, (y + 1)*step.y, (z + 1)*step.z, w*step.w),
							glm::ivec4((x + 1)*step.x + 1, (y + 1)*step.y + 1, (z + 1)*step.z + 1, (w + 1)*step.w));
					}
					if (maze->IsWallExist(POS_X, glm::ivec4(x, y, z, w)) && maze->IsWallExist(POS_Y, glm::ivec4(x, y, z, w)) &&
						maze->IsWallExist(POS_W, glm::ivec4(x, y, z, w)))
					{
						createShape(glm::ivec4((x + 1)*step.x, (y + 1)*step.y, z*step.z, (w + 1)*step.w),
							glm::ivec4((x + 1)*step.x + 1, (y + 1)*step.y + 1, (z + 1)*step.z, (w + 1)*step.w + 1));
					}
					if (maze->IsWallExist(POS_X, glm::ivec4(x, y, z, w)) && maze->IsWallExist(POS_Z, glm::ivec4(x, y, z, w)) &&
						maze->IsWallExist(POS_W, glm::ivec4(x, y, z, w)))
					{
						createShape(glm::ivec4((x + 1)*step.x, y*step.y, (z + 1)*step.z, (w + 1)*step.w),
							glm::ivec4((x + 1)*step.x + 1, (y + 1)*step.y, (z + 1)*step.z + 1, (w + 1)*step.w + 1));
					}
					if (maze->IsWallExist(POS_Y, glm::ivec4(x, y, z, w)) && maze->IsWallExist(POS_Z, glm::ivec4(x, y, z, w)) &&
						maze->IsWallExist(POS_W, glm::ivec4(x, y, z, w)))
					{
						createShape(glm::ivec4(x*step.x, (y + 1)*step.y, (z + 1)*step.z, (w + 1)*step.w),
							glm::ivec4((x + 1)*step.x, (y + 1)*step.y + 1, (z + 1)*step.z + 1, (w + 1)*step.w + 1));
					}
					if (maze->IsWallExist(POS_X, glm::ivec4(x, y, z, w)) && maze->IsWallExist(POS_Y, glm::ivec4(x, y, z, w)) &&
						maze->IsWallExist(POS_Z, glm::ivec4(x, y, z, w)) && maze->IsWallExist(POS_W, glm::ivec4(x, y, z, w)))
					{
						createShape(glm::ivec4((x + 1)*step.x, (y + 1)*step.y, (z + 1)*step.z, (w + 1)*step.w),
							glm::ivec4((x + 1)*step.x + 1, (y + 1)*step.y + 1, (z + 1)*step.z + 1, (w + 1)*step.w + 1));
					}
				}
			}
		}
	}
}

void Field::GenerateLight(Maze* maze)
{
	auto generateLight = [&](glm::ivec4 pos)
	{
		int index = GetIndex(pos.x, pos.y, pos.z, pos.w);
		GenerateLightRecursive(pos.x, pos.y, pos.z, pos.w, lightDist, -1);
		curLightMap[index] = UINT32_MAX;
		curMap[index] = LIGHT_BLOCK | WALL_BLOCK;
		cubesCount++;
	};

	Random* rnd = Random::GetInstance();

	std::vector<int> tmpWalls;
	for (int x = 0; x < maze->size.x; x++)
	{
		for (int y = 0; y < maze->size.y; y++)
		{
			for (int z = 0; z < maze->size.z; z++)
			{
				for (int w = 0; w < maze->size.w; w++)
				{
					for (int e = 0; e < EDGES_COUNT; e++)
					{
						if ((e == POS_X || e == NEG_X) && maze->size.x == 1 ||
							(e == POS_Y || e == NEG_Y) && maze->size.y == 1 ||
							(e == POS_Z || e == NEG_Z) && maze->size.z == 1 ||
							(e == POS_W || e == NEG_W) && maze->size.w == 1) continue;
						if (maze->IsWallExist(e, glm::ivec4(x, y, z, w)))
						{
							tmpWalls.push_back(e);
						}
					}

					if (tmpWalls.size() == 0)
					{
						generateLight(glm::ivec4(
							x*roomSize + roomSize / 2,
							y*roomSize + roomSize / 2,
							z*roomSize + roomSize / 2,
							w*roomSize + roomSize / 2));
					}
					else
					{
						int randomEdgeIndex = rnd->GetInt(0, tmpWalls.size() - 1);
						glm::ivec4 light(
							x*roomSize + rnd->GetInt(1, roomSize - 1),
							y*roomSize + rnd->GetInt(1, roomSize - 1),
							z*roomSize + rnd->GetInt(1, roomSize - 1),
							w*roomSize + rnd->GetInt(1, roomSize - 1));

						switch (tmpWalls[randomEdgeIndex])
						{
						case NEG_X: light.x = x*roomSize + 1; break;
						case POS_X: light.x = (x + 1)*roomSize - 1; break;
						case NEG_Y: light.y = y*roomSize + 1; break;
						case POS_Y: light.y = (y + 1)*roomSize - 1; break;
						case NEG_Z: light.z = z*roomSize + 1; break;
						case POS_Z: light.z = (z + 1)*roomSize - 1; break;
						case NEG_W: light.w = w*roomSize + 1; break;
						case POS_W: light.w = (w + 1)*roomSize - 1; break;
						default:
							break;
						}

						if (maze->size.x == 1) light.x = roomSize / 2;
						if (maze->size.y == 1) light.y = roomSize / 2;
						if (maze->size.z == 1) light.z = roomSize / 2;
						if (maze->size.w == 1) light.w = roomSize / 2;

						generateLight(light);
					}

					tmpWalls.clear();
				}
			}
		}
	}
	
	for (int x = 0; x < size.x; x++)
	{
		for (int y = 0; y < size.y; y++)
		{
			for (int z = 0; z < size.z; z++)
			{
				for (int w = 0; w < size.w; w++)
				{
					for (int e = 0; e < EDGES_COUNT; e++)
					{
						int level = 15;
						unsigned int scaledLevel = 10;

						int index = GetIndex(x, y, z, w);
						if (index == 0)
							int b = 0;
						bool isMapEdge = false;
						if (x == 0 && e == NEG_X) isMapEdge = true;
						if (y == 0 && e == NEG_Y) isMapEdge = true;
						if (z == 0 && e == NEG_Z) isMapEdge = true;
						if (w == 0 && e == NEG_W) isMapEdge = true;
						if (x == size.x - 1 && e == POS_X) isMapEdge = true;
						if (y == size.y - 1 && e == POS_Y) isMapEdge = true;
						if (z == size.z - 1 && e == POS_Z) isMapEdge = true;
						if (w == size.w - 1 && e == POS_W) isMapEdge = true;

						if (isMapEdge)
						{
							// clear light
							curLightMap[index] &= ~((Texture::LIGHT_GRAD - 1) << (e * 4));
							// set light
							curLightMap[index]  |= scaledLevel << (e * 4);
						}

					}
				}
			}
		}
	}
	int i = 1;
}

void Field::CreateWinRoom()
{
	curMap = winMap;
	curLightMap = winLightMap;
	size = winMapSize;

	int mapSize = winMapSize.x*winMapSize.y*winMapSize.z*winMapSize.w;
	memset(curMap, 0, mapSize);

	// Generate walls
	for (int x = 0; x < winMapSize.x; x++)
	{
		for (int y = 0; y < winMapSize.y; y++)
		{
			for (int z = 0; z < winMapSize.z; z++)
			{
				for (int w = 0; w < winMapSize.w; w++)
				{
					int index = GetIndex(x, y, z, w);
					if (!(x > 0 && x < 8 &&
						y > 0 && y < 8 &&
						z > 0 && z < 8 &&
						w > 0 && w < 8))
					{
						winMap[index] = WALL_BLOCK;
					}
					else
					{
						winMap[index] = 0;
					}
					winLightMap[index] = 0;
				}
			}
		}
	}

	static const glm::ivec4 letters[] = {
		// W
		glm::ivec4(8,6,2,0), glm::ivec4(8,5,2,0), glm::ivec4(8,4,2,0), glm::ivec4(8,3,2,0),
		glm::ivec4(8,2,3,0), glm::ivec4(8,4,4,0), glm::ivec4(8,3,4,0), glm::ivec4(8,2,5,0),
		glm::ivec4(8,6,6,0), glm::ivec4(8,5,6,0), glm::ivec4(8,4,6,0), glm::ivec4(8,3,6,0),
		// I
		glm::ivec4(5,2,8,0), glm::ivec4(4,2,8,0), glm::ivec4(3,2,8,0), glm::ivec4(4,3,8,0),
		glm::ivec4(4,4,8,0), glm::ivec4(4,5,8,0), glm::ivec4(5,6,8,0), glm::ivec4(4,6,8,0),
		glm::ivec4(3,6,8,0),
		// N
		glm::ivec4(6,0,6,0), glm::ivec4(6,0,5,0), glm::ivec4(6,0,4,0), glm::ivec4(6,0,3,0),
		glm::ivec4(6,0,2,0), glm::ivec4(5,0,5,0), glm::ivec4(4,0,4,0), glm::ivec4(3,0,3,0),
		glm::ivec4(2,0,6,0), glm::ivec4(2,0,5,0), glm::ivec4(2,0,4,0), glm::ivec4(2,0,3,0),
		glm::ivec4(2,0,2,0)
	};
	for (int w = 0; w < winMapSize.w; w++)
	{
		for (int i = 0; i < sizeof(letters) / sizeof(letters[0]); i++)
		{
			winMap[GetIndex(letters[i].x, letters[i].y, letters[i].z, w)] = WALL_BLOCK;
			winLightMap[GetIndex(letters[i].x, letters[i].y, letters[i].z, w)] = UINT32_MAX;
		}
	}

	this->LoadMazeToGL(shader);
}

void Field::GenerateLightRecursive(int px, int py, int pz, int pw, unsigned int level, int side)
{
	if (level == 0 || !IsCubeIndexValid(px, py, pz, pw))
		return;

	unsigned int scaledLevel = int(level / float(lightDist) * (Texture::LIGHT_GRAD - 1));

	int index = GetIndex(px, py, pz, pw);

	if ((curMap[index] & WALL_BLOCK) != 0)
	{
		if (((curLightMap[index] >> (side * 4)) & (Texture::LIGHT_GRAD - 1)) < scaledLevel)
		{
			// clear light
			curLightMap[index] &= ~((Texture::LIGHT_GRAD - 1) << (side * 4));
			// set light
			curLightMap[index] |= scaledLevel << (side * 4);
			//curLightMap[index] = UINT32_MAX;
		}
		return;
	}

	if (curLightMap[index] >= level)
		return;
	curLightMap[index] = level;

	if (side != POS_X) GenerateLightRecursive(px + 1, py, pz, pw, level - 1, NEG_X);
	if (side != NEG_X) GenerateLightRecursive(px - 1, py, pz, pw, level - 1, POS_X);
	if (side != POS_Y) GenerateLightRecursive(px, py + 1, pz, pw, level - 1, NEG_Y);
	if (side != NEG_Y) GenerateLightRecursive(px, py - 1, pz, pw, level - 1, POS_Y);
	if (side != POS_Z) GenerateLightRecursive(px, py, pz + 1, pw, level - 1, NEG_Z);
	if (side != NEG_Z) GenerateLightRecursive(px, py, pz - 1, pw, level - 1, POS_Z);
	if (side != POS_W) GenerateLightRecursive(px, py, pz, pw + 1, level - 1, NEG_W);
	if (side != NEG_W) GenerateLightRecursive(px, py, pz, pw - 1, level - 1, POS_W);
}

int Field::GetIndex(const int x, const int y, const int z, const int w)
{
	assert(IsCubeIndexValid(x, y, z, w));
	return x*size.y*size.z*size.w + y*size.z*size.w + z*size.w + w;
};

bool Field::IsCubeIndexValid(int x, int y, int z, int w)
{
	return (
		x >= 0 && x < size.x &&
		y >= 0 && y < size.y &&
		z >= 0 && z < size.z &&
		w >= 0 && w < size.w);
};

Cell_t Field::GetCube(int x, int y, int z, int w)
{
	return curMap[GetIndex(x, y, z, w)];
};

void Field::LoadMazeToGL(Shader* shader)
{
	glUseProgram(shader->ID);

	GLint mapSizeLoc = glGetUniformLocation(shader->ID, "mapSize");
	glUniform4i(mapSizeLoc, size.x, size.y, size.z, size.w); // Texture unit 4 is for current map. 

	int texWxMax = ceil(std::pow(size.w, 1 / 3.0f)); //cube root
	int texWyMax = texWxMax;
	int texWzMax = texWyMax;	
	GLint MapWnAddedSizeLoc = glGetUniformLocation(shader->ID, "MapWnAddedSize");
	glUniform3i(MapWnAddedSizeLoc, texWxMax, texWyMax, texWzMax); 
	 

	int texSizeX = size.x * texWxMax;
	int texSizeY = size.y * texWyMax;
	int texSizeZ = size.z * texWzMax;
	GLint MapTexSizeLoc = glGetUniformLocation(shader->ID, "MapTexSize");
	glUniform3i(MapTexSizeLoc, texSizeX, texSizeY, texSizeZ); 

	//Convert 4-axis coordinates to 3-axis coordinates
	//Encode w coordinate equally across all xyz coords
	//returns INT value which encodes xyz point for 4d point
	auto GetTexIndex = [&](int x, int y, int z, int w)
	{
		//Assume w coordinate is index of 3 other coordinates
		//What are they if we have length for each of them?
		int texWz = w / (texWxMax*texWyMax);
		int texWy = (w % (texWxMax*texWyMax)) / texWxMax;
		int texWx = (w % (texWxMax*texWyMax)) % texWxMax;

		assert(w == texWz * texWxMax*texWyMax + texWy*texWxMax + texWx);

		int texX = texWx * size.x + x;
		int texY = texWy * size.y + y;
		int texZ = texWz * size.z + z;
		
		return 4 * (texZ * texSizeY * texSizeX + texY * texSizeX + texX);
	};

	//each index is vec4 float structure 
	//vec4.x - alpha channel for whole cube
	//vec4.y - texture type (1 for regular, 2 for light)
	//vec4.z - light level
	uint64_t totalSizeForTexture = texSizeZ * texSizeY * texSizeX + texSizeY * texSizeX + texSizeX; //GetTexIndex(size.x, size.y, size.z, size.w);
	uint8_t* curMapTexture = new uint8_t[4*totalSizeForTexture];
	uint8_t* curLightMapTexture = new uint8_t[4*totalSizeForTexture];
	//floatBitsToInt
	//uint64_t idx = 0;
	for (int x = 0; x < size.x; x++)
		for (int y = 0; y < size.y; y++)
			for (int z = 0; z < size.z; z++)
				for (int w = 0; w < size.w; w++)
				{
					int FieldIdx = GetIndex(x, y, z, w);
					int TexIdx = GetTexIndex(x, y, z, w);
					curMapTexture[TexIdx] = 0; //no block by default
					curMapTexture[TexIdx + 1] = 0; 
					curMapTexture[TexIdx + 2] = 0; 
					curMapTexture[TexIdx + 3] = 0;

					if ((curMap[FieldIdx] & WALL_BLOCK) != 0)
					{
						curMapTexture[TexIdx] = 255; // fully solid block
						curMapTexture[TexIdx + 1] = 10; //regular block type
					}
					if ((curMap[FieldIdx] & LIGHT_BLOCK) != 0)
					{
						curMapTexture[TexIdx] = 255; // fully solid block
						curMapTexture[TexIdx + 1] = 255; //light block type
					}

					Light_t light = curLightMap[FieldIdx];
					int LightLevelNegX = ((light >> (NEG_X * 4)) & (Texture::LIGHT_GRAD - 1));
					int LightLevelPosX = ((light >> (POS_X * 4)) & (Texture::LIGHT_GRAD - 1));
					int LightLevelNegY = ((light >> (NEG_Y * 4)) & (Texture::LIGHT_GRAD - 1));
					int LightLevelPosY = ((light >> (POS_Y * 4)) & (Texture::LIGHT_GRAD - 1));
					int LightLevelNegZ = ((light >> (NEG_Z * 4)) & (Texture::LIGHT_GRAD - 1));
					int LightLevelPosZ = ((light >> (POS_Z * 4)) & (Texture::LIGHT_GRAD - 1));
					int LightLevelNegW = ((light >> (NEG_W * 4)) & (Texture::LIGHT_GRAD - 1));
					int LightLevelPosW = ((light >> (POS_W * 4)) & (Texture::LIGHT_GRAD - 1));
					
					if (LightLevelPosX > 0)
						int i = 0;
					curLightMapTexture[TexIdx + 0] =  LightLevelPosX * 16 + LightLevelNegX;
					curLightMapTexture[TexIdx + 1] =  LightLevelPosY * 16 + LightLevelNegY;
					curLightMapTexture[TexIdx + 2] =  LightLevelPosZ * 16 + LightLevelNegZ;
					curLightMapTexture[TexIdx + 3] =  LightLevelPosW * 16 + LightLevelNegW;

				}

	
	int idx = GetTexIndex(9,4,4,11);
	idx = 0;
	curMapTexture[idx + 0] = 255;
	curMapTexture[idx + 1] = 10;
	curMapTexture[idx + 2] = 0;
	curMapTexture[idx + 3] = 0;

	GLuint newCurMapTextureId;
	glActiveTexture(GL_TEXTURE0 + 1);
	glGenTextures(1, &newCurMapTextureId);
	glBindTexture(GL_TEXTURE_3D, newCurMapTextureId);
	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); //GL_NEAREST GL_LINEAR
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	

	glUseProgram(shader->ID);
	GLint curMapId = glGetUniformLocation(shader->ID, "currentMap");
	glUniform1i(curMapId, 1); // Texture unit 4 is for current map.

	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, texSizeX, texSizeY, texSizeZ, 0, GL_RGBA, GL_UNSIGNED_BYTE, curMapTexture);


	GLuint newCurLightMapTextureId;
	glActiveTexture(GL_TEXTURE0 + 2);
	glGenTextures(1, &newCurLightMapTextureId);
	glBindTexture(GL_TEXTURE_3D, newCurLightMapTextureId);
	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	GLint curMapLightId = glGetUniformLocation(shader->ID, "currentLightMap");
	glUseProgram(shader->ID);
	glUniform1i(curMapLightId, 2); // Texture unit 4 is for current map.

	glActiveTexture(GL_TEXTURE0 + 2);
	glBindTexture(GL_TEXTURE_3D, newCurLightMapTextureId);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, texSizeX, texSizeY, texSizeZ, 0, GL_RGBA, GL_UNSIGNED_BYTE, curLightMapTexture);
	
	delete[] curMapTexture;
	delete[] curLightMapTexture;

}
