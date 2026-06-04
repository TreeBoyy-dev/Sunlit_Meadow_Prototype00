#pragma once
#include <SDL3/SDL.h>
#include <string>

#include "BlockManager.h"
#include "UI_Renderer.h"
#include "ItemModel.h"

class Item;

enum ItemCategory {
    ITEM_CATEGORY_MISC,
    ITEM_CATEGORY_BLOCK,
    ITEM_CATEGORY_TOOL,
    ITEM_CATEGORY_FOOD,
    ITEM_CATEGORY_MATERIAL,

    ITEM_CATEGORY_COUNT     // ALWAYS LAST!!
};

struct ItemInstance {
    Item* item = nullptr;
    short int count = 0;

    bool isEmpty() const { return item == nullptr || count <= 0; }
};

class Item {
protected:
    Uint16       id = 0;
    std::string  name;
    ItemCategory category = ITEM_CATEGORY_MISC;

    float        weight = 0.0f;
    short int    maxStackSize = 64;

    ItemModel model;

public:
    Item(
        std::string  name,
        ItemCategory category = ITEM_CATEGORY_MISC,
        float        weight = 0.2f,
        short int    maxStackSize = 200
    );
    virtual ~Item() = default;

    // Prepare this item's visual.
    virtual void initModel(
        SDL_GPUDevice* gpu,
        const char* texturePath,
        const char* textureFile,
        const char* modelPath,
        const char* modelFile,
        BlockManager* blockManager = nullptr);

    // Draw the item's icon as a square at pixel (x, y) with side length `size`.
    void drawModelAt(UI_Renderer* ui, float x, float y, float size);

    // --- accessors ---
    Uint16             getID()           const { return id; }
    const std::string& getName()         const { return name; }
    ItemCategory       getCategory()     const { return category; }
    float              getWeight()       const { return weight; }
    short int          getMaxStackSize() const { return maxStackSize; }

    void setID(Uint16 newId) { id = newId; }
};

class Item_Placable : public Item {
private:
    Uint16 blockId = 0;
    Block* block = nullptr;

public:
    Item_Placable(
        std::string name,
        Uint16      blockId,
        float       weight = 0.5f,
        short int   maxStackSize = 100
    );

    virtual void initModel(
        SDL_GPUDevice* gpu,
        const char* texturePath,
        const char* textureFile,
        const char* modelPath,
        const char* modelFile,
        BlockManager* blockManager = nullptr
    ) override;

    Uint16 getBlockId() const { return blockId; }
    Block* getBlock()   const { return block; }
};