#pragma once

enum Datatype {
	INVENTORY,
};

class Data {
protected:
	Datatype datatype;
	Datatype getDatatype() { return datatype; };

public:
	Data(Datatype datatype) : datatype(datatype) {};
};