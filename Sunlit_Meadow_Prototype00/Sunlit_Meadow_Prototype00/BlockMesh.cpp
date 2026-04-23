#include "BlockMesh.h"

bool BlockMesh::init(
    AppState* state,
    std::vector<Block>& blocks,
    SDL_GPUTexture* textureArrayIn
)
{
    textureArray = textureArrayIn;

    buildMesh(blocks);
    return uploadToGPU(state);
}

void BlockMesh::destroy(AppState* state)
{
    if (vertexBuffer) {
        SDL_ReleaseGPUBuffer(state->gpu, vertexBuffer);
        vertexBuffer = nullptr;
    }

    if (indexBuffer) {
        SDL_ReleaseGPUBuffer(state->gpu, indexBuffer);
        indexBuffer = nullptr;
    }

    vertices.clear();
    indices.clear();
    blockSet.clear();
    numIndices = 0;
    textureArray = nullptr;
}

bool BlockMesh::hasBlock(int x, int y, int z) const
{
    return blockSet.find({ x, y, z }) != blockSet.end();
}

void BlockMesh::addFace(
    const Vec3& v0,
    const Vec3& v1,
    const Vec3& v2,
    const Vec3& v3,
    const Vec3& normal,
    int materialIndex
)
{
    uint16_t startIndex = (uint16_t)vertices.size();
    float mat = (float)materialIndex;
    SDL_FColor white = { 1.0f, 1.0f, 1.0f, 1.0f };

    vertices.push_back({ v0, { 0.0f, 0.0f }, white, mat });
    vertices.push_back({ v1, { 1.0f, 0.0f }, white, mat });
    vertices.push_back({ v2, { 1.0f, 1.0f }, white, mat });
    vertices.push_back({ v3, { 0.0f, 1.0f }, white, mat });

    indices.push_back(startIndex + 0);
    indices.push_back(startIndex + 1);
    indices.push_back(startIndex + 2);
    indices.push_back(startIndex + 0);
    indices.push_back(startIndex + 2);
    indices.push_back(startIndex + 3);
}

void BlockMesh::buildMesh(std::vector<Block>& blocks)
{
    vertices.clear();
    indices.clear();
    blockSet.clear();

    for (Block& block : blocks) {
        blockSet.insert(block.getPosition());
    }

    for (Block& block : blocks) {
        BlockCoordinates pos = block.getPosition();

        float x = (float)pos.x;
        float y = (float)pos.y;
        float z = (float)pos.z;

        Vec3 p000 = { x,     y,     z };
        Vec3 p100 = { x + 1, y,     z };
        Vec3 p110 = { x + 1, y + 1, z };
        Vec3 p010 = { x,     y + 1, z };

        Vec3 p001 = { x,     y,     z + 1 };
        Vec3 p101 = { x + 1, y,     z + 1 };
        Vec3 p111 = { x + 1, y + 1, z + 1 };
        Vec3 p011 = { x,     y + 1, z + 1 };

        if (!hasBlock(pos.x, pos.y + 1, pos.z)) {
            addFace(p011, p111, p110, p010, { 0.0f, 1.0f, 0.0f }, block.getMaterialUP());
        }

        if (!hasBlock(pos.x, pos.y - 1, pos.z)) {
            addFace(p000, p100, p101, p001, { 0.0f, -1.0f, 0.0f }, block.getMaterialDOWN());
        }

        if (!hasBlock(pos.x, pos.y, pos.z - 1)) {
            addFace(p100, p000, p010, p110, { 0.0f, 0.0f, -1.0f }, block.getMaterialNORTH());
        }

        if (!hasBlock(pos.x, pos.y, pos.z + 1)) {
            addFace(p001, p101, p111, p011, { 0.0f, 0.0f, 1.0f }, block.getMaterialSOUTH());
        }

        if (!hasBlock(pos.x + 1, pos.y, pos.z)) {
            addFace(p101, p100, p110, p111, { 1.0f, 0.0f, 0.0f }, block.getMaterialEAST());
        }

        if (!hasBlock(pos.x - 1, pos.y, pos.z)) {
            addFace(p000, p001, p011, p010, { -1.0f, 0.0f, 0.0f }, block.getMaterialWEST());
        }
    }

    numIndices = (uint32_t)indices.size();
}

bool BlockMesh::uploadToGPU(AppState* state)
{
    if (vertices.empty() || indices.empty()) {
        SDL_Log("Block mesh is empty");
        return false;
    }

    SDL_GPUBufferCreateInfo vbInfo = {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = (Uint32)(vertices.size() * sizeof(VertexData)),
    };
    vertexBuffer = SDL_CreateGPUBuffer(state->gpu, &vbInfo);
    if (!vertexBuffer) {
        SDL_Log("Failed to create block vertex buffer: %s", SDL_GetError());
        return false;
    }

    SDL_GPUBufferCreateInfo ibInfo = {
        .usage = SDL_GPU_BUFFERUSAGE_INDEX,
        .size = (Uint32)(indices.size() * sizeof(uint16_t)),
    };
    indexBuffer = SDL_CreateGPUBuffer(state->gpu, &ibInfo);
    if (!indexBuffer) {
        SDL_Log("Failed to create block index buffer: %s", SDL_GetError());
        return false;
    }

    SDL_GPUTransferBufferCreateInfo transferVBInfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = (Uint32)(vertices.size() * sizeof(VertexData)),
    };
    SDL_GPUTransferBuffer* transferVB = SDL_CreateGPUTransferBuffer(state->gpu, &transferVBInfo);

    SDL_GPUTransferBufferCreateInfo transferIBInfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = (Uint32)(indices.size() * sizeof(uint16_t)),
    };
    SDL_GPUTransferBuffer* transferIB = SDL_CreateGPUTransferBuffer(state->gpu, &transferIBInfo);

    if (!transferVB || !transferIB) {
        SDL_Log("Failed to create transfer buffers");
        return false;
    }

    void* vbMapped = SDL_MapGPUTransferBuffer(state->gpu, transferVB, false);
    SDL_memcpy(vbMapped, vertices.data(), vertices.size() * sizeof(VertexData));
    SDL_UnmapGPUTransferBuffer(state->gpu, transferVB);

    void* ibMapped = SDL_MapGPUTransferBuffer(state->gpu, transferIB, false);
    SDL_memcpy(ibMapped, indices.data(), indices.size() * sizeof(uint16_t));
    SDL_UnmapGPUTransferBuffer(state->gpu, transferIB);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(state->gpu);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTransferBufferLocation transferBufferLocation = {
        .transfer_buffer = transferVB,
        .offset = 0,
    };
    SDL_GPUBufferRegion bufferRegion = {
        .buffer = vertexBuffer,
        .offset = 0,
        .size = (Uint32)(vertices.size() * sizeof(VertexData)),
    };
    SDL_UploadToGPUBuffer(
        copyPass,
        &transferBufferLocation,
        &bufferRegion,
        false
    );

    transferBufferLocation = {
        .transfer_buffer = transferIB,
        .offset = 0,
    };
    bufferRegion = {
        .buffer = indexBuffer,
        .offset = 0,
        .size = (Uint32)(indices.size() * sizeof(uint16_t)),
    };
    SDL_UploadToGPUBuffer(
        copyPass,
        &transferBufferLocation,
        &bufferRegion,
        false
    );

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(cmd);

    SDL_ReleaseGPUTransferBuffer(state->gpu, transferVB);
    SDL_ReleaseGPUTransferBuffer(state->gpu, transferIB);

    return true;
}

void BlockMesh::draw(
    AppState* state,
    SDL_GPUCommandBuffer* cmd,
    SDL_GPURenderPass* pass,
    const UBO& ubo
)
{
    if (!vertexBuffer || !indexBuffer || !textureArray || numIndices == 0) {
        return;
    }

    SDL_GPUBufferBinding vertex_buffer_binding = {
        .buffer = vertexBuffer,
        .offset = 0,
    };

    SDL_GPUBufferBinding index_buffer_binding = {
        .buffer = indexBuffer,
        .offset = 0,
    };

    SDL_GPUTextureSamplerBinding tex_sampler_binding = {
        .texture = textureArray,
        .sampler = state->sampler,
    };

    SDL_BindGPUGraphicsPipeline(pass, state->pipeline);
    SDL_BindGPUVertexBuffers(pass, 0, &vertex_buffer_binding, 1);
    SDL_BindGPUIndexBuffer(pass, &index_buffer_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
    SDL_PushGPUVertexUniformData(cmd, 0, &ubo, (Uint32)sizeof(ubo));
    SDL_BindGPUFragmentSamplers(pass, 0, &tex_sampler_binding, 1);
    SDL_DrawGPUIndexedPrimitives(pass, numIndices, 1, 0, 0, 0);
}