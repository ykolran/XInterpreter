#include "stdafx.h"
#include "xvm.h"
#include "XInterpreterDlg.h"
#include "xcompiler.h"
#include "xobject.h"
#include <algorithm>

InterpretResult XVM::interpret(const char* source) 
{
	XCompiler compiler(source, FunctionType::SCRIPT);
	CallFrame frame;
	frame.function = compiler.compile();
	if (!frame.function) {
		return InterpretResult::COMPILE_ERROR;
	}
	frame.ip = frame.function->m_chunk.begin();
	frame.slots = m_stack.begin();
	m_file = nullptr;
	m_frames.push_back(frame);

	return run();
}

inline double	add		(double a, double b) { return a + b; };
inline double	subtract(double a, double b) { return a - b; };
inline double	multiply(double a, double b) { return a * b; };
inline double	divide	(double a, double b) { return a / b; };
inline bool		greater	(double a, double b) { return a > b; };
inline bool		less	(double a, double b) { return a < b; };

template<typename OP>
InterpretResult XVM::binaryOp(OP op)
{
	const Value& bPeek = peek(0);
	const Value& aPeek = peek(1);

	if (op == reinterpret_cast<OP>(add))
	{
		if (bPeek.isString() && aPeek.isString())
		{
			std::string b = pop();
			std::string a = pop();
			push(a + b);
			return InterpretResult::OK;
		}
	}
	if (bPeek.isNumber() && aPeek.isNumber())
	{
		Value b = pop();
		Value a = pop();
		push(op(a.asDouble(), b.asDouble()));
		return InterpretResult::OK;
	}
	else if (bPeek.isColumn() && aPeek.isColumn())
	{
		int len = bPeek.asColumn().length;

		if (len == aPeek.asColumn().length)
		{
			Value b = pop();
			Value a = pop();

			Value result(ColumnLength{ len });
			for (int i = 0; i < len; i++)
				result.asColumn().data[i] = op(a.asColumn().data[i], b.asColumn().data[i]);
			push(result);
			return InterpretResult::OK;
		}
		else if (len == 1)
		{
			Value b = pop();
			Value a = pop();

			Value result(ColumnLength{ aPeek.asColumn().length });
			for (int i = 0; i < aPeek.asColumn().length; i++)
				result.asColumn().data[i] = op(a.asColumn().data[i], b.asColumn().data[0]);
			push(result);
			return InterpretResult::OK;
		}
		else if (aPeek.asColumn().length == 1)
		{
			Value b = pop();
			Value a = pop();

			Value result(ColumnLength{ len });
			for (int i = 0; i < len; i++)
				result.asColumn().data[i] = op(a.asColumn().data[0], b.asColumn().data[i]);
			push(result);
			return InterpretResult::OK;
		}
		else
		{
			runtimeError("Vectors must be the same length.");
			return InterpretResult::RUNTIME_ERROR;
		}
	}
	else if (bPeek.isColumn() && aPeek.isNumber())
	{
		Value b = pop();
		Value a = pop();

		Value result(ColumnLength{ b.asColumn().length });
		for (int i = 0; i < b.asColumn().length; i++)
			result.asColumn().data[i] = op(a.asDouble(), b.asColumn().data[i]);
		push(result);
		return InterpretResult::OK;
	}
	else if (bPeek.isNumber() && aPeek.isColumn())
	{
		Value b = pop();
		Value a = pop();

		Value result(ColumnLength{ a.asColumn().length });
		for (int i = 0; i < a.asColumn().length; i++)
			result.asColumn().data[i] = op(a.asColumn().data[i], b.asDouble());
		push(result);
		return InterpretResult::OK;
	}
	else
	{
		if (op == reinterpret_cast<OP>(add))
			runtimeError("Operands must be numbers, vectors, or strings.");
		else
			runtimeError("Operands must be numbers or vectors.");
		return InterpretResult::RUNTIME_ERROR;
	}

}


InterpretResult XVM::run() 
{
#ifdef DEBUG_TRACE_EXECUTION                       
	DLG()->m_executionTrace.AppendFormat("== run ==\r\n");
#endif
	for (;;) {
#ifdef DEBUG_TRACE_EXECUTION                                        
		DLG()->m_executionTrace.AppendFormat("          ");
		for (const auto& value : m_stack) {
			DLG()->m_executionTrace.AppendFormat("[");
			printValue(DLG()->m_executionTrace, value);
			DLG()->m_executionTrace.AppendFormat("]");
		}
		DLG()->m_executionTrace.AppendFormat("\r\n");
		chunk().disassembleInstruction(ip(), true);
#endif  
		OpCode instruction = readOpCode();
		InterpretResult result = InterpretResult::OK;

		switch (instruction) 
		{
		case OpCode::CONSTANT:	push(readConstant());		 break;
		case OpCode::_TRUE:		push(true);					 break;
		case OpCode::_FALSE:	push(false);				 break;
		case OpCode::POP:		pop();						 break;
		case OpCode::GET_LOCAL: 
		{
			uint8_t slot = readByte();
			push(m_frames.back().slots[slot]);
			break;
		}
		case OpCode::SET_LOCAL: 
		{
			uint8_t slot = readByte();
			m_frames.back().slots[slot] = peek(0);
			break;
		}
		case OpCode::GET_GLOBAL:
		{
			const std::string& name = readConstant();
			auto it = globals.find(name);
			if (it == globals.end())
			{
				runtimeError("Undefined variable '%s'.", name.c_str());
				return InterpretResult::RUNTIME_ERROR;
			}
			push(it->second);
			break;
		}
		case OpCode::DEFINE_GLOBAL: 
		{
			const Value& c = readConstant();
			if (globals.find(c) == globals.end())
				globals.emplace(c, peek(0));
			else
				globals.at(c) = peek(0);
			pop();
			break;
		}
		case OpCode::SET_GLOBAL: 
		{
			const Value& value = readConstant();
			const std::string& name(value);
			auto it = globals.find(name);
			if (it == globals.end())
			{
				runtimeError("Undefined variable '%s'.", name.c_str());
				return InterpretResult::RUNTIME_ERROR;
			}
			else
			{
				globals.at(name) = peek(0);
			}
			break;
		}
		case OpCode::FILE:
		{
			intptr_t filePtr = 0;
			for (int i = 0; i<sizeof(intptr_t); i++)
				filePtr |= static_cast<intptr_t>(*ip()++) << i * 8;

			m_file = reinterpret_cast<CLogDataFile*>(filePtr);
			break;
		}
		case OpCode::GET_COLUMN:
		{
			uint8_t slot = readByte();
			push(Value(ColumnLength{m_file->m_sizeFilled}, m_file->m_data[slot]));
			break;
		}
		case OpCode::EQUAL:
		{
			Value b = pop();
			Value a = pop();
			if (b.isBool() && a.isBool())
				push(a.asBool() == b.asBool());
			else if (b.isNumber() && a.isNumber())
				push(a.asDouble() == b.asDouble());
			else if (b.isString() && a.isString())
				push(a.asString() == b.asString());
			else if (b.isColumn() && a.isColumn())
			{
				bool equals = (b.asColumn().length == a.asColumn().length);
				for (int i=0; i<b.asColumn().length && equals; i++)
					equals = (b.asColumn().data[i] == a.asColumn().data[i]);
				push(equals);
			}
			break;
		}
		case OpCode::ADD:		result = binaryOp(add);		 break;
		case OpCode::SUBTRACT:	result = binaryOp(subtract); break;
		case OpCode::MULTIPLY:	result = binaryOp(multiply); break;
		case OpCode::DIVIDE:	result = binaryOp(divide);	 break;
		case OpCode::GREATER:	result = binaryOp(greater);	 break;
		case OpCode::LESS:		result = binaryOp(less);	 break;
		case OpCode::NOT:		push(!pop().asBool());		 break;
		case OpCode::NEGATE:	result = negate();			 break;
		case OpCode::PRINT:		
			printValue(DLG()->m_output, pop());
			DLG()->m_output.AppendFormat("\r\n");
			break;
		case OpCode::JUMP: 
		{
			uint16_t offset = readShort();
			ip() += offset;
			break;
		}
		case OpCode::JUMP_IF_FALSE: 
		{
			uint16_t offset = readShort();
			if (!peek(0)) 
				ip() += offset;
			break;
		}
		case OpCode::LOOP: 
		{
			uint16_t offset = readShort();
			ip() -= offset;
			break;
		}
		case OpCode::RETURN:
			return InterpretResult::OK;
		}

		if (result != InterpretResult::OK)
			return result;
	}
}

InterpretResult XVM::negate()
{
	if (!peek(0).isNumber())
	{
		runtimeError("Operand must be a number.");
		return InterpretResult::RUNTIME_ERROR;
	}

	push(-pop().asDouble());
	return InterpretResult::OK;
}

void XVM::printObj(CString& out, Value value) const
{
	switch (value.obj->type)
	{
	case ObjType::FUNCTION:
		if (value.asFunction().m_name.empty())
			out.Append("<script>");
		else
			out.AppendFormat("<fn %s>", value.asFunction().m_name.c_str());
		break;
	case ObjType::STRING: 
		out.Append(value.asString().c_str());
		break;
	case ObjType::OWNING_COLUMN:
	case ObjType::SHARED_COLUMN:
		for (int i = 0; i < std::min(value.asColumn().length, 2); i++)
			out.AppendFormat("%g ", value.asColumn().data[i]);
		if (value.asColumn().length > 2)
			out.Append("...");
		break;
	}
}

void XVM::printValue(CString& out, Value value) const
{ 
	switch (value.type) 
	{
	case ValueType::BOOL:   
		if (value)
			out.Append("true");
		else
			out.Append("false");
		break;
	case ValueType::NUMBER: 
		out.AppendFormat("%g", value.asDouble()); 
		break;
	case ValueType::OBJ:	
		printObj(out, value); 
		break;
	}
}


void XVM::runtimeError(const char* format, ...) 
{
	va_list args;
	va_start(args, format);
	DLG()->m_output.AppendFormatV(format, args);
	va_end(args);
	DLG()->m_output.AppendFormat("\r\n");

	DLG()->m_output.AppendFormat("[line %d] in script\r\n",
		chunk().getLine(ip()));

	m_stack.clear();
}