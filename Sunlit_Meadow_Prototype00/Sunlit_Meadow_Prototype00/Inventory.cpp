#include "Inventory.h"

ItemInstance Inventory::getItemsFromSlot(int slot) {
    auto it = items.find(slot);
    if (it == items.end())
        return ItemInstance{};   // empty instance (item == nullptr, count == 0)
    return it->second;
}

ItemInstance Inventory::setItemsToSlot(ItemInstance newItems, int slot) {
    ItemInstance previous{};

    auto it = items.find(slot);
    if (it != items.end())
        previous = it->second;

    if (newItems.isEmpty())
        items.erase(slot);       // keep empty slots out of the map
    else
        items[slot] = newItems;

    return previous;
}

int Inventory::hasItem(Uint16 id) {
    for (const auto& [slot, instance] : items) {
        if (!instance.isEmpty() && instance.item->getID() == id)
            return slot;
    }
    return -1;
}