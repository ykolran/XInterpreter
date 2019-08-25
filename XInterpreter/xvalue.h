#pragma once
#include <string>
#include <vector>

enum class ValueType 
{
	BOOL,
	NUMBER,
	OBJ
};

enum class ObjType
{
	STRING,
	SHARED_COLUMN,
	OWNING_COLUMN,
	FUNCTION,
	NATIVE
};

struct ColumnLength
{
	int len;
};

struct Obj;
struct ObjColumn;
struct ObjString;
struct ObjFunction;
struct ObjNative;

struct Value 
{
	Value(bool b) : type(ValueType::BOOL), number(b) {}
	Value(double n) : type(ValueType::NUMBER), number(n) {}

	Value(const char* c, int len);
	Value(const std::string& str);
	Value(std::shared_ptr<ObjFunction> fun);
	Value(std::shared_ptr<ObjNative> fun);
	
	Value(ColumnLength len, double* c);
	Value(ColumnLength len);

	operator bool() const { return number != 0.0; }
	operator double() const { return number; }
	operator ObjColumn&();
	operator const ObjColumn&() const;
	operator const ObjFunction&() const;
	operator const ObjNative&() const;
	operator const std::string&() const;

	bool isBool() const { return type == ValueType::BOOL; }
	bool isNumber() const { return type == ValueType::NUMBER; }
	bool isObj() const { return type == ValueType::OBJ; }
	bool isString() const;
	bool isColumn() const;
	bool isFunction() const;
	bool isNative() const;

	bool asBool() const { return static_cast<bool>(*this); }
	double asDouble() const { return static_cast<double>(*this); }
	ObjColumn& asColumn() { return static_cast<ObjColumn&>(*this); }
	const ObjColumn& asColumn() const { return static_cast<const ObjColumn&>(*this); }
	std::shared_ptr<ObjFunction> asFunction();
	std::shared_ptr<ObjNative> asNative();
	const std::string& asString() const { return static_cast<const std::string&>(*this); }

	ValueType type;
	double number;
	std::shared_ptr<Obj> obj;
};
