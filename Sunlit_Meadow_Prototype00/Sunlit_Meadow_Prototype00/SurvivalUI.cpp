#include "SurvivalUI.h"
#include "LoadTextureFromFile.h"
#include "BuildAbsolutePath.h"
#include "Inventory.h"
#include "Globals.h"

#include <filesystem>
#include <fstream>
#include <cstdint>
#include <cctype>

static const char* kUITextureFolder = "Textures/UI/";

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

bool InitSurvivalUI(SDL_GPUDevice* gpu) {
    std::string folder = BuildAbsolutePath(kUITextureFolder, "");
    if (folder.empty()) {
        SDL_Log("[SurvivalUI] could not resolve UI texture folder");
        return false;
    }

    std::error_code ec;
    std::filesystem::directory_iterator dirIt(folder, ec);
    if (ec) {
        SDL_Log("[SurvivalUI] cannot open '%s': %s", folder.c_str(), ec.message().c_str());
        return false;
    }

    bool allOk = true;

    for (const auto& entry : dirIt) {
        if (!entry.is_regular_file()) continue;

        const std::filesystem::path& p = entry.path();

        // Only .png (case-insensitive).
        std::string ext = p.extension().string();
        for (char& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (ext != ".png") continue;

        std::string name = p.stem().string();     // "heart"     from "heart.png"
        std::string fileName = p.filename().string();  // "heart.png"

        GPUTextureWH gpuTextureWH;
        if (!loadTextureFromFile(&gpuTextureWH, gpu, kUITextureFolder, fileName.c_str()))
        {
            SDL_Log("[SurvivalUI] failed to load UI texture '%s'", fileName.c_str());
            allOk = false;
            continue;
        }
        SDL_GPUTexture* tex = gpuTextureWH.texture;

        UITexture uiTex;
        uiTex.texture = tex;
        uiTex.w = (int)gpuTextureWH.width;
        uiTex.h = (int)gpuTextureWH.height;

        auto [it, inserted] = UITextureSet.emplace(std::move(name), std::move(uiTex));
        if (!inserted) {
            SDL_Log("[SurvivalUI] duplicate UI texture name '%s' (kept first)",
                it->first.c_str());
        }
    }

    SDL_Log("[SurvivalUI] loaded %zu UI texture(s) from '%s'",
        UITextureSet.size(), kUITextureFolder);
    return allOk;
}

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

        ui->drawItemModel(model, x, y, itemSize, itemSize, pitch, yaw, roll);
    }
}

void drawHotbar(UI_Renderer* ui) {

    auto it = UITextureSet.find("hotbar_transperent");
    if (it == UITextureSet.end()) return;

    const UITexture& hb = it->second;

    constexpr float kHotbarScale = 5.0f;

    float w = static_cast<float>(hb.w) * kHotbarScale;
    float h = static_cast<float>(hb.h) * kHotbarScale;

    const float bottomMargin = 16.0f;
    float x = (ui->getScreenW() - w) * 0.5f;
    float y = ui->getScreenH() - h - bottomMargin;

    ui->drawTexture(hb.texture, x, y, w, h);
    drawInventoryHotbar(ui, x, y, kHotbarScale);
}