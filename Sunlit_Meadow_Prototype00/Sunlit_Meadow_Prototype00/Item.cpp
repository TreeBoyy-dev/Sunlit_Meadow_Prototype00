#include "Item.h"

#include "BlockManager.h"
#include "Block.h"
#include "UI_Renderer.h"
#include "ObjParser.h"
#include "LoadTextureFromFile.h"

// ----------------------------------------------------------------------------
// Item
// ----------------------------------------------------------------------------
Item::Item(
    std::string  name,
    ItemCategory category,
    float        weight,
    short int    maxStackSize
)
    : name(std::move(name))
    , category(category)
    , weight(weight)
    , maxStackSize(maxStackSize)
{
}

void Item::initModel(
    SDL_GPUDevice* gpu,
    const char* texturePath,
    const char* textureFile,
    const char* modelPath,
    const char* modelFile,
    BlockManager* blockManager)
{
    std::vector<ModelVertex> outVertices;
    std::vector<Uint16> outIndices;

    if (!obj_parse(
        BuildAbsolutePath(modelPath, modelFile),
        outVertices,
        outIndices
    ))
        SDL_Log("[Item] failed to load Model '%s'", modelFile);

    model.setMesh(outVertices, outIndices);

    GPUTextureWH gpuTextureWH;
    if (!loadTextureFromFile(
        &gpuTextureWH,
        gpu,
        texturePath,
        textureFile
    ))
        SDL_Log("[Item] failed to load Texture '%s'", textureFile);
    

    model.setTexture(gpuTextureWH.texture);
}

void Item::drawModelAt(UI_Renderer* ui, float x, float y, float size) {
    if (!ui) return;

    if (!model.isEmpty()) {
        ui->drawItemModel(&model, x, y, 50, 50, 1.6, 1.6, 0);
    }
    else {
        ui->drawRect(x, y, size, size, 1.0f, 0.0f, 1.0f, 1.0f);
    }
}

// ----------------------------------------------------------------------------
// Item_Placable
// ----------------------------------------------------------------------------
Item_Placable::Item_Placable(
    std::string name,
    Uint16      blockId,
    float       weight,
    short int   maxStackSize
)
    : Item(std::move(name), ITEM_CATEGORY_BLOCK, weight, maxStackSize)
    , blockId(blockId)
{
}

void Item_Placable::initModel(
    SDL_GPUDevice* gpu,
    const char* texturePath,
    const char* textureFile,
    const char* modelPath,
    const char* modelFile,
    BlockManager* blockManager
) {
    if (!blockManager) {
        SDL_Log("Item_Placable '%s': no BlockManager passed to initMesh", name.c_str());
        return;
    }

    block = blockManager->getById(blockId);
    if (!block) {
        SDL_Log("Item_Placable '%s': no block with id %u", name.c_str(), (unsigned)blockId);
        return;
    }

    // A placeable item ignores the texture/model parameters above and derives
    // everything from its block. Blocks live under "Textures/Blocks".
    static const char* blockTexturePath = "Textures/Blocks";

    // --- geometry -----------------------------------------------------------
    // The block hands back a ModelVertex mesh whose UVs are remapped into a
    // 3-cell [ side | top | bottom ] atlas. obj_parse (inside buildItemModel)
    // bakes MODEL_ROTATION, so the icon shares the regular items' orientation
    // space (the offscreen camera looks down -Z; the tilt you described is the
    // pitch/yaw/roll you pass to drawItem). ItemModel::computeBounds recenters
    // the mesh afterwards.
    std::vector<ModelVertex> verts;
    std::vector<Uint16>      indices;
    if (!block->buildItemModel(verts, indices)) {
        SDL_Log("[Item_Placable] '%s': buildItemModel failed", name.c_str());
        return;
    }
    model.setMesh(verts, indices);

    // --- texture ------------------------------------------------------------
    // Build the matching atlas from the block's three materials. Cell order
    // here must mirror Block::buildItemModel: side, then top, then bottom.
    // A null top/bottom file (e.g. MATERIAL_AIR) just reuses the side image.
    const char* sideFile =   materialTextureFile(block->getSideMaterial().material);
    const char* topFile =    materialTextureFile(block->getTopMaterial().material);
    const char* bottomFile = materialTextureFile(block->getBottomMaterial().material);

    if (!sideFile) {
        // No usable texture at all (e.g. an "air" placeable). Leave the model
        // textureless; drawModelAt falls back to its magenta placeholder.
        SDL_Log("[Item_Placable] '%s': block has no side texture, skipping", name.c_str());
        return;
    }

    GPUTextureWH atlas;
    if (!buildBlockIconTexture(&atlas, gpu, blockTexturePath, sideFile, topFile, bottomFile)) {
        SDL_Log("[Item_Placable] '%s': failed to build block icon atlas", name.c_str());
        return;
    }

    model.setTexture(atlas.texture);
}