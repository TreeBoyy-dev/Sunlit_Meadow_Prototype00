#include "BlockManager.h"

void BlockManager::initBlockManager() {

	//for each block in the game:
	// - initialize the block with its respective data like model, name, unique id etc. 
	//		(model hold vertecies, indecies, textures, so everthing to display the block)
	// - add the block to a list of blocks

	//after all blocks are initialized:
	// - iterate through list:
	//   > create a block for each alternative varients for every block that has one
	//	   for example: if(block.hasSLab) newBlock = Slab(data of initial block);

}