#include "BlockManager.h"
#include "BlockModel.h"
#include "BuildAbsolutePath.h"

#include <memory>
#include <vector>

bool BlockManager::registerBlock(const BlockDef& def) {
    // 1) State layout — bits allocate upward from bit 0 in declaration
    //    order; bit 15 stays reserved for the fluid flag.
    StateLayout layout;
    for (const StatePropDef& p : def.states) {
        if (!layout.addProperty(p.name, p.type)) {
            SDL_Log("[BlockManager] block '%s' skipped (bad state layout)",
                    def.name.c_str());
            return false;
        }
    }

    // 2) Model — bake every variant / multipart part now, on the main
    //    thread. The chunk-mesh workers only ever read the baked data.
    auto model = std::make_unique<BlockModel>(def.top, def.bottom, def.side);
    // "air" is never meshed (ChunkMesh skips id 0), so don't waste time
    // parsing an .obj for it.
    if (def.id != 0) {
        if (!model->bake(def, layout)) {
            SDL_Log("[BlockManager] block '%s' skipped (bake failed)",
                    def.name.c_str());
            return false;
        }
    }

    // Icon geometry: variant-0 .obj (multipart blocks use their first part).
    std::string iconObj = def.multipart && !def.parts.empty()
        ? def.parts[0].part
        : def.geometry;

    auto newBlock = std::make_unique<Block>(
        def.id,
        def.name,
        std::move(iconObj),
        std::move(model),
        std::move(layout),
        def.collision,
        def.transparent
    );
    // def.obstructs is no longer consumed — face occlusion is
    // derived from baked geometry (BakedMesh::coverMask).
    // The JSON field is still parsed and tolerated for older block files.
    Block* ptr = newBlock.get();

    blocksById[def.id] = ptr;
    blocksByName[def.name] = ptr;
    blocks.push_back(std::move(newBlock));
    return true;
}

void BlockManager::init() {
    // Templates are resolved against the block files at load time, in memory
    BlockDefLoader loader;
    std::vector<BlockDef> defs;
    if (!loader.loadAll(
            BuildAbsolutePath("Assets", "BlockTemplates"),
            BuildAbsolutePath("Assets", "Blocks"),
            defs)) {
        SDL_Log("[BlockManager] FATAL: could not load block definitions");
        return;
    }

    // defs arrive sorted by explicit id. Warn about gaps: ItemManager builds
    // placeable items by walking ids 1..N-1 and bails on the first hole.
    Uint16 expected = 0;
    for (const BlockDef& def : defs) {
        if (def.id != expected) {
            SDL_Log("[BlockManager] WARNING: block id gap — expected %u, got %u "
                    "('%s'). ItemManager expects contiguous ids!",
                    expected, def.id, def.name.c_str());
        }
        expected = def.id + 1;
    }

    for (const BlockDef& def : defs)
        registerBlock(def);

    SDL_Log("[BlockManager] registered %zu blocks from JSON", blocks.size());
}

Block* BlockManager::getById(Uint16 id) {
    auto it = blocksById.find(id);
    return it != blocksById.end() ? it->second : nullptr;
}

Collision* BlockManager::getCollissionById(Uint16 id) {
    Block* b = getById(id);
    if (!b) return nullptr;
    return &b->getCollision();   // address of a member of a Block that lives for the program
}

Block* BlockManager::getByName(const std::string& name) {
    auto it = blocksByName.find(name);
    return it != blocksByName.end() ? it->second : nullptr;
}

int BlockManager::getNumberOfBlocks() {
    return blocks.size();
}
