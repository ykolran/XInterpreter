#pragma once
#include "XChunk.h"

struct Obj
{
	Obj(ObjType t) : type(t) {}

	Obj(const Obj&) = delete;
	Obj& operator=(const Obj&) = delete;

	bool isString() const { return type == ObjType::STRING; }
	bool isColumn() const { return type == ObjType::SHARED_COLUMN || type == ObjType::OWNING_COLUMN; }
	bool isFunction() const { return type == ObjType::FUNCTION; }
	bool isNative() const { return type == ObjType::NATIVE; }

	ObjType type;
};

struct ObjString : public Obj
{
	ObjString(const char* c, unsigned int len) :
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
		categorize(nullptr),
		length(cl.len),
		isBoolean(false)
	{
	}

	ObjColumn(ColumnLength cl, double* col) :
		Obj(ObjType::SHARED_COLUMN),
		data(col),
		categorize(nullptr),
		length(cl.len),
		isBoolean(false)
	{
	}

	~ObjColumn()
	{
		if (type == ObjType::OWNING_COLUMN)
			delete[] data;
	}

	double * data;
	ObjColumn * categorize;
	unsigned int length;
	bool isBoolean;
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

struct ObjNative : public Obj
{
	ObjNative(std::string name, int arity) :
		Obj(ObjType::NATIVE),
		m_name(name),
		m_arity(arity),
		m_success(false)
	{
	}

	virtual double operator()(std::vector<double>) = 0;
	std::string m_name;
	std::string m_error;
	int m_arity;
	bool m_success;
};