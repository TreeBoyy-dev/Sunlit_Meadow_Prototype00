#include "SurvivalUI.h"
#include "LoadTextureFromFile.h"
#include "BuildAbsolutePath.h"

#include <filesystem>
#include <fstream>
#include <cstdint>
#include <cctype>

static const char* kUITextureFolder = "Textures/UI/";

static bool readPngSize(const std::filesystem::path& file, int& outW, int& outH) {
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
}

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

        // loadTextureFromFile() builds its own absolute path from
        // (filePath, fileName), so pass the *relative* folder + the file name,
        // matching how the rest of the project calls its loaders.
        SDL_GPUTexture* tex = loadTextureFromFile(gpu, kUITextureFolder, fileName.c_str());
        if (!tex) {
            SDL_Log("[SurvivalUI] failed to load UI texture '%s'", fileName.c_str());
            allOk = false;
            continue;
        }

        UITexture uiTex;
        uiTex.texture = tex;
        readPngSize(p, uiTex.w, uiTex.h);

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

void drawHotbar(UI_Renderer* ui) {

    auto it = UITextureSet.find("hotbar_transperent");
    if (it == UITextureSet.end()) return;

    const UITexture& hb = it->second;

    constexpr float kHotbarScale = 3.0f;

    float w = static_cast<float>(hb.w) * kHotbarScale;
    float h = static_cast<float>(hb.h) * kHotbarScale;

    const float bottomMargin = 16.0f;
    float x = (ui->getScreenW() - w) * 0.5f;
    float y = ui->getScreenH() - h - bottomMargin;

    ui->drawTexture(hb.texture, x, y, w, h);
}