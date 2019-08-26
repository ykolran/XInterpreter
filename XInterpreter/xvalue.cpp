#include "stdafx.h"
#include "XValue.h"
#include "XObject.h"

Value::Value(const char* c, unsigned int len) : 
	type(ValueType::OBJ), 
	obj(new ObjString(c, len)) 
{
}

Value::Value(const std::string& str) : 
	type(ValueType::OBJ), 
	obj(new ObjString(str.c_str(), str.length())) 
{
}

Value::Value(std::shared_ptr<ObjFunction> fun) :
	type(ValueType::OBJ),
	obj(fun)
{
}

Value::Value(std::shared_ptr<ObjNative> fun) :
	type(ValueType::OBJ),
	obj(fun)
{}
	

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

Value::operator ObjColumn&()
{ 
	return static_cast<ObjColumn&>(*obj.get()); 
}

Value::operator const ObjColumn&() const
{
	return static_cast<const ObjColumn&>(*obj.get());
}

Value::operator const ObjFunction&() const
{
	return static_cast<const ObjFunction&>(*obj.get());
}

Value::operator const ObjNative&() const
{
	return static_cast<const ObjNative&>(*obj.get());
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

bool Value::isNative() const
{
	return isObj() && obj->isNative();
}

std::shared_ptr<ObjNative> Value::asNative()
{ 
	return std::static_pointer_cast<ObjNative>(obj); 
}

std::shared_ptr<ObjFunction> Value::asFunction()
{
	return std::static_pointer_cast<ObjFunction>(obj);
}