#include "ItemManager.h"

#include "BlockManager.h"
#include "Block.h"
#include "LoadTextureFromFile.h"

// Where item icon PNGs live, relative to the same base BuildAbsolutePath() uses.
static const char* baseItemIconPath = "Textures/Items";

SDL_GPUTexture* ItemManager::getIconFromFile(
    const char* iconPath,
    const char* iconFile
) {
    if (gpu && iconPath && iconFile) {
        GPUTextureWH gpuTextureWH;
        if (loadTextureFromFile(&gpuTextureWH, gpu, iconPath, iconFile)) {
            ownedIcons.push_back(gpuTextureWH.texture);
            return gpuTextureWH.texture;
        }
        else {
            SDL_Log("[ItemManager]: failed to load icon '%s'", iconFile);
        }
    }
    else
        SDL_Log("[ItemManager]: failed to load icon '%s' - no gpu, file or path", iconFile);
}

Item* ItemManager::registerItem(
    std::unique_ptr<Item> item,
    SDL_GPUTexture* icon,
    BlockManager* blockManager
) {
    Uint16 id = nextId++;
    item->setID(id);

    // Let the item resolve any block-derived data (Item_Placable caches its block).
    item->initMesh(blockManager);

    Item* ptr = item.get();
    itemsById[id] = ptr;
    itemsByName[ptr->getName()] = ptr;
    items.push_back(std::move(item));
    return ptr;
}

void ItemManager::initAssets(SDL_GPUDevice* gpuDevice, BlockManager* blockManager) {
    gpu = gpuDevice;   // cached for registerItem's icon uploads

    registerPlacableItems(gpuDevice, blockManager);

    // --- plain items --------------------------------------------------------
    registerItem(std::make_unique<Item>(
        "stick", ITEM_CATEGORY_MATERIAL, 0.1f, 64),
        getIconFromFile(baseItemIconPath, "stick.png"),
        blockManager
    );
    registerItem(std::make_unique<Item>(
        "apple", ITEM_CATEGORY_FOOD, 0.2f, 16),
        getIconFromFile(baseItemIconPath, "apple.png"),
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
            registerItem(std::make_unique<Item_Placable>(
                b->getName(), b->getID()), icon, blockManager
            );
            ownedIcons.push_back(icon);
        }
        else
            SDL_Log("[ItemManager]: no icon from block '%s'", b->getName());
    }
}

void ItemManager::destroy(SDL_GPUDevice* gpuDevice) {
    if (!gpuDevice) return;
    for (SDL_GPUTexture* tex : ownedIcons)
        if (tex) SDL_ReleaseGPUTexture(gpuDevice, tex);
    ownedIcons.clear();
}

Item* ItemManager::getItemByName(const std::string& name) {
    auto it = itemsByName.find(name);
    return it != itemsByName.end() ? it->second : nullptr;
}

Item* ItemManager::getItemById(Uint16 id) {
    auto it = itemsById.find(id);
    return it != itemsById.end() ? it->second : nullptr;
}