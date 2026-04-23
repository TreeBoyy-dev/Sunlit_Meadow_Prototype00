#pragma once

#include "Block.h"
#include "BlockMesh.h"

class Chunk {
private:
	Vec3 chunkCoordinates;
	Block blocks[16][16][16];
	//Biome* biome;
	//Zone* zone;
	//Layer* layer;

	BlockMesh opaqueMesh;
	BlockMesh transparentMesh;
};

//is chunk in renderdistance??:
//cube of renderdistance radius(?)
//for each cunk in cube calc distance to player
//each chunk that is within distance and not in the list of chunkCordinates is put in a list of rendered chunks