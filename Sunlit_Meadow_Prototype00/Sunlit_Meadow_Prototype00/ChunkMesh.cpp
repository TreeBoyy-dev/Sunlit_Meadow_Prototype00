#include "ChunkMesh.h"
#include "BlockModel.h"
#include "Block.h"
#include "Globals.h"

void ChunkMesh::destroy(AppState* state)
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

bool ChunkMesh::hasBlock(int x, int y, int z, ChunkBorderAir borderAir) const
{
    if (x == CHUNK_SIZE) return !borderAir.front [y][z]; // x+
    if (x == -1)         return !borderAir.back  [y][z]; // x-
    if (y == CHUNK_SIZE) return !borderAir.right [x][z]; // y+
    if (y == -1)         return !borderAir.left  [x][z]; // y-
    if (z == CHUNK_SIZE) return !borderAir.top   [x][y]; // z+
    if (z == -1)         return !borderAir.bottom[x][y]; // z-

    return blockSet.find({ x, y, z }) != blockSet.end();
}

void ChunkMesh::buildMesh(std::vector<LocationalBlockID>& blocks, ChunkBorderAir borderAir)
{
    vertices.clear();
    indices.clear();
    blockSet.clear();

    for (LocationalBlockID& block : blocks) {
        blockSet.insert(block);
    }

    for (LocationalBlockID& block : blocks) {
        float x = (float)block.x;
        float y = (float)block.y;
        float z = (float)block.z;

        Block* b = blockManager.getById(block.id);

        AdjacencyInfo adj = {
            hasBlock(block.x,     block.y,     block.z + 1, borderAir),
            hasBlock(block.x,     block.y,     block.z - 1, borderAir),
            hasBlock(block.x + 1, block.y,     block.z,     borderAir),
            hasBlock(block.x - 1, block.y,     block.z,     borderAir),
            hasBlock(block.x,     block.y + 1, block.z,     borderAir),
            hasBlock(block.x,     block.y - 1, block.z,     borderAir),
        };
        b->generateMeshFromModel(vertices, indices, adj, block.x, block.y, block.z);
    }

    numIndices = (uint32_t)indices.size();
}

bool ChunkMesh::uploadToGPU(AppState* state, SDL_GPUTexture* textureArrayIn)
{
    textureArray = textureArrayIn;
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
        .size = (Uint32)(indices.size() * sizeof(Uint16)),
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
        .size = (Uint32)(indices.size() * sizeof(Uint16)),
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
    SDL_memcpy(ibMapped, indices.data(), indices.size() * sizeof(Uint16));
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
        .size = (Uint32)(indices.size() * sizeof(Uint16)),
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

void ChunkMesh::draw(
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