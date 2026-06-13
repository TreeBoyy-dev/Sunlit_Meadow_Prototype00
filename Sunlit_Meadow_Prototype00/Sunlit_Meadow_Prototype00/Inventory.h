#pragma once
#include "Item.h"
#include "Data.h"

#include <unordered_map>

class Inventory : public Data{
protected:
	std::unordered_map<int, ItemInstance> items;

public:
	Inventory(Datatype datatype) : Data(datatype) {};

	ItemInstance getItemsFromSlot  (int slot);
	ItemInstance setItemsToSlot    (ItemInstance items, int slot);

	bool		 addItemToInventory(ItemInstance items, int slot = -1);
	ItemInstance takeItems         (int slot, int amount = -1);

	//returns the slot a type of item is or -1 if none is present
	int			 hasItem         (Uint16 id);
};