#pragma once
#include "xchunk.h"

struct Obj
{
	Obj(ObjType t) : type(t) {}

	Obj(const Obj&) = delete;
	Obj& operator=(const Obj&) = delete;

	bool isString() const { return type == ObjType::STRING; }
	bool isColumn() const { return type == ObjType::SHARED_COLUMN || type == ObjType::OWNING_COLUMN; }
	bool isFunction() const { return type == ObjType::FUNCTION; }

	ObjType type;
};

struct ObjString : public Obj
{
	ObjString(const char* c, int len) :
		Obj(ObjType::STRING),
		str(c, len)
	{
	}

	std::string str;
};


struct ObjColumn : public Obj
{
	ObjColumn(ColumnLength cl) :
		Obj(ObjType::OWNING_COLUMN),
		data(new double[cl.len]),
		length(cl.len)
	{
	}

	ObjColumn(ColumnLength cl, double* col) :
		Obj(ObjType::SHARED_COLUMN),
		data(col),
		length(cl.len)
	{
	}

	~ObjColumn()
	{
		if (type == ObjType::OWNING_COLUMN)
			delete[] data;
	}

	double * data;
	int length;

};

struct ObjFunction : public Obj
{
	ObjFunction(std::string name, int arity) :
		Obj(ObjType::FUNCTION),
		m_name(name),
		m_arity(arity)
	{
	}

	std::string m_name;
	int m_arity;
	XChunk m_chunk;
};