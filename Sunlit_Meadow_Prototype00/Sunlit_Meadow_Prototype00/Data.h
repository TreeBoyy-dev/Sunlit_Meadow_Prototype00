#pragma once

#include <vector>

enum Datatype {
	INVENTORY,
    ENTITY,
};

class Data {
protected:
	Datatype datatype;

public:
	Data(Datatype datatype) : datatype(datatype) {};
    Datatype getDatatype() { return datatype; };
};

inline Data* findByType(std::vector<Data*>* list, Datatype type) {
    if (list == nullptr)
        return nullptr;

    for (Data* element : *list) {
        if (element != nullptr && element->getDatatype() == type)
            return element;
    }
    return nullptr;
}