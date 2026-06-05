#include "ItemManager.h"

#include "BlockManager.h"
#include "Block.h"
#include "LoadTextureFromFile.h"

static const char* baseTexturePath = "Textures/Items";
static const char* baseModelPath = "Models";

Item* ItemManager::registerItem(
    std::unique_ptr<Item> item,
    SDL_GPUDevice* gpu,
    const char* textureFile,
    const char* modelFile,
    BlockManager* blockManager
) {
    Uint16 id = nextId++;
    item->setID(id);

    // Let the item resolve any block-derived data (Item_Placable caches its block).
    item->initModel(
        gpu,
        baseTexturePath,
        textureFile,
        baseModelPath,
        modelFile,
        blockManager
    );

    Item* ptr = item.get();
    itemsById[id] = ptr;
    itemsByName[ptr->getName()] = ptr;
    items.push_back(std::move(item));
    return ptr;
}

void ItemManager::initAssets(SDL_GPUDevice* gpuDevice, BlockManager* blockManager) {
    gpu = gpuDevice;   // cached for registerItem's icon uploads

    registerPlacableItems(gpu, blockManager);

    // --- plain items --------------------------------------------------------
    registerItem(
        std::make_unique<Item>("stick", ITEM_CATEGORY_MATERIAL, 0.1f, 64),
        gpu,
        "stick.png", "stick.obj",
        blockManager
    );

    SDL_Log("[ItemManager]: registered %zu items", items.size());
}

void ItemManager::registerPlacableItems(SDL_GPUDevice* gpuDevice, BlockManager* blockManager) {

    int numberOfBlocks = blockManager->getNumberOfBlocks();
    for (int i = 0; i < numberOfBlocks; i++)
    {
        Block* b = blockManager ? blockManager->getById(i) : nullptr;
        if (!b) {
            SDL_Log("[ItemManager]: no block found to build a placeable item from: id: %d", i);
            return;
        }
        SDL_GPUTexture* icon = b->getIcon();
        if (!icon)
        {
            std::unique_ptr<Item> item = std::make_unique<Item_Placable>(b->getName(), b->getID());

            Uint16 id = nextId++;
            item->setID(id);

            item->initModel(gpu, "", "", "", "", blockManager);

            Item* ptr = item.get();
            itemsById[id] = ptr;
            itemsByName[ptr->getName()] = ptr;
            items.push_back(std::move(item));
        }
        else
            SDL_Log("[ItemManager]: no icon from block '%s'", b->getName());
    }
}

void ItemManager::destroy(SDL_GPUDevice* gpuDevice) {
    if (!gpuDevice) return;
}

void ItemManager::drawItem(
    UI_Renderer* ui,
    const std::string name,
    float panelX, float panelY, float panelW, float panelH,
    float pitch, float yaw, float roll,
    float scale,
    SDL_FColor tint,
    bool cullBackFaces)
{
    Item* item = getItemByName(name);
    if (item == nullptr) {
        SDL_Log("[ItemManager] no item returned from item '%s'", name.c_str());
        return;
    }
    ItemModel* model = item->getModel();
    if (model == nullptr) {
        SDL_Log("[ItemManager] no model returned from item '%s'", item->getName().c_str());
        return;
    }

    ui->drawItemModel(
        model,
        panelX, panelY, panelW, panelH,
        pitch, yaw, roll,
        scale,
        tint,
        cullBackFaces);
}

Item* ItemManager::getItemByName(const std::string& name) {
    auto it = itemsByName.find(name);
    return it != itemsByName.end() ? it->second : nullptr;
}

Item* ItemManager::getItemById(Uint16 id) {
    auto it = itemsById.find(id);
    return it != itemsById.end() ? it->second : nullptr;
}