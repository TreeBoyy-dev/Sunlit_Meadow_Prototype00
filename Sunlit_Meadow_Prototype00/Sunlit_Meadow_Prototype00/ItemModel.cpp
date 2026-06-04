#include "ItemModel.h"
#include <cmath>
#include <cstring>

void ItemModel::setMesh(const std::vector<ModelVertex>& verts,
    const std::vector<Uint16>& inds)
{
    vertices = verts;
    indices = inds;
    uploaded = false;
    computeBounds();
}

void ItemModel::computeBounds()
{
    if (vertices.empty()) {
        center = { 0.0f, 0.0f, 0.0f };
        radius = 1.0f;
        return;
    }

    Vec3 mn = vertices[0].position;
    Vec3 mx = vertices[0].position;
    for (const auto& v : vertices) {
        mn.x = SDL_min(mn.x, v.position.x); mx.x = SDL_max(mx.x, v.position.x);
        mn.y = SDL_min(mn.y, v.position.y); mx.y = SDL_max(mx.y, v.position.y);
        mn.z = SDL_min(mn.z, v.position.z); mx.z = SDL_max(mx.z, v.position.z);
    }
    center = { (mn.x + mx.x) * 0.5f, (mn.y + mx.y) * 0.5f, (mn.z + mx.z) * 0.5f };

    float r = 0.0f;
    for (const auto& v : vertices) {
        float dx = v.position.x - center.x;
        float dy = v.position.y - center.y;
        float dz = v.position.z - center.z;
        r = SDL_max(r, sqrtf(dx * dx + dy * dy + dz * dz));
    }
    radius = (r > 0.0001f) ? r : 1.0f;
}

bool ItemModel::ensureUploaded(SDL_GPUDevice* gpu, SDL_GPUCommandBuffer* cmd)
{
    if (uploaded) return (vbo != nullptr && ibo != nullptr);
    if (vertices.empty() || indices.empty()) return false;

    const Uint32 vbSize = (Uint32)(vertices.size() * sizeof(ModelVertex));
    const Uint32 ibSize = (Uint32)(indices.size() * sizeof(Uint16));

    SDL_GPUBufferCreateInfo vbInfo = { .usage = SDL_GPU_BUFFERUSAGE_VERTEX, .size = vbSize };
    vbo = SDL_CreateGPUBuffer(gpu, &vbInfo);

    SDL_GPUBufferCreateInfo ibInfo = { .usage = SDL_GPU_BUFFERUSAGE_INDEX, .size = ibSize };
    ibo = SDL_CreateGPUBuffer(gpu, &ibInfo);

    if (!vbo || !ibo) {
        SDL_Log("[ItemModel] failed to create GPU buffers: %s", SDL_GetError());
        return false;
    }

    // One transfer buffer holds both the vertices and the indices back to back.
    SDL_GPUTransferBufferCreateInfo tbInfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = vbSize + ibSize,
    };
    SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(gpu, &tbInfo);
    Uint8* mapped = (Uint8*)SDL_MapGPUTransferBuffer(gpu, tb, false);
    memcpy(mapped, vertices.data(), vbSize);
    memcpy(mapped + vbSize, indices.data(), ibSize);
    SDL_UnmapGPUTransferBuffer(gpu, tb);

    SDL_GPUCopyPass* cp = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTransferBufferLocation vsrc = { .transfer_buffer = tb, .offset = 0 };
    SDL_GPUBufferRegion           vdst = { .buffer = vbo, .offset = 0, .size = vbSize };
    SDL_UploadToGPUBuffer(cp, &vsrc, &vdst, false);

    SDL_GPUTransferBufferLocation isrc = { .transfer_buffer = tb, .offset = vbSize };
    SDL_GPUBufferRegion           idst = { .buffer = ibo, .offset = 0, .size = ibSize };
    SDL_UploadToGPUBuffer(cp, &isrc, &idst, false);

    SDL_EndGPUCopyPass(cp);
    SDL_ReleaseGPUTransferBuffer(gpu, tb);

    uploaded = true;
    return true;
}

void ItemModel::destroy(SDL_GPUDevice* gpu)
{
    if (vbo) { SDL_ReleaseGPUBuffer(gpu, vbo); vbo = nullptr; }
    if (ibo) { SDL_ReleaseGPUBuffer(gpu, ibo); ibo = nullptr; }
    texture = nullptr;
    uploaded = false;
}