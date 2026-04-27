#include "Chunk.h"
#include "Globals.h"

Chunk::Chunk() :
	chunkCoordinates({0,0,0}),
	isGenerated(false),
	drawOpaqueMesh(false),
	drawTransparentMesh(false)
{}

Chunk::Chunk(ChunkCoord chunkCoordinates) :
	chunkCoordinates(chunkCoordinates),
	isGenerated(false),
	drawOpaqueMesh(false),
	drawTransparentMesh(false)
{}

bool Chunk::initMeshes(
	AppState* state,
	SDL_GPUTexture* textureArray
) {
	std::vector<LocationalBlockID> opaqueblocks;
	std::vector<LocationalBlockID> transparentblocks;

	for (int x = 0; x < CHUNK_SIZE; x++) {
		for (int y = 0; y < CHUNK_SIZE; y++) {
			for (int z = 0; z < CHUNK_SIZE; z++) {

				LocationalBlockID absLocationalBlockID = {
					x + chunkCoordinates.x * CHUNK_SIZE,
					y + chunkCoordinates.y * CHUNK_SIZE,
					z + chunkCoordinates.z * CHUNK_SIZE,
					blockIDs[x][y][z]
				};
				Block* block = blockManager.getById(blockIDs[x][y][z]);

				if (block == nullptr)
					SDL_Log("Block = nullptr in Chunk init meshes!!!");
				else if (!block->isTransparent())
					opaqueblocks.push_back(absLocationalBlockID);
				else if (block->getName() != "air")
					transparentblocks.push_back(absLocationalBlockID);
			}
		}
	}
	if (opaqueblocks.size() > 0) {
		drawOpaqueMesh = true;
		if (!opaqueMesh.init(
			state,
			opaqueblocks,
			textureArray
		)) {
			//return false;
			SDL_Log("failed to init opaqueMesh at %d|%d|%d",
				chunkCoordinates.x,
				chunkCoordinates.y,
				chunkCoordinates.z
			);
		}
	}
	if (transparentblocks.size() > 0) {
		drawTransparentMesh = true;
		if (!transparentMesh.init(
			state,
			transparentblocks,
			textureArray
		)) {
			//return false;
			SDL_Log("failed to init transparentMesh at %d|%d|%d",
				chunkCoordinates.x,
				chunkCoordinates.y,
				chunkCoordinates.z
			);
		}
	}

	return true;
}
bool Chunk::drawMeshes(
	AppState* state,
	SDL_GPUCommandBuffer* cmd,
	SDL_GPURenderPass* pass,
	const UBO& ubo
) {
	if(drawOpaqueMesh) opaqueMesh.draw(
		state,
		cmd,
		pass,
		ubo
	);
	if (drawTransparentMesh) transparentMesh.draw(
		state,
		cmd,
		pass,
		ubo
	);
	return true;
}
void Chunk::destroyMeshes(AppState* state) {
	opaqueMesh.destroy(state);
	transparentMesh.destroy(state);
}

bool Chunk::getIsGenerated() {
	return isGenerated;
}
ChunkCoord Chunk::getChunkCoordinates() {
	return chunkCoordinates;
}

bool Chunk::generateChunk() {

	//generate shape: air/stone
	generateShape(blockIDs, chunkCoordinates);

	//generate biomes

	//generate features: grass, vegitation, structures

	return true;
}

void Chunk::generateShape(Uint16 blockIDs[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE], ChunkCoord chunkCoordinates)
{
	for (int x = 0; x < CHUNK_SIZE; x++) {
		int xAbs = chunkCoordinates.x * CHUNK_SIZE + x;
		for (int y = 0; y < CHUNK_SIZE; y++) {
			int yAbs = chunkCoordinates.y * CHUNK_SIZE + y;
			for (int z = 0; z < CHUNK_SIZE; z++) {
				int zAbs = chunkCoordinates.z * CHUNK_SIZE + z;

				Block* block;
				if (zAbs <=2) {
					block = blockManager.getByName("cobble_stone");
				}
				else if (zAbs == 3) {
					if (rand() % 2 == 0)
						block = blockManager.getByName("cobble_stone");
					else
						block = blockManager.getByName("diorite");
				}
				else if (zAbs == 4) {
					int r = rand() % 3;

					if (r == 0)
						block = blockManager.getByName("cobble_stone");
					else if (r == 1)
						block = blockManager.getByName("diorite");
					else
						block = blockManager.getByName("dirt");
				}
				else if (zAbs > 20) {
					block = blockManager.getByName("diorite");
				}
				else {
					block = blockManager.getByName("air");
				}

				if (block != nullptr)
					blockIDs[x][y][z] = block->getID();
				else
					SDL_Log("Block = nullptr in Chunk generation!!!");
			}
		}
	}
}