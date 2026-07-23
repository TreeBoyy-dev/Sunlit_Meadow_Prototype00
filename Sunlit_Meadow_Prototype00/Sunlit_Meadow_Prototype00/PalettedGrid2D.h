#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include <cstdint>

#include "PalettedContainer.h"   // PaletteConfig::kWidths — the ONE place to retune bit widths

// =====================================================================
//  PalettedGrid2D
//  2D sibling of PalettedContainer: stores width x width cells of
//  Uint16 ids as bit-packed local indices into a small palette.
//
//   - A uniform grid collapses to a 1-entry palette -> 0 bits/cell
//     -> 0 bytes of cell data — same trick as air chunks. With the
//     current single-biome placeholder every region map costs almost
//     nothing.
//   - Palette entries are plain Uint16 ids (no state field).
//   - Runtime-sized (the region maps pass MAP_GRID_SIZE = 136), unlike
//     the compile-time 16^3 of PalettedContainer.
//
//  Cell layout is x-major to match the engine's convention:
//      linear = cx * width + cy
//
//  There is deliberately NO set(): the region maps are immutable after
//  construction and are read lock-free from the generator worker
//  thread. Leaving mutation out keeps that threading contract honest —
//  if a mutable map is ever needed, it must grow its own locking story
//  first.
// =====================================================================

class PalettedGrid2D {
public:
    explicit PalettedGrid2D(int width)
        : m_width(width)
    {
        palette.push_back(0);   // default: all id 0, 0 bits, 0 data bytes
    }

    // ---- Build from a dense width*width array of ids ----
    // denseIds length must be width*width, indexed as cx * width + cy.
    void fromDense(const Uint16* denseIds) {
        const int volume = m_width * m_width;

        palette.clear();
        data.clear();
        bits = 0; perWord = 0;

        std::vector<int> local(volume);
        for (int i = 0; i < volume; ++i) {
            Uint16 id = denseIds[i];
            int li = -1;
            for (int p = 0; p < (int)palette.size(); ++p)
                if (palette[p] == id) { li = p; break; }
            if (li < 0) { li = (int)palette.size(); palette.push_back(id); }
            local[i] = li;
        }
        if (palette.empty()) palette.push_back(0);

        setBits(widthFor(palette.size()));          // allocates data (or none if 0 bits)
        if (bits) for (int i = 0; i < volume; ++i) setIndexAt(i, local[i]);
    }

    Uint16 get(int cx, int cy) const {
        if (cx < 0 || cx >= m_width ||
            cy < 0 || cy >= m_width) {
            SDL_Log("[PalettedGrid2D] cell out of range: %d|%d (width %d)",
                cx, cy, m_width);
            return 0;
        }
        return palette[indexAt(cx * m_width + cy)];
    }

    // ---- Stats ----
    int     width()       const { return m_width; }
    size_t  paletteSize() const { return palette.size(); }
    size_t  memoryBytes() const {
        return sizeof(*this) + palette.capacity() * sizeof(Uint16) + data.capacity() * sizeof(uint64_t);
    }

private:
    int m_width;

    std::vector<Uint16>   palette;
    std::vector<uint64_t> data;        // packed local indices, no straddle across words
    uint8_t bits = 0;                  // bits per index (0 when palette size <= 1)
    int     perWord = 0;               // indices per 64-bit word (0 when bits == 0)

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

    // Only ever called once per fromDense (bits starts at 0 there), but kept
    // in PalettedContainer's shape so the two stay easy to compare.
    void setBits(uint8_t newBits) {
        if (newBits == bits) return;
        const int volume = m_width * m_width;
        std::vector<int> snapshot(volume);
        for (int i = 0; i < volume; ++i) snapshot[i] = indexAt(i);  // old layout
        bits = newBits;
        perWord = newBits ? (64 / newBits) : 0;
        data.assign(newBits ? (volume + perWord - 1) / perWord : 0, 0ull);
        if (newBits) for (int i = 0; i < volume; ++i) setIndexAt(i, snapshot[i]);
    }
};
