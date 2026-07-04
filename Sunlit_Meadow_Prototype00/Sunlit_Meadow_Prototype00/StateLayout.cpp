#include "StateLayout.h"

Uint8 statePropTypeBits(StatePropType type) {
    switch (type) {
    case StatePropType::Rot4:      return 2;
    case StatePropType::Axis3:     return 2;
    case StatePropType::Rot6:      return 3;
    case StatePropType::Half:      return 1;
    case StatePropType::Shape5:    return 3;
    case StatePropType::Connect4:  return 4;
    case StatePropType::WallSide4: return 8;
    case StatePropType::Flag:      return 1;
    default:                       return 0;
    }
}

StatePropType statePropTypeFromName(const std::string& name) {
    if (name == "rot4")      return StatePropType::Rot4;
    if (name == "axis3")     return StatePropType::Axis3;
    if (name == "rot6")      return StatePropType::Rot6;
    if (name == "half")      return StatePropType::Half;
    if (name == "shape5")    return StatePropType::Shape5;
    if (name == "connect4")  return StatePropType::Connect4;
    if (name == "wallSide4") return StatePropType::WallSide4;
    if (name == "flag")      return StatePropType::Flag;
    return StatePropType::Invalid;
}

bool StateLayout::addProperty(const std::string& name, StatePropType type) {
    Uint8 bits = statePropTypeBits(type);
    if (bits == 0) {
        SDL_Log("[StateLayout] unknown property type for '%s'", name.c_str());
        return false;
    }
    // Template bits may only use 0..14; bit 15 is the global fluid bit.
    if (usedBits + bits > 15) {
        SDL_Log("[StateLayout] property '%s' (%u bits) overflows past bit 14 "
                "(already %u bits used, bit 15 is reserved for fluids)",
                name.c_str(), bits, usedBits);
        return false;
    }
    StateProperty p;
    p.name = name;
    p.type = type;
    p.bitOffset = usedBits;
    p.bitCount = bits;
    props.push_back(p);
    usedBits += bits;
    return true;
}

Uint16 StateLayout::get(Uint16 state, int propIndex) const {
    if (propIndex < 0 || propIndex >= (int)props.size()) return 0;
    const StateProperty& p = props[propIndex];
    Uint16 mask = (Uint16)((1u << p.bitCount) - 1u);
    return (Uint16)((state >> p.bitOffset) & mask);
}

Uint16 StateLayout::set(Uint16 state, int propIndex, Uint16 value) const {
    if (propIndex < 0 || propIndex >= (int)props.size()) return state;
    const StateProperty& p = props[propIndex];
    Uint16 mask = (Uint16)(((1u << p.bitCount) - 1u) << p.bitOffset);
    return (Uint16)((state & ~mask) | ((Uint16)(value << p.bitOffset) & mask));
}

int StateLayout::indexOf(const std::string& name) const {
    for (int i = 0; i < (int)props.size(); ++i)
        if (props[i].name == name) return i;
    return -1;
}

Uint16 StateLayout::modelMask() const {
    return (Uint16)((1u << usedBits) - 1u);
}
