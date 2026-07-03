#include "SurvivalUI.h"
#include "LoadTextureFromFile.h"
#include "BuildAbsolutePath.h"
#include "Inventory.h"
#include "Globals.h"

#include <filesystem>
#include <fstream>
#include <cstdint>
#include <cctype>


/*static bool readPngSize(const std::filesystem::path& file, int& outW, int& outH) {
    std::ifstream in(file, std::ios::binary);
    if (!in) return false;

    unsigned char header[24];
    in.read(reinterpret_cast<char*>(header), sizeof(header));
    if (in.gcount() < static_cast<std::streamsize>(sizeof(header))) return false;

    // 8-byte PNG signature, then the IHDR chunk:
    // width is a big-endian uint32 at byte 16, height at byte 20.
    static const unsigned char sig[8] = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };
    for (int i = 0; i < 8; ++i)
        if (header[i] != sig[i]) return false;

    auto be32 = [](const unsigned char* p) -> uint32_t {
        return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) |
            (uint32_t(p[2]) << 8) | uint32_t(p[3]);
        };
    outW = static_cast<int>(be32(&header[16]));
    outH = static_cast<int>(be32(&header[20]));
    return true;
}*/

void drawSurvivalUI(UI_Renderer* ui) {
	drawHotbar(ui);
}

void drawInventoryHotbar(
    UI_Renderer* ui,
    const int originX,
    const int originY,
    float multiplier
) {
    Entity* player = entityManager.getEntityById(0);
    if (player == nullptr) {            // getEntityById can return null
        SDL_Log("no player entity (id 0)");
        return;
    }

    Inventory* inventory;
    Data* found = player->getData(INVENTORY);
    if (found == nullptr) {
        //SDL_Log("no Inventory found");
        return;
    }
    else
        inventory = static_cast<Inventory*>(found);

    const int offsetX = 17;

    const float itemSize = 16.0f * multiplier;
    const float stepX = offsetX * multiplier;

    constexpr float kBlockPitch = 0.79f;
    constexpr float kBlockYaw = 0.91f;
    constexpr float kBlockRoll = -2.43; //0.68f;

    constexpr float kItemPitch = 1.6f;
    constexpr float kItemYaw = 1.6f;
    constexpr float kItemRoll = 0.0f;

    for (int i = 0; i < 10; i++) {
        ItemInstance instance = inventory->getItemsFromSlot(i);
        if (instance.isEmpty())
            continue;

        ItemModel* model = instance.item->getModel();
        if (model == nullptr || model->isEmpty())
            continue;

        const bool isBlock = (instance.item->getCategory() == ITEM_CATEGORY_BLOCK);
        const float pitch = isBlock ? kBlockPitch : kItemPitch;
        const float yaw = isBlock ? kBlockYaw : kItemYaw;
        const float roll = isBlock ? kBlockRoll : kItemRoll;

        const float x = static_cast<float>(originX) + stepX * i;
        const float y = static_cast<float>(originY);

        char buffer[256];
        const SDL_FColor white = { 1.0f, 1.0f, 1.0f, 1.0f };

        snprintf(buffer, sizeof(buffer), "%d", instance.count);
        ui->drawText(buffer, x+8, y+12, white);

        ui->drawItemModel(model, x, y, itemSize, itemSize, pitch, yaw, roll);
    }
}

void drawHotbar(UI_Renderer* ui) {

    UITexture* hb = ui->FindUITexture("hotbar_transperent");

    constexpr float kHotbarScale = 5.0f;

    float w = static_cast<float>(hb->w) * kHotbarScale;
    float h = static_cast<float>(hb->h) * kHotbarScale;

    const float bottomMargin = 16.0f;
    float x = (ui->getScreenW() - w) * 0.5f;
    float y = ui->getScreenH() - h - bottomMargin;

    ui->drawTexture(hb->texture, x, y, w, h);
    drawInventoryHotbar(ui, x, y, kHotbarScale);
}