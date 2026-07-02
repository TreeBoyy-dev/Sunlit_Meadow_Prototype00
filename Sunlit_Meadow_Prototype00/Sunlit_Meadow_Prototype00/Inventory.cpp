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

bool Inventory::addItemToInventory(ItemInstance newItems, int slot) {
    if (newItems.isEmpty()) {
        SDL_Log("[Inventory] no Items to add");
        return false;
    }

    // --- specific slot requested ---
    if (slot != -1) {
        auto it = items.find(slot);
        if (it != items.end() && !it->second.isEmpty()) {
            SDL_Log("[Inventory] Items not added - slot already occupied");
            return false;
        }
        items[slot] = newItems;
        return true;
    }

    // --- no slot given: merge into an existing stack of the same item ---
    int existing = hasItem(newItems.item->getID());
    if (existing != -1) {
        items[existing].count += newItems.count;
        return true;
    }

    // --- otherwise add a new entry in the lowest available slot ---
    int free = 0;
    while (items.find(free) != items.end())
        ++free;
    items[free] = newItems;
    return true;
}

ItemInstance Inventory::takeItems(int slot, int amount) {
    auto it = items.find(slot);
    if (it == items.end())
        return ItemInstance{};   // nothing in this slot

    ItemInstance& stack = it->second;

    // take everything (amount == -1 or more than is there)
    if (amount < 0 || amount >= stack.count) {
        ItemInstance taken = stack;
        items.erase(it);         // keep empty slots out of the map
        return taken;
    }

    // take only part of the stack
    ItemInstance taken{ stack.item, static_cast<short int>(amount) };
    stack.count -= static_cast<short int>(amount);
    return taken;
}

int Inventory::hasItem(Uint16 id) {
    for (const auto& [slot, instance] : items) {
        if (!instance.isEmpty() && instance.item->getID() == id)
            return slot;
    }
    return -1;
}

void Inventory::printContents() {
    if (items.empty()) {
        SDL_Log("[Inventory] DEBUG: empty");
        return;
    }

    for (const auto& [slot, instance] : items) {
        if (instance.isEmpty())
            continue;   // skip stray empty entries

        SDL_Log("[Inventory] DEBUG: item: %s(%d) | count: %d | slot: %d",
            instance.item->getName().c_str(),
            instance.item->getID(),
            instance.count,
            slot);
    }
}