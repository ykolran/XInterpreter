#include "stdafx.h"
#include "xvalue.h"
#include "xobject.h"

Value::Value(const char* c, int len) : 
	type(ValueType::OBJ), 
	obj(new ObjString(c, len)) 
{
}

Value::Value(const std::string& str) : 
	type(ValueType::OBJ), 
	obj(new ObjString(str.c_str(), str.length())) 
{
}

Value::Value(ColumnLength len, double* c) : 
	type(ValueType::OBJ), 
	obj(new ObjColumn(len, c)) 
{
}

Value::Value(ColumnLength len) : 
	type(ValueType::OBJ), 
	obj(new ObjColumn(len)) 
{
}

Value::operator const ObjColumn&() const 
{ 
	return static_cast<const ObjColumn&>(*obj.get()); 
}

Value::operator const ObjFunction&() const
{
	return static_cast<const ObjFunction&>(*obj.get());
}

Value::operator const std::string&() const
{ 
	return static_cast<const ObjString*>(obj.get())->str; 
}

bool Value::isString() const 
{ 
	return isObj() && obj->isString(); 
}

bool Value::isColumn() const 
{ 
	return isObj() && obj->isColumn(); 
}

bool Value::isFunction() const
{
	return isObj() && obj->isFunction();
}
