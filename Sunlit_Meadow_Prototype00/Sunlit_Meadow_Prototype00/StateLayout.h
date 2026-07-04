#pragma once
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

#include <string>
#include <vector>

// =====================================================================
//  StateLayout
//
//  A block's state is a Uint16, packed like this:
//
//    bit:   15 | 14 ........................ 0
//           ^    ^
//           |    template-defined properties, allocated upward from bit 0
//           fluid bit (global, every block, always bit 15)
//
//  Template properties are declared in the template JSON ("states": [...])
//  and get their bits assigned automatically at load time, in declaration
//  order, growing from bit 0 upward. The fluid bit is pinned at the
//  opposite end (MSB) so it can never collide with template bits.
//
//  `state & ~STATE_FLUID_MASK` cheaply strips the fluid bit when looking
//  up model variants.
// =====================================================================

// Bit 15: "a fluid is in this cell". Which fluid and what level live in the
// chunk's separate fluid PalettedContainer, NOT in the block state.
constexpr Uint16 STATE_FLUID_MASK = 0x8000;

// Property types a template may declare. Bit widths are fixed per type.
enum class StatePropType : Uint8 {
    Rot4,       // 2 bits: N/E/S/W horizontal facing        (stairs, furnaces)
    Axis3,      // 2 bits: Z(up)/X/Y axis                    (logs / pillars)
    Rot6,       // 3 bits: N/E/S/W/Up/Down                   (piston-like)
    Half,       // 1 bit : bottom/top                        (stairs, slabs)
    Shape5,     // 3 bits: straight, inner_l, inner_r, outer_l, outer_r (stair corners)
    Connect4,   // 4 bits: 1 bit per horizontal side N/E/S/W (fences)
    WallSide4,  // 8 bits: 2 bits per side: none/low/tall    (walls)
    Flag,       // 1 bit : generic on/off                    (wall center post, misc)

    Invalid
};

// Fixed bit width for each property type.
Uint8 statePropTypeBits(StatePropType type);
// "rot4" -> StatePropType::Rot4 etc. Returns Invalid for unknown names.
StatePropType statePropTypeFromName(const std::string& name);

struct StateProperty {
    std::string   name;
    StatePropType type = StatePropType::Invalid;
    Uint8         bitOffset = 0;
    Uint8         bitCount = 0;
};

class StateLayout {
private:
    std::vector<StateProperty> props;   // built from template JSON
    Uint8 usedBits = 0;                 // total template bits allocated so far

public:
    // Appends a property, allocating its bits directly above the previous
    // one. Returns false (and adds nothing) if the property would spill
    // into bit 15 (the fluid bit) or the type is unknown.
    bool addProperty(const std::string& name, StatePropType type);

    // Value of property `propIndex` inside `state` (shifted down to 0).
    Uint16 get(Uint16 state, int propIndex) const;
    // Returns `state` with property `propIndex` set to `value`.
    Uint16 set(Uint16 state, int propIndex, Uint16 value) const;

    // Index of a property by name, -1 if not present.
    // For setup code; hot paths should cache the returned index.
    int indexOf(const std::string& name) const;

    // Mask covering all template bits actually used (never includes the
    // fluid bit). `state & modelMask()` is the model-variant index.
    Uint16 modelMask() const;

    int  propertyCount() const { return (int)props.size(); }
    const StateProperty& property(int i) const { return props[i]; }
    Uint8 totalBits() const { return usedBits; }
};
