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

Uint16 ChunkMesh::getNeighborId(int x, int y, int z, ChunkBorderAir borderAir) const
{
    const int baseX = m_chunkCoord.x * CHUNK_SIZE;
    const int baseY = m_chunkCoord.y * CHUNK_SIZE;
    const int baseZ = m_chunkCoord.z * CHUNK_SIZE;

    const int lx = x - baseX;
    const int ly = y - baseY;
    const int lz = z - baseZ;

    if (x == baseX + CHUNK_SIZE) return borderAir.front[ly][lz];
    if (x == baseX - 1)          return borderAir.back[ly][lz];
    if (y == baseY + CHUNK_SIZE) return borderAir.right[lx][lz];
    if (y == baseY - 1)          return borderAir.left[lx][lz];
    if (z == baseZ + CHUNK_SIZE) return borderAir.top[lx][ly];
    if (z == baseZ - 1)          return borderAir.bottom[lx][ly];

    auto it = blockSet.find({ x, y, z, 0 });
    return it != blockSet.end() ? it->id : 0;
}
bool ChunkMesh::neighborObstructs(Uint16 id, int faceIndex, BlockManager& blockManager)
{
    Block* b = blockManager.getById(id);
    return b->getObstructs(faceIndex);
}

void ChunkMesh::buildMesh(
    std::vector<LocationalBlockID>& blocks,
    ChunkBorderAir borderAir,
    ChunkCoord chunkCoords,
    BlockManager& blockManager)
{
    m_chunkCoord = chunkCoords;

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

        // In buildMesh, replace the AdjacencyInfo block:
        AdjacencyInfo adj = {
            neighborObstructs(getNeighborId(block.x + 1, block.y, block.z, borderAir), 0, blockManager), // front:  neighbor's back
            neighborObstructs(getNeighborId(block.x - 1, block.y, block.z, borderAir), 0, blockManager), // back:   neighbor's front
            neighborObstructs(getNeighborId(block.x, block.y + 1, block.z, borderAir), 0, blockManager), // right:  neighbor's left
            neighborObstructs(getNeighborId(block.x, block.y - 1, block.z, borderAir), 0, blockManager), // left:   neighbor's right
            neighborObstructs(getNeighborId(block.x, block.y, block.z + 1, borderAir), 0, blockManager), // top:    neighbor's down
            neighborObstructs(getNeighborId(block.x, block.y, block.z - 1, borderAir), 0, blockManager), // bottom: neighbor's up
        };
        /*
        SDL_Log("pos: %f|%f|%f  adj: %d %d %d %d %d %d  ids: %d %d %d %d %d %d  ",
            x, y, z,
            adj.front, adj.back, adj.right, adj.left, adj.top, adj.bottom,
            getNeighborId(block.x + 1, block.y, block.z, borderAir), // front:  neighbor's back
            getNeighborId(block.x - 1, block.y, block.z, borderAir), // back:   neighbor's front
            getNeighborId(block.x, block.y + 1, block.z, borderAir), // right:  neighbor's left
            getNeighborId(block.x, block.y - 1, block.z, borderAir), // left:   neighbor's right
            getNeighborId(block.x, block.y, block.z + 1, borderAir), // top:    neighbor's down
            getNeighborId(block.x, block.y, block.z - 1, borderAir) // bottom: neighbor's up
        );//*/

        b->generateMeshFromModel(vertices, indices, adj, block.x, block.y, block.z);
    }

    numIndices = (uint32_t)indices.size();
}

bool ChunkMesh::uploadToGPU(AppState* state, SDL_GPUTexture* textureArrayIn)
{
    if (vertexBuffer) {
        SDL_ReleaseGPUBuffer(state->gpu, vertexBuffer);
        vertexBuffer = nullptr;
    }

    if (indexBuffer) {
        SDL_ReleaseGPUBuffer(state->gpu, indexBuffer);
        indexBuffer = nullptr;
    }

    textureArray = textureArrayIn;
    if (vertices.empty() || indices.empty()) {
        //SDL_Log("Block mesh is empty");
        return false;
    }

    SDL_GPUBufferCreateInfo vbInfo = {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = (Uint32)(vertices.size() * sizeof(WorldVertex)),
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
        .size = (Uint32)(vertices.size() * sizeof(WorldVertex)),
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
    SDL_memcpy(vbMapped, vertices.data(), vertices.size() * sizeof(WorldVertex));
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
        .size = (Uint32)(vertices.size() * sizeof(WorldVertex)),
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

    SDL_BindGPUVertexBuffers(pass, 0, &vertex_buffer_binding, 1);
    SDL_BindGPUIndexBuffer(pass, &index_buffer_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
    SDL_PushGPUVertexUniformData(cmd, 0, &ubo, (Uint32)sizeof(ubo));
    SDL_BindGPUFragmentSamplers(pass, 0, &tex_sampler_binding, 1);
    SDL_DrawGPUIndexedPrimitives(pass, numIndices, 1, 0, 0, 0);
}