#pragma once
#include "Item.h"
#include "Data.h"

#include <unordered_map>


class Inventory : public Data{
protected:
	std::unordered_map<ItemInstance, int> items;

public:
	Inventory(Datatype datatype) : Data(datatype) {};

	ItemInstance getItemsFromSlot(int slot);
	int			 setItemsToSlot  (ItemInstance items);

	int			 hasItem         (Uint16 id);
};