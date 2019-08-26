#pragma once
#include <stdint.h>

enum class XOpCode : uint8_t
{
	CONSTANT,
	NIL,
	_TRUE,
	_FALSE,
	POP,
	GET_LOCAL,
	SET_LOCAL,
	GET_GLOBAL,
	DEFINE_GLOBAL,
	SET_GLOBAL,
	EQUAL,
	GREATER,
	LESS,
	ADD,
	SUBTRACT,
	MULTIPLY,
	DIVIDE,
	NOT,
	NEGATE,
	PRINT,
	JUMP,
	JUMP_IF_FALSE,
	LOOP,
	CALL,
	RETURN
};