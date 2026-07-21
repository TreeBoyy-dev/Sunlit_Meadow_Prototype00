#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "Materials.h"

#include <unordered_map>

const char* baseTexturePathMaterials = "Textures/Blocks/";

bool UploadTextureArrayLayer(
    SDL_GPUDevice* gpu,
    SDL_GPUTexture* textureArray,
    const char* filePath,
    const char* fileName,
    Material material
)
{
    if (!gpu || !textureArray || !filePath || !fileName) {
        SDL_Log("UploadTextureArrayLayer: invalid argument");
        return false;
    }

    std::string fullPath = BuildAbsolutePath(filePath, fileName);

    SDL_Surface* loadedSurface = SDL_LoadSurface(fullPath.c_str());
    if (!loadedSurface) {
        SDL_Log("SDL_LoadSurface failed for '%s', loading default texture", fileName);// fullPath.c_str());//, SDL_GetError());

        loadedSurface = SDL_LoadSurface(BuildAbsolutePath(filePath, "default_texture.png").c_str());

        if (!loadedSurface) {
            SDL_Log("failed to load default_texture");
            return false;
        }
    }

    SDL_Surface* surface = SDL_ConvertSurface(loadedSurface, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(loadedSurface);

    if (!surface) {
        SDL_Log("SDL_ConvertSurface failed for '%s': %s", fullPath.c_str(), SDL_GetError());
        return false;
    }

    const Uint32 width = (Uint32)surface->w;
    const Uint32 height = (Uint32)surface->h;
    const Uint32 bytesPerPixel = 4;
    const Uint32 dataSize = width * height * bytesPerPixel;

    SDL_GPUTransferBufferCreateInfo transferInfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = dataSize
    };

    SDL_GPUTransferBuffer* transferBuffer =
        SDL_CreateGPUTransferBuffer(gpu, &transferInfo);

    if (!transferBuffer) {
        SDL_Log("SDL_CreateGPUTransferBuffer failed: %s", SDL_GetError());
        SDL_DestroySurface(surface);
        return false;
    }

    void* mapped = SDL_MapGPUTransferBuffer(gpu, transferBuffer, false);
    if (!mapped) {
        SDL_Log("SDL_MapGPUTransferBuffer failed: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(gpu, transferBuffer);
        SDL_DestroySurface(surface);
        return false;
    }

    SDL_memcpy(mapped, surface->pixels, dataSize);
    SDL_UnmapGPUTransferBuffer(gpu, transferBuffer);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(gpu);
    if (!cmd) {
        SDL_Log("SDL_AcquireGPUCommandBuffer failed: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(gpu, transferBuffer);
        SDL_DestroySurface(surface);
        return false;
    }

    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTextureTransferInfo src = {
        .transfer_buffer = transferBuffer,
        .offset = 0,
        .pixels_per_row = width,
        .rows_per_layer = height
    };

    SDL_GPUTextureRegion dst = {
        .texture = textureArray,
        .mip_level = 0,
        .layer = (Uint32)material,
        .x = 0,
        .y = 0,
        .z = 0,
        .w = width,
        .h = height,
        .d = 1
    };

    SDL_UploadToGPUTexture(copyPass, &src, &dst, false);

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(cmd);

    SDL_ReleaseGPUTransferBuffer(gpu, transferBuffer);
    SDL_DestroySurface(surface);

    return true;
}

const char* materialTextureFile(Material material) {
    switch (material) {
    case MATERIAL_COBBLESTONE:        return "cobblestone.png";
    case MATERIAL_DIORITE:            return "diorite.png";
    case MATERIAL_DIRT:               return "dirt.png";
    case MATERIAL_GRASS_BLOCK_TOP:    return "grass_block_top.png";
    case MATERIAL_GRASS_BLOCK_SIDE:   return "grass_block_side.png";
    case MATERIAL_GRASS_BLOCK_SIDE_OV:return "grass_block_side_overlay.png";
    case MATERIAL_BIRCH_LOG_SIDE:     return "birch_log_side.png";
    case MATERIAL_BIRCH_LOG_TOP:      return "birch_log_top.png";
    case MATERIAL_BIRCH_LEAVES:       return "birch_leaves.png";
    case MATERIAL_CHESTNUT_LOG_SIDE:  return "chestnut_log_side.png";
    case MATERIAL_CHESTNUT_LOG_TOP:   return "chestnut_log_top.png";
    case MATERIAL_CHESTNUT_LEAVES:    return "chestnut_leaves.png";
        //savanna: eucalyptus
    case MATERIAL_EUCALYPTUS_LOG_SIDE:return "eucalyptus_log_side.png";
    case MATERIAL_EUCALYPTUS_LOG_TOP: return "eucalyptus_log_top.png";
    case MATERIAL_EUCALYPTUS_STRIPPED_LOG_SIDE: return "eucalyptus_stripped_log_side.png";
    case MATERIAL_EUCALYPTUS_STRIPPED_LOG_TOP: return "eucalyptus_stripped_log_top.png";
    case MATERIAL_EUCALYPTUS_PLANKS:  return "eucalyptus_planks.png";
    case MATERIAL_EUCALYPTUS_LEAVES:  return "eucalyptus_leaves.png";
        //baobab
    case MATERIAL_BAOBAB_LOG_SIDE:    return "baobab_log_side.png";
    case MATERIAL_BAOBAB_LOG_TOP:     return "baobab_log_top.png";
    case MATERIAL_BAOBAB_STRIPPED_LOG_SIDE: return "baobab_stripped_log_side.png";
    case MATERIAL_BAOBAB_STRIPPED_LOG_TOP: return "baobab_stripped_log_top.png";
    case MATERIAL_BAOBAB_PLANKS:      return "baobab_planks.png";
    case MATERIAL_BAOBAB_LEAVES:      return "baobab_leaves.png";
        //acacia
    case MATERIAL_ACACIA_LOG_SIDE:    return "acacia_log_side.png";
    case MATERIAL_ACACIA_LOG_TOP:     return "acacia_log_top.png";
    case MATERIAL_ACACIA_STRIPPED_LOG_SIDE: return "acacia_stripped_log_side.png";
    case MATERIAL_ACACIA_STRIPPED_LOG_TOP: return "acacia_stripped_log_top.png";
    case MATERIAL_ACACIA_PLANKS:      return "acacia_planks.png";
    case MATERIAL_ACACIA_LEAVES:      return "acacia_leaves.png";
        //oak
    case MATERIAL_OAK_LOG_SIDE:       return "oak_log_side.png";
    case MATERIAL_OAK_LOG_TOP:        return "oak_log_top.png";
    case MATERIAL_OAK_STRIPPED_LOG_SIDE: return "oak_stripped_log_side.png";
    case MATERIAL_OAK_STRIPPED_LOG_TOP: return "oak_stripped_log_top.png";
    case MATERIAL_OAK_PLANKS:         return "oak_planks.png";
    case MATERIAL_OAK_LEAVES:         return "oak_leaves.png";
        //dark_oak
    case MATERIAL_DARK_OAK_LOG_SIDE:  return "dark_oak_log_side.png";
    case MATERIAL_DARK_OAK_LOG_TOP:   return "dark_oak_log_top.png";
    case MATERIAL_DARK_OAK_STRIPPED_LOG_SIDE: return "dark_oak_stripped_log_side.png";
    case MATERIAL_DARK_OAK_STRIPPED_LOG_TOP: return "dark_oak_stripped_log_top.png";
    case MATERIAL_DARK_OAK_PLANKS:    return "dark_oak_planks.png";
    case MATERIAL_DARK_OAK_LEAVES:    return "dark_oak_leaves.png";
        //maple
    case MATERIAL_MAPLE_LOG_SIDE:     return "maple_log_side.png";
    case MATERIAL_MAPLE_LOG_TOP:      return "maple_log_top.png";
    case MATERIAL_MAPLE_STRIPPED_LOG_SIDE: return "maple_stripped_log_side.png";
    case MATERIAL_MAPLE_STRIPPED_LOG_TOP: return "maple_stripped_log_top.png";
    case MATERIAL_MAPLE_PLANKS:       return "maple_planks.png";
    case MATERIAL_MAPLE_LEAVES:       return "maple_leaves.png";
        //beech
    case MATERIAL_BEECH_LOG_SIDE:     return "beech_log_side.png";
    case MATERIAL_BEECH_LOG_TOP:      return "beech_log_top.png";
    case MATERIAL_BEECH_STRIPPED_LOG_SIDE: return "beech_stripped_log_side.png";
    case MATERIAL_BEECH_STRIPPED_LOG_TOP: return "beech_stripped_log_top.png";
    case MATERIAL_BEECH_PLANKS:       return "beech_planks.png";
    case MATERIAL_BEECH_LEAVES:       return "beech_leaves.png";
    case MATERIAL_BIRCH_STRIPPED_LOG_SIDE: return "birch_stripped_log_side.png";
    case MATERIAL_BIRCH_STRIPPED_LOG_TOP: return "birch_stripped_log_top.png";
    case MATERIAL_BIRCH_PLANKS:       return "birch_planks.png";
        //hickory
    case MATERIAL_HICKORY_LOG_SIDE:   return "hickory_log_side.png";
    case MATERIAL_HICKORY_LOG_TOP:    return "hickory_log_top.png";
    case MATERIAL_HICKORY_STRIPPED_LOG_SIDE: return "hickory_stripped_log_side.png";
    case MATERIAL_HICKORY_STRIPPED_LOG_TOP: return "hickory_stripped_log_top.png";
    case MATERIAL_HICKORY_PLANKS:     return "hickory_planks.png";
    case MATERIAL_HICKORY_LEAVES:     return "hickory_leaves.png";
        //spruce
    case MATERIAL_SPRUCE_LOG_SIDE:    return "spruce_log_side.png";
    case MATERIAL_SPRUCE_LOG_TOP:     return "spruce_log_top.png";
    case MATERIAL_SPRUCE_STRIPPED_LOG_SIDE: return "spruce_stripped_log_side.png";
    case MATERIAL_SPRUCE_STRIPPED_LOG_TOP: return "spruce_stripped_log_top.png";
    case MATERIAL_SPRUCE_PLANKS:      return "spruce_planks.png";
    case MATERIAL_SPRUCE_LEAVES:      return "spruce_leaves.png";
        //pine
    case MATERIAL_PINE_LOG_SIDE:      return "pine_log_side.png";
    case MATERIAL_PINE_LOG_TOP:       return "pine_log_top.png";
    case MATERIAL_PINE_STRIPPED_LOG_SIDE: return "pine_stripped_log_side.png";
    case MATERIAL_PINE_STRIPPED_LOG_TOP: return "pine_stripped_log_top.png";
    case MATERIAL_PINE_PLANKS:        return "pine_planks.png";
    case MATERIAL_PINE_LEAVES:        return "pine_leaves.png";
        //fir
    case MATERIAL_FIR_LOG_SIDE:       return "fir_log_side.png";
    case MATERIAL_FIR_LOG_TOP:        return "fir_log_top.png";
    case MATERIAL_FIR_STRIPPED_LOG_SIDE: return "fir_stripped_log_side.png";
    case MATERIAL_FIR_STRIPPED_LOG_TOP: return "fir_stripped_log_top.png";
    case MATERIAL_FIR_PLANKS:         return "fir_planks.png";
    case MATERIAL_FIR_LEAVES:         return "fir_leaves.png";
        //larch
    case MATERIAL_LARCH_LOG_SIDE:     return "larch_log_side.png";
    case MATERIAL_LARCH_LOG_TOP:      return "larch_log_top.png";
    case MATERIAL_LARCH_STRIPPED_LOG_SIDE: return "larch_stripped_log_side.png";
    case MATERIAL_LARCH_STRIPPED_LOG_TOP: return "larch_stripped_log_top.png";
    case MATERIAL_LARCH_PLANKS:       return "larch_planks.png";
    case MATERIAL_LARCH_LEAVES:       return "larch_leaves.png";
        //kapok
    case MATERIAL_KAPOK_LOG_SIDE:     return "kapok_log_side.png";
    case MATERIAL_KAPOK_LOG_TOP:      return "kapok_log_top.png";
    case MATERIAL_KAPOK_STRIPPED_LOG_SIDE: return "kapok_stripped_log_side.png";
    case MATERIAL_KAPOK_STRIPPED_LOG_TOP: return "kapok_stripped_log_top.png";
    case MATERIAL_KAPOK_PLANKS:       return "kapok_planks.png";
    case MATERIAL_KAPOK_LEAVES:       return "kapok_leaves.png";
        //rubber_tree
    case MATERIAL_RUBBER_TREE_LOG_SIDE: return "rubber_tree_log_side.png";
    case MATERIAL_RUBBER_TREE_LOG_TOP: return "rubber_tree_log_top.png";
    case MATERIAL_RUBBER_TREE_STRIPPED_LOG_SIDE: return "rubber_tree_stripped_log_side.png";
    case MATERIAL_RUBBER_TREE_STRIPPED_LOG_TOP: return "rubber_tree_stripped_log_top.png";
    case MATERIAL_RUBBER_TREE_PLANKS: return "rubber_tree_planks.png";
    case MATERIAL_RUBBER_TREE_LEAVES: return "rubber_tree_leaves.png";
        //mahogany
    case MATERIAL_MAHOGANY_LOG_SIDE:  return "mahogany_log_side.png";
    case MATERIAL_MAHOGANY_LOG_TOP:   return "mahogany_log_top.png";
    case MATERIAL_MAHOGANY_STRIPPED_LOG_SIDE: return "mahogany_stripped_log_side.png";
    case MATERIAL_MAHOGANY_STRIPPED_LOG_TOP: return "mahogany_stripped_log_top.png";
    case MATERIAL_MAHOGANY_PLANKS:    return "mahogany_planks.png";
    case MATERIAL_MAHOGANY_LEAVES:    return "mahogany_leaves.png";
        //savanna: granite
    case MATERIAL_GRANITE:            return "granite.png";
    case MATERIAL_COBBLED_GRANITE:    return "cobbled_granite.png";
        //laterite
    case MATERIAL_LATERITE:           return "laterite.png";
    case MATERIAL_COBBLED_LATERITE:   return "cobbled_laterite.png";
        //sandstone
    case MATERIAL_SANDSTONE:          return "sandstone.png";
    case MATERIAL_COBBLED_SANDSTONE:  return "cobbled_sandstone.png";
        //limestone
    case MATERIAL_LIMESTONE:          return "limestone.png";
    case MATERIAL_COBBLED_LIMESTONE:  return "cobbled_limestone.png";
        //shale
    case MATERIAL_SHALE:              return "shale.png";
    case MATERIAL_COBBLED_SHALE:      return "cobbled_shale.png";
        //slate
    case MATERIAL_SLATE:              return "slate.png";
    case MATERIAL_COBBLED_SLATE:      return "cobbled_slate.png";
        //gneiss
    case MATERIAL_GNEISS:             return "gneiss.png";
    case MATERIAL_COBBLED_GNEISS:     return "cobbled_gneiss.png";
        //schist
    case MATERIAL_SCHIST:             return "schist.png";
    case MATERIAL_COBBLED_SCHIST:     return "cobbled_schist.png";
        //andesite
    case MATERIAL_ANDESITE:           return "andesite.png";
    case MATERIAL_COBBLED_ANDESITE:   return "cobbled_andesite.png";
        //obsidian
    case MATERIAL_OBSIDIAN:           return "obsidian.png";
    case MATERIAL_COBBLED_OBSIDIAN:   return "cobbled_obsidian.png";
        //pumice
    case MATERIAL_PUMICE:             return "pumice.png";
    case MATERIAL_COBBLED_PUMICE:     return "cobbled_pumice.png";
        //tuff
    case MATERIAL_TUFF:               return "tuff.png";
    case MATERIAL_COBBLED_TUFF:       return "cobbled_tuff.png";
        //scoria
    case MATERIAL_SCORIA:             return "scoria.png";
    case MATERIAL_COBBLED_SCORIA:     return "cobbled_scoria.png";
    case MATERIAL_AIR:
    default:                         return nullptr;
    }
}

Material materialFromName(const std::string& name) {
    // One map, built once. Keys are the enum names minus the MATERIAL_ prefix.
    static const std::unordered_map<std::string, Material> table = {
        { "AIR",                 MATERIAL_AIR },
        { "COBBLESTONE",         MATERIAL_COBBLESTONE },
        { "DIORITE",             MATERIAL_DIORITE },
        { "DIRT",                MATERIAL_DIRT },
        { "GRASS_BLOCK_TOP",     MATERIAL_GRASS_BLOCK_TOP },
        { "GRASS_BLOCK_SIDE",    MATERIAL_GRASS_BLOCK_SIDE },
        { "GRASS_BLOCK_SIDE_OV", MATERIAL_GRASS_BLOCK_SIDE_OV },
        { "BIRCH_LOG_SIDE",      MATERIAL_BIRCH_LOG_SIDE },
        { "BIRCH_LOG_TOP",       MATERIAL_BIRCH_LOG_TOP },
        { "BIRCH_LEAVES",        MATERIAL_BIRCH_LEAVES },
        { "CHESTNUT_LOG_SIDE",   MATERIAL_CHESTNUT_LOG_SIDE },
        { "CHESTNUT_LOG_TOP",    MATERIAL_CHESTNUT_LOG_TOP },
        { "CHESTNUT_LEAVES",     MATERIAL_CHESTNUT_LEAVES },
        //savanna: eucalyptus
        { "EUCALYPTUS_LOG_SIDE",         MATERIAL_EUCALYPTUS_LOG_SIDE },
        { "EUCALYPTUS_LOG_TOP",          MATERIAL_EUCALYPTUS_LOG_TOP },
        { "EUCALYPTUS_STRIPPED_LOG_SIDE",  MATERIAL_EUCALYPTUS_STRIPPED_LOG_SIDE },
        { "EUCALYPTUS_STRIPPED_LOG_TOP",  MATERIAL_EUCALYPTUS_STRIPPED_LOG_TOP },
        { "EUCALYPTUS_PLANKS",           MATERIAL_EUCALYPTUS_PLANKS },
        { "EUCALYPTUS_LEAVES",           MATERIAL_EUCALYPTUS_LEAVES },
        //baobab
        { "BAOBAB_LOG_SIDE",             MATERIAL_BAOBAB_LOG_SIDE },
        { "BAOBAB_LOG_TOP",              MATERIAL_BAOBAB_LOG_TOP },
        { "BAOBAB_STRIPPED_LOG_SIDE",    MATERIAL_BAOBAB_STRIPPED_LOG_SIDE },
        { "BAOBAB_STRIPPED_LOG_TOP",     MATERIAL_BAOBAB_STRIPPED_LOG_TOP },
        { "BAOBAB_PLANKS",               MATERIAL_BAOBAB_PLANKS },
        { "BAOBAB_LEAVES",               MATERIAL_BAOBAB_LEAVES },
        //acacia
        { "ACACIA_LOG_SIDE",             MATERIAL_ACACIA_LOG_SIDE },
        { "ACACIA_LOG_TOP",              MATERIAL_ACACIA_LOG_TOP },
        { "ACACIA_STRIPPED_LOG_SIDE",    MATERIAL_ACACIA_STRIPPED_LOG_SIDE },
        { "ACACIA_STRIPPED_LOG_TOP",     MATERIAL_ACACIA_STRIPPED_LOG_TOP },
        { "ACACIA_PLANKS",               MATERIAL_ACACIA_PLANKS },
        { "ACACIA_LEAVES",               MATERIAL_ACACIA_LEAVES },
        //oak
        { "OAK_LOG_SIDE",                MATERIAL_OAK_LOG_SIDE },
        { "OAK_LOG_TOP",                 MATERIAL_OAK_LOG_TOP },
        { "OAK_STRIPPED_LOG_SIDE",       MATERIAL_OAK_STRIPPED_LOG_SIDE },
        { "OAK_STRIPPED_LOG_TOP",        MATERIAL_OAK_STRIPPED_LOG_TOP },
        { "OAK_PLANKS",                  MATERIAL_OAK_PLANKS },
        { "OAK_LEAVES",                  MATERIAL_OAK_LEAVES },
        //dark_oak
        { "DARK_OAK_LOG_SIDE",           MATERIAL_DARK_OAK_LOG_SIDE },
        { "DARK_OAK_LOG_TOP",            MATERIAL_DARK_OAK_LOG_TOP },
        { "DARK_OAK_STRIPPED_LOG_SIDE",  MATERIAL_DARK_OAK_STRIPPED_LOG_SIDE },
        { "DARK_OAK_STRIPPED_LOG_TOP",   MATERIAL_DARK_OAK_STRIPPED_LOG_TOP },
        { "DARK_OAK_PLANKS",             MATERIAL_DARK_OAK_PLANKS },
        { "DARK_OAK_LEAVES",             MATERIAL_DARK_OAK_LEAVES },
        //maple
        { "MAPLE_LOG_SIDE",              MATERIAL_MAPLE_LOG_SIDE },
        { "MAPLE_LOG_TOP",               MATERIAL_MAPLE_LOG_TOP },
        { "MAPLE_STRIPPED_LOG_SIDE",     MATERIAL_MAPLE_STRIPPED_LOG_SIDE },
        { "MAPLE_STRIPPED_LOG_TOP",      MATERIAL_MAPLE_STRIPPED_LOG_TOP },
        { "MAPLE_PLANKS",                MATERIAL_MAPLE_PLANKS },
        { "MAPLE_LEAVES",                MATERIAL_MAPLE_LEAVES },
        //beech
        { "BEECH_LOG_SIDE",              MATERIAL_BEECH_LOG_SIDE },
        { "BEECH_LOG_TOP",               MATERIAL_BEECH_LOG_TOP },
        { "BEECH_STRIPPED_LOG_SIDE",     MATERIAL_BEECH_STRIPPED_LOG_SIDE },
        { "BEECH_STRIPPED_LOG_TOP",      MATERIAL_BEECH_STRIPPED_LOG_TOP },
        { "BEECH_PLANKS",                MATERIAL_BEECH_PLANKS },
        { "BEECH_LEAVES",                MATERIAL_BEECH_LEAVES },
        { "BIRCH_STRIPPED_LOG_SIDE",     MATERIAL_BIRCH_STRIPPED_LOG_SIDE },
        { "BIRCH_STRIPPED_LOG_TOP",      MATERIAL_BIRCH_STRIPPED_LOG_TOP },
        { "BIRCH_PLANKS",                MATERIAL_BIRCH_PLANKS },
        //hickory
        { "HICKORY_LOG_SIDE",            MATERIAL_HICKORY_LOG_SIDE },
        { "HICKORY_LOG_TOP",             MATERIAL_HICKORY_LOG_TOP },
        { "HICKORY_STRIPPED_LOG_SIDE",   MATERIAL_HICKORY_STRIPPED_LOG_SIDE },
        { "HICKORY_STRIPPED_LOG_TOP",    MATERIAL_HICKORY_STRIPPED_LOG_TOP },
        { "HICKORY_PLANKS",              MATERIAL_HICKORY_PLANKS },
        { "HICKORY_LEAVES",              MATERIAL_HICKORY_LEAVES },
        //spruce
        { "SPRUCE_LOG_SIDE",             MATERIAL_SPRUCE_LOG_SIDE },
        { "SPRUCE_LOG_TOP",              MATERIAL_SPRUCE_LOG_TOP },
        { "SPRUCE_STRIPPED_LOG_SIDE",    MATERIAL_SPRUCE_STRIPPED_LOG_SIDE },
        { "SPRUCE_STRIPPED_LOG_TOP",     MATERIAL_SPRUCE_STRIPPED_LOG_TOP },
        { "SPRUCE_PLANKS",               MATERIAL_SPRUCE_PLANKS },
        { "SPRUCE_LEAVES",               MATERIAL_SPRUCE_LEAVES },
        //pine
        { "PINE_LOG_SIDE",               MATERIAL_PINE_LOG_SIDE },
        { "PINE_LOG_TOP",                MATERIAL_PINE_LOG_TOP },
        { "PINE_STRIPPED_LOG_SIDE",      MATERIAL_PINE_STRIPPED_LOG_SIDE },
        { "PINE_STRIPPED_LOG_TOP",       MATERIAL_PINE_STRIPPED_LOG_TOP },
        { "PINE_PLANKS",                 MATERIAL_PINE_PLANKS },
        { "PINE_LEAVES",                 MATERIAL_PINE_LEAVES },
        //fir
        { "FIR_LOG_SIDE",                MATERIAL_FIR_LOG_SIDE },
        { "FIR_LOG_TOP",                 MATERIAL_FIR_LOG_TOP },
        { "FIR_STRIPPED_LOG_SIDE",       MATERIAL_FIR_STRIPPED_LOG_SIDE },
        { "FIR_STRIPPED_LOG_TOP",        MATERIAL_FIR_STRIPPED_LOG_TOP },
        { "FIR_PLANKS",                  MATERIAL_FIR_PLANKS },
        { "FIR_LEAVES",                  MATERIAL_FIR_LEAVES },
        //larch
        { "LARCH_LOG_SIDE",              MATERIAL_LARCH_LOG_SIDE },
        { "LARCH_LOG_TOP",               MATERIAL_LARCH_LOG_TOP },
        { "LARCH_STRIPPED_LOG_SIDE",     MATERIAL_LARCH_STRIPPED_LOG_SIDE },
        { "LARCH_STRIPPED_LOG_TOP",      MATERIAL_LARCH_STRIPPED_LOG_TOP },
        { "LARCH_PLANKS",                MATERIAL_LARCH_PLANKS },
        { "LARCH_LEAVES",                MATERIAL_LARCH_LEAVES },
        //kapok
        { "KAPOK_LOG_SIDE",              MATERIAL_KAPOK_LOG_SIDE },
        { "KAPOK_LOG_TOP",               MATERIAL_KAPOK_LOG_TOP },
        { "KAPOK_STRIPPED_LOG_SIDE",     MATERIAL_KAPOK_STRIPPED_LOG_SIDE },
        { "KAPOK_STRIPPED_LOG_TOP",      MATERIAL_KAPOK_STRIPPED_LOG_TOP },
        { "KAPOK_PLANKS",                MATERIAL_KAPOK_PLANKS },
        { "KAPOK_LEAVES",                MATERIAL_KAPOK_LEAVES },
        //rubber_tree
        { "RUBBER_TREE_LOG_SIDE",        MATERIAL_RUBBER_TREE_LOG_SIDE },
        { "RUBBER_TREE_LOG_TOP",         MATERIAL_RUBBER_TREE_LOG_TOP },
        { "RUBBER_TREE_STRIPPED_LOG_SIDE",  MATERIAL_RUBBER_TREE_STRIPPED_LOG_SIDE },
        { "RUBBER_TREE_STRIPPED_LOG_TOP",  MATERIAL_RUBBER_TREE_STRIPPED_LOG_TOP },
        { "RUBBER_TREE_PLANKS",          MATERIAL_RUBBER_TREE_PLANKS },
        { "RUBBER_TREE_LEAVES",          MATERIAL_RUBBER_TREE_LEAVES },
        //mahogany
        { "MAHOGANY_LOG_SIDE",           MATERIAL_MAHOGANY_LOG_SIDE },
        { "MAHOGANY_LOG_TOP",            MATERIAL_MAHOGANY_LOG_TOP },
        { "MAHOGANY_STRIPPED_LOG_SIDE",  MATERIAL_MAHOGANY_STRIPPED_LOG_SIDE },
        { "MAHOGANY_STRIPPED_LOG_TOP",   MATERIAL_MAHOGANY_STRIPPED_LOG_TOP },
        { "MAHOGANY_PLANKS",             MATERIAL_MAHOGANY_PLANKS },
        { "MAHOGANY_LEAVES",             MATERIAL_MAHOGANY_LEAVES },
        //savanna: granite
        { "GRANITE",                     MATERIAL_GRANITE },
        { "COBBLED_GRANITE",             MATERIAL_COBBLED_GRANITE },
        //laterite
        { "LATERITE",                    MATERIAL_LATERITE },
        { "COBBLED_LATERITE",            MATERIAL_COBBLED_LATERITE },
        //sandstone
        { "SANDSTONE",                   MATERIAL_SANDSTONE },
        { "COBBLED_SANDSTONE",           MATERIAL_COBBLED_SANDSTONE },
        //limestone
        { "LIMESTONE",                   MATERIAL_LIMESTONE },
        { "COBBLED_LIMESTONE",           MATERIAL_COBBLED_LIMESTONE },
        //shale
        { "SHALE",                       MATERIAL_SHALE },
        { "COBBLED_SHALE",               MATERIAL_COBBLED_SHALE },
        //slate
        { "SLATE",                       MATERIAL_SLATE },
        { "COBBLED_SLATE",               MATERIAL_COBBLED_SLATE },
        //gneiss
        { "GNEISS",                      MATERIAL_GNEISS },
        { "COBBLED_GNEISS",              MATERIAL_COBBLED_GNEISS },
        //schist
        { "SCHIST",                      MATERIAL_SCHIST },
        { "COBBLED_SCHIST",              MATERIAL_COBBLED_SCHIST },
        //andesite
        { "ANDESITE",                    MATERIAL_ANDESITE },
        { "COBBLED_ANDESITE",            MATERIAL_COBBLED_ANDESITE },
        //obsidian
        { "OBSIDIAN",                    MATERIAL_OBSIDIAN },
        { "COBBLED_OBSIDIAN",            MATERIAL_COBBLED_OBSIDIAN },
        //pumice
        { "PUMICE",                      MATERIAL_PUMICE },
        { "COBBLED_PUMICE",              MATERIAL_COBBLED_PUMICE },
        //tuff
        { "TUFF",                        MATERIAL_TUFF },
        { "COBBLED_TUFF",                MATERIAL_COBBLED_TUFF },
        //scoria
        { "SCORIA",                      MATERIAL_SCORIA },
        { "COBBLED_SCORIA",              MATERIAL_COBBLED_SCORIA },
    };
    auto it = table.find(name);
    return it != table.end() ? it->second : MATERIAL_COUNT;
}

bool UploadTextureArrayLayers(
    SDL_GPUDevice* gpu,
    SDL_GPUTexture* textureArray
) {
    bool success = true;
    // Skip MATERIAL_AIR (no texture); every other material maps to one layer.
    for (int m = MATERIAL_AIR + 1; m < MATERIAL_COUNT; ++m) {
        Material material = (Material)m;
        const char* file = materialTextureFile(material);
        if (!file) continue;

        if (!UploadTextureArrayLayer(gpu, textureArray, baseTexturePathMaterials, file, material)) {
            SDL_Log("Failed to load Texture for material %d ('%s')", m, file);
            success = false;
        }
    }
    return success;
}