#include "Item.h"

#include "BlockManager.h"
#include "Block.h"
#include "UI_Renderer.h"

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

void Item::initMesh(BlockManager* blockManager) {
    // Plain items have no block-derived geometry. Their icon is uploaded by the
    // ItemManager and handed over via setIcon(), so there is nothing to build
    // here. Kept virtual so Item_Placable (and future item types) can hook in.
}

void Item::drawModelAt(UI_Renderer* ui, float x, float y, float size) {
    if (!ui) return;

    if (icon) {
        ui->drawTexture(icon, x, y, size, size);
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

void Item_Placable::initMesh(BlockManager* blockManager) {
    if (!blockManager) {
        SDL_Log("Item_Placable '%s': no BlockManager passed to initMesh", name.c_str());
        return;
    }

    block = blockManager->getById(blockId);
    if (!block) {
        SDL_Log("Item_Placable '%s': no block with id %u", name.c_str(), (unsigned)blockId);
        return;
    }

}