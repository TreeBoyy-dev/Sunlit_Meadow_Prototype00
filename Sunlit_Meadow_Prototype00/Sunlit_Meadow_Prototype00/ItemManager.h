#pragma once
#include <SDL3/SDL.h>

#include <vector>
#include <unordered_map>
#include <memory>
#include <string>

#include "Item.h"

class ItemManager {
private:
    std::vector<std::unique_ptr<Item>>     items;
    std::unordered_map<std::string, Item*> itemsByName;
    std::unordered_map<Uint16, Item*>      itemsById;

    Uint16 nextId = 0;

    SDL_GPUDevice* gpu = nullptr;

    // Internal: assign an id, load + attach the icon, run initMesh (so a
    // placeable can resolve its block), then store the item in both maps.
    // Takes ownership of `item`. iconPath/iconFile may be null for no icon.
    Item* registerItem(
        std::unique_ptr<Item> item,
        SDL_GPUDevice* gpu,
        const char* textureFile,
        const char* modelFile,
        BlockManager* blockManager
    );
    void registerPlacableItems(SDL_GPUDevice* gpuDevice, BlockManager* blockManager);

public:
    void initAssets(SDL_GPUDevice* gpuDevice, BlockManager* blockManager);
    // Release the icon textures this manager owns.
    void destroy(SDL_GPUDevice* gpuDevice);

    void drawItem(
        UI_Renderer* ui,
        const std::string name,
        float panelX, float panelY, float panelW, float panelH,
        float pitch, float yaw, float roll,
        float scale = 1.0f,
        SDL_FColor tint = { 1.0f, 1.0f, 1.0f, 1.0f },
        bool cullBackFaces = true
    );

    Item* getItemByName(const std::string& name);
    Item* getItemById(Uint16 id);
};