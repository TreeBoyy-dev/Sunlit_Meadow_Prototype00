#include "ChunkMesh.h"
#include "BlockModel.h"
#include "Block.h"
#include "Globals.h"

#include <chrono>
#include <bit>

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

// getNeighborId() (a dead stub that always returned 0) and
// neighborObstructs() (never called, and would have crashed on unknown ids)
// are deleted.

void ChunkMesh::buildMesh(
    PalettedContainer* storage,
    ChunkCoord chunkCoords,
    BlockManager& blockManager,
    bool isTranperent)
{
    using clock = std::chrono::steady_clock;
    const auto tBuild0 = clock::now();
    m_stats = BuildStats{};   // fresh stats for this pass

    m_chunkCoord = chunkCoords;
    isTranperentMesh = isTranperent;

    vertices.clear();
    indices.clear();
    numIndices = 0;

    // ---- resolve the PALETTE once, not every cell. ----
    // The old loop did getId + getState (two bit-unpacks) + an unordered_map
    // lookup for all 4096 cells, twice per chunk. Palettes are tiny (median
    // 2 in the baseline), so resolve Block*, state, and "does this entry
    // emit in this pass" per ENTRY, then the cell loop is an array index.
    const std::vector<PaletteEntry>& pal = storage->getPalette();

    struct Resolved {
        Block* block = nullptr;
        Uint16 state = 0;
        Uint8  cover = 0;
        bool   emit  = false;
    };
    std::vector<Resolved> resolved(pal.size());

    bool anyEmit = false;
    for (size_t p = 0; p < pal.size(); ++p) {
        Block* b = blockManager.getById(pal[p].id);
        resolved[p].block = b;
        resolved[p].state = pal[p].state;
        resolved[p].emit  = b && pal[p].id != 0
                          && b->isTransparent() == isTranperentMesh;
        // a cell hides its neighbor's touching face only when it is
        // rendered in the SAME pass (opaque vs transparent meshes never
        // culled each other before either) and its variant fully covers the
        // shared plane. Derived from baked geometry, per palette ENTRY.
        resolved[p].cover = resolved[p].emit
                          ? b->getCoverMask(pal[p].state) : 0;
        anyEmit |= resolved[p].emit;
    }

    // Nothing in this chunk belongs to this pass (all-air chunks, and the
    // transparent pass over opaque-only terrain — ~85% of all passes in the
    // baseline run). Skip the cell loop entirely.
    if (!anyEmit) {
        m_stats.buildMs = std::chrono::duration<double, std::milli>(clock::now() - tBuild0).count();
        return;
    }

    // ---- bulk-decode the packed indices in one sequential walk ----
    // instead of 4096 independent div/mod/shift unpacks.
    Uint16 local[PalettedContainer::VOLUME];
    storage->decodeIndices(local);

    const int baseX = chunkCoords.x * CHUNK_SIZE;
    const int baseY = chunkCoords.y * CHUNK_SIZE;
    const int baseZ = chunkCoords.z * CHUNK_SIZE;

    // ---- per-face visibility from in-chunk neighbors. ----
    // Face d of the cell is hidden when the neighbor in direction d fully
    // covers the shared plane (its cover bit for the OPPOSITE direction).
    // Cells on the chunk border keep their outward face visible — exactly
    // what the old within-chunk faceCulling produced, so output is
    // identical; cross-chunk hiding is a later phase.
    // Linear strides in the x-major layout.
    constexpr int STRIDE_X = CHUNK_SIZE * CHUNK_SIZE;
    constexpr int STRIDE_Y = CHUNK_SIZE;
    int i = 0;
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_SIZE; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++, i++) {
                const Resolved& r = resolved[local[i]];
                if (!r.block) { m_stats.unknownIds++; continue; }
                if (!r.emit) continue;

                Uint8 vis = FaceDir::AllVisible;
                if (x < CHUNK_SIZE - 1 && (resolved[local[i + STRIDE_X]].cover & (1u << FaceDir::NegX)))
                    vis &= (Uint8)~(1u << FaceDir::PosX);
                if (x > 0              && (resolved[local[i - STRIDE_X]].cover & (1u << FaceDir::PosX)))
                    vis &= (Uint8)~(1u << FaceDir::NegX);
                if (y < CHUNK_SIZE - 1 && (resolved[local[i + STRIDE_Y]].cover  & (1u << FaceDir::NegY)))
                    vis &= (Uint8)~(1u << FaceDir::PosY);
                if (y > 0              && (resolved[local[i - STRIDE_Y]].cover  & (1u << FaceDir::PosY)))
                    vis &= (Uint8)~(1u << FaceDir::NegY);
                if (z < CHUNK_SIZE - 1 && (resolved[local[i + 1]].cover   & (1u << FaceDir::NegZ)))
                    vis &= (Uint8)~(1u << FaceDir::PosZ);
                if (z > 0              && (resolved[local[i - 1]].cover   & (1u << FaceDir::PosZ)))
                    vis &= (Uint8)~(1u << FaceDir::NegZ);

                m_stats.facesHidden += 6u - (uint32_t)std::popcount((unsigned)vis);

                const size_t vertsBefore = vertices.size();
                r.block->generateMeshFromModel(
                    vertices, indices,
                    x + baseX, y + baseY, z + baseZ,
                    r.state, vis
                );
                m_stats.cellsEmitted++;
                if (vis == FaceDir::AllVisible && vertices.size() == vertsBefore)
                    m_stats.emptyEmits++;   // paid the model lookup, got nothing
            }
        }
    }
    numIndices = (uint32_t)indices.size();

    m_stats.vertsEmitted = (uint32_t)vertices.size();
    m_stats.indsEmitted  = (uint32_t)indices.size();
    m_stats.buildMs = std::chrono::duration<double, std::milli>(clock::now() - tBuild0).count();

    if (m_stats.unknownIds > 0)
        SDL_Log("[ChunkMesh] buildMesh %d|%d|%d: %u cells with unknown block ids",
            chunkCoords.x, chunkCoords.y, chunkCoords.z, m_stats.unknownIds);
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
    using clock = std::chrono::steady_clock;

    const auto t0 = clock::now();
    faceCulling();                       // sets m_stats.quadsCulled
    const auto t1 = clock::now();
    greedyMeshing();
    const auto t2 = clock::now();

    m_stats.cullMs   = std::chrono::duration<double, std::milli>(t1 - t0).count();
    m_stats.greedyMs = std::chrono::duration<double, std::milli>(t2 - t1).count();
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