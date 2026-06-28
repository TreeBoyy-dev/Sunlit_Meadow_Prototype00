#pragma once

#include <vector>
#include <memory>

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

inline Data* findByType(std::vector<std::unique_ptr<Data>>* list, Datatype type) {
    if (list == nullptr)
        return nullptr;
    for (const std::unique_ptr<Data>& element : *list) {
        if (element != nullptr && element->getDatatype() == type)
            return element.get();   // non-owning raw pointer back to caller
    }
    return nullptr;
}