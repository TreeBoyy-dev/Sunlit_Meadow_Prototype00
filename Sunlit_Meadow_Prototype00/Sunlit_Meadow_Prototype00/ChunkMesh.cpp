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
    numIndices = 0;
    textureArray = nullptr;
}

Uint16 ChunkMesh::getNeighborId(int x, int y, int z) const
{
    //auto it = blockSet.find({ x, y, z, 0 });
    return 0; //it != blockSet.end() ? it->id : 0;
}
bool ChunkMesh::neighborObstructs(Uint16 id, int faceIndex, BlockManager& blockManager)
{
    Block* b = blockManager.getById(id);
    return b->getObstructs(faceIndex);
}

void ChunkMesh::buildMesh(
    PalettedContainer* storage,
    ChunkCoord chunkCoords,
    BlockManager& blockManager,
    bool isTranperent)
{
    m_chunkCoord = chunkCoords;
    isTranperentMesh = isTranperent;

    vertices.clear();
    indices.clear();

    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_SIZE; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {

                Uint16 id = storage->getId(x, y, z);
                Uint16 state = storage->getState(x, y, z);
                Block* b = blockManager.getById(id);
                if (!b) {
                    SDL_Log("[ChunkMesh] buildMesh: unknown block id %u at %d,%d,%d", id, x, y, z);
                    continue;
                }
                if(b->isTransparent() != isTranperentMesh || id == 0)
                    continue;

                b->generateMeshFromModel(
                    vertices, indices,
                    x + chunkCoords.x * CHUNK_SIZE,
                    y + chunkCoords.y * CHUNK_SIZE,
                    z + chunkCoords.z * CHUNK_SIZE,
                    state
                );
            }
        }
    }
    numIndices = (uint32_t)indices.size();
    /*
        SDL_Log("[ChunkMesh] (transperent: %d) buildMesh at %d|%d|%d:",
            isTranperentMesh, chunkCoords.x, chunkCoords.y, chunkCoords.z);
        Uint32 maxIdx = 0;
        for (Uint32 i : indices) maxIdx = SDL_max(maxIdx, i);
        SDL_Log("verts=%zu maxIndex=%u",
            vertices.size(), maxIdx);
    */
}

void ChunkMesh::optimizeMesh() {
    faceCulling();
    greedyMeshing();
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
        .size = (Uint32)(indices.size() * sizeof(Uint32)),
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
        .size = (Uint32)(indices.size() * sizeof(Uint32)),
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
    SDL_memcpy(ibMapped, indices.data(), indices.size() * sizeof(Uint32));
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
        .size = (Uint32)(indices.size() * sizeof(Uint32)),
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
        SDL_Log("[ChunkMesh] draw: couldn't draw Chunkmesh (indices: &d)", numIndices);
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
    SDL_BindGPUIndexBuffer(pass, &index_buffer_binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
    SDL_PushGPUVertexUniformData(cmd, 0, &ubo, (Uint32)sizeof(ubo));
    SDL_BindGPUFragmentSamplers(pass, 0, &tex_sampler_binding, 1);
    SDL_DrawGPUIndexedPrimitives(pass, numIndices, 1, 0, 0, 0);
}