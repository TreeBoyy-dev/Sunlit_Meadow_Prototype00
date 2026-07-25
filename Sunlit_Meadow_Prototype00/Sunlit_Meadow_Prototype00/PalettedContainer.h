#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <algorithm>
#include "WorldTypes.h"

// =====================================================================
//  PalettedContainer
//  Stores 16^3 block cells as bit-packed local indices into a small
//  per-chunk palette of (id, state) pairs.
//
//   - Air / uniform chunks collapse to a 1-entry palette -> 0 bits/cell
//     -> 0 bytes of cell data ("air uses no memory").
//   - A chunk with N distinct (id,state) combos uses the smallest
//     allowed index width that can address N entries.
//   - Worst case in a single chunk is 4096 distinct combos -> 12 bits,
//
//  Cell layout is x-major to match the engine's blockIDs[x][y][z]:
//      linear = (x*16 + y)*16 + z
// =====================================================================

namespace PaletteConfig {
    // ---- The ONE place to retune bit widths ----
    // Ascending list of allowed index widths. The smallest width that can
    // address the whole palette is chosen.
    //   {0,4,8,12,16} -> nibble-aligned tiers -> better for runtime
    //   {0,1,2,3,...,16} -> tightest possible packing (saves a bit on tiny
    //                       palettes at the cost of odd widths).
    static constexpr uint8_t kWidths[] = { 0, 4, 8, 12, 16 };
    static constexpr int     kNumWidths = sizeof(kWidths) / sizeof(kWidths[0]);
}

struct PaletteEntry {
    Uint16 id = 0;
    Uint16 state = 0;
    bool operator==(const PaletteEntry& o) const { return id == o.id && state == o.state; }
};

class PalettedContainer {
public:
    static constexpr int VOLUME = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;

    PalettedContainer() {
        palette.push_back(PaletteEntry{ 0, 0 }); // default: all air, 0 bits, 0 data bytes
    }

    // ---- Build from a dense 16^3 array of global ids (generator path) ----
    // denseIds length must be VOLUME, indexed as (x*16 + y)*16 + z.
    //
    // The old version linear-scanned the palette for EVERY
    // cell (O(cells x palette)). Now:
    //   - a last-id run cache handles the common case (terrain is long runs
    //     of the same id) with zero hashing,
    //   - a hash map handles id changes in O(1),
    //   - and the palette is canonicalized so that IF air (id 0) is present,
    //     it sits at index 0. NOTE: the invariant
    //     is guaranteed after fromDenseIds; set() may later append air at a
    //     higher index (harmless — callers must not assume the reverse,
    //     "index 0 is air", only "air, if present, is index 0" post-build).
    void fromDenseIds(const Uint16* denseIds) {
        palette.clear();
        data.clear();
        bits = 0; perWord = 0;

        std::vector<Uint16> local(VOLUME);

        std::unordered_map<Uint16, int> lookup;
        lookup.reserve(16);
        Uint16 runId  = denseIds[0];
        int    runIdx = 0;
        palette.push_back(PaletteEntry{ runId, 0 });
        lookup.emplace(runId, 0);
        int airIndex = (runId == 0) ? 0 : -1;

        for (int i = 0; i < VOLUME; ++i) {
            const Uint16 id = denseIds[i];
            if (id != runId) {                       // run ended -> re-resolve
                auto [it, inserted] = lookup.try_emplace(id, (int)palette.size());
                if (inserted) {
                    palette.push_back(PaletteEntry{ id, 0 });
                    if (id == 0) airIndex = it->second;
                }
                runId = id;
                runIdx = it->second;
            }
            local[i] = (Uint16)runIdx;
        }

        // Canonicalize: air at index 0 when the chunk contains any air.
        if (airIndex > 0) {
            std::swap(palette[0], palette[airIndex]);
            const Uint16 a = (Uint16)airIndex;
            for (Uint16& li : local) {
                if (li == 0)      li = a;
                else if (li == a) li = 0;
            }
        }

        setBits(widthFor(palette.size()));          // allocates data (or none if 0 bits)
        if (bits) for (int i = 0; i < VOLUME; ++i) setIndexAt(i, local[i]);
    }

    Uint16 getId(int x, int y, int z) const { return palette[indexAt(lin(x, y, z))].id; }
    Uint16 getState(int x, int y, int z) const { return palette[indexAt(lin(x, y, z))].state; }

    // ONE bit-unpack for (id, state) instead of two. Use this
    // anywhere both halves of a cell are needed (fluids, meshing, edits).
    PaletteEntry getCell(int x, int y, int z) const {
        return palette[indexAt(lin(x, y, z))];
    }

    // read-only palette access, so callers can resolve per-ENTRY
    // data (Block*, transparency, "is air") once per chunk instead of once
    // per cell. Palettes are tiny (median 2 in the baseline run).
    const std::vector<PaletteEntry>& getPalette() const { return palette; }

    // True when every cell is palette[0] (0 bits/cell). Cheap emptiness /
    // uniformity test: uniform air is isUniform() && getPalette()[0].id == 0.
    bool isUniform() const { return bits == 0; }

    // bulk-decode ALL cell indices into out[VOLUME] (x-major, same
    // linear order as fromDenseIds). One sequential walk over the packed
    // words — replaces 4096 independent div/mod/shift indexAt() calls in the
    // mesher's hot loop.
    void decodeIndices(Uint16* out) const {
        if (bits == 0) {
            std::fill(out, out + VOLUME, (Uint16)0);
            return;
        }
        const uint64_t mask = (uint64_t(1) << bits) - 1;
        int i = 0;
        for (size_t w = 0; w < data.size() && i < VOLUME; ++w) {
            uint64_t word = data[w];
            for (int k = 0; k < perWord && i < VOLUME; ++k) {
                out[i++] = (Uint16)(word & mask);
                word >>= bits;
            }
        }
    }

    void set(int x, int y, int z, Uint16 id, Uint16 state = 0) {
        int idx = findOrAdd(PaletteEntry{ id, state });   // may widen + repack
        setIndexAt(lin(x, y, z), idx);
    }

    // ---- Stats ----
    size_t  paletteSize() const { return palette.size(); }   // distinct (id,state) combos
    size_t  distinctIds() const {                            // distinct ids ignoring state
        size_t n = 0;
        for (size_t i = 0; i < palette.size(); ++i) {
            bool seen = false;
            for (size_t j = 0; j < i; ++j) if (palette[j].id == palette[i].id) { seen = true; break; }
            if (!seen) ++n;
        }
        return n;
    }
    uint8_t bitsPerIndex() const { return bits; }
    size_t  memoryBytes() const {
        return sizeof(*this) + palette.capacity() * sizeof(PaletteEntry) + data.capacity() * sizeof(uint64_t);
    }

    // Drop palette entries no longer referenced and shrink the index width.
    // Call after a batch of edits if you care about reclaiming space.
    void recompact() {
        std::vector<int> used(palette.size(), 0);
        for (int i = 0; i < VOLUME; ++i) used[indexAt(i)] = 1;

        std::vector<int> remap(palette.size(), -1);
        std::vector<PaletteEntry> np;
        for (int p = 0; p < (int)palette.size(); ++p)
            if (used[p]) { remap[p] = (int)np.size(); np.push_back(palette[p]); }
        if (np.empty()) np.push_back(PaletteEntry{ 0, 0 });

        std::vector<int> local(VOLUME);
        for (int i = 0; i < VOLUME; ++i) local[i] = remap[indexAt(i)];

        palette.swap(np);
        bits = 255;                                  // force setBits to rebuild
        setBits(widthFor(palette.size()));
        if (bits) for (int i = 0; i < VOLUME; ++i) setIndexAt(i, local[i]);
    }

private:
    std::vector<PaletteEntry> palette;
    std::vector<uint64_t>     data;        // packed local indices, no straddle across words
    uint8_t bits = 0;                   // bits per index (0 when palette size <= 1)
    int     perWord = 0;                   // indices per 64-bit word (0 when bits == 0)

    static int lin(int x, int y, int z) { return (x * CHUNK_SIZE + y) * CHUNK_SIZE + z; }

    static uint8_t widthFor(size_t count) {
        for (int i = 0; i < PaletteConfig::kNumWidths; ++i)
            if ((size_t(1) << PaletteConfig::kWidths[i]) >= count)
                return PaletteConfig::kWidths[i];
        return PaletteConfig::kWidths[PaletteConfig::kNumWidths - 1];
    }

    int indexAt(int linear) const {
        if (bits == 0) return 0;                     // single-entry palette
        int word = linear / perWord;
        int shift = (linear % perWord) * bits;
        uint64_t mask = (uint64_t(1) << bits) - 1;
        return (int)((data[word] >> shift) & mask);
    }
    void setIndexAt(int linear, int value) {
        if (bits == 0) return;
        int word = linear / perWord;
        int shift = (linear % perWord) * bits;
        uint64_t mask = (uint64_t(1) << bits) - 1;
        data[word] = (data[word] & ~(mask << shift)) | ((uint64_t(value) & mask) << shift);
    }

    void setBits(uint8_t newBits) {
        if (newBits == bits) return;
        std::vector<int> snapshot(VOLUME);
        for (int i = 0; i < VOLUME; ++i) snapshot[i] = indexAt(i);  // old layout
        bits = newBits;
        perWord = newBits ? (64 / newBits) : 0;
        data.assign(newBits ? (VOLUME + perWord - 1) / perWord : 0, 0ull);
        if (newBits) for (int i = 0; i < VOLUME; ++i) setIndexAt(i, snapshot[i]);
    }

    int findOrAdd(const PaletteEntry& e) {
        for (int p = 0; p < (int)palette.size(); ++p)
            if (palette[p] == e) return p;
        int idx = (int)palette.size();
        palette.push_back(e);
        uint8_t need = widthFor(palette.size());
        if (need > bits) setBits(need);              // widen + repack existing cells
        return idx;
    }
};