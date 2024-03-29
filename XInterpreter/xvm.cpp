#include "stdafx.h"
#include "XVM.h"
#include "XInterpreterDlg.h"
#include "XCompiler.h"
#include "XObject.h"
#include <algorithm>
#include <time.h>

struct ClockNative : public ObjNative
{
	ClockNative() : ObjNative("clock", 0)
	{
	}

	virtual double operator()(std::vector<double>)
	{
		m_success = true;
		return (double)clock() / CLOCKS_PER_SEC;
	}
};

struct RMS : public ObjNative
{
	RMS() : ObjNative("rms", 3)
	{
	}

	virtual double operator()(std::vector<double> in)
	{
		m_success = true;
		return sqrt(in[0]* in[0] + in[1] * in[1] + in[2] * in[2]);
	}
};

XVM::XVM()
{
	defineNative("clock", std::make_shared<ClockNative>());
	defineNative("rms", std::make_shared<RMS>());
}

InterpretResult XVM::interpret(const char* source, bool trace, bool printByteCode)
{
	XScanner scanner(source);
	XCompiler compiler(scanner, "", FunctionType::SCRIPT, printByteCode);
	std::shared_ptr<ObjFunction> function = compiler.compile();
	if (!function) {
		return InterpretResult::COMPILE_ERROR;
	}
	CallFrame frame(function, function->m_chunk.begin(), 0);
	m_frames.push_back(frame);

	return run(trace);
}

inline double	add		(double a, double b) { return a + b; };
inline double	subtract(double a, double b) { return a - b; };
inline double	multiply(double a, double b) { return a * b; };
inline double	divide	(double a, double b) { return a / b; };
inline bool		greater	(double a, double b) { return a > b; };
inline bool		less	(double a, double b) { return a < b; };
inline bool		equals	(double a, double b) { return a == b; };

using boolBinaryOpType = bool(*)(double, double);
using doubleBinaryOpType = double(*)(double, double);

inline bool isBooleanResult(boolBinaryOpType) { return true; }
inline bool isBooleanResult(doubleBinaryOpType) { return false; }

template<typename OP>
InterpretResult XVM::binaryOp(OP op)
{
	const Value& bPeek = peek(0);
	const Value& aPeek = peek(1);

	if (op == reinterpret_cast<OP>(equals))
	{
		if (bPeek.isBool() && aPeek.isBool())
		{
			Value b = pop();
			Value a = pop();
			return push(a.asBool() == b.asBool());
		}
		else if (bPeek.isString() && aPeek.isString())
		{
			Value b = pop();
			Value a = pop();
			return push(a.asString() == b.asString());
		}
	}
	if (op == reinterpret_cast<OP>(add))
	{
		if (bPeek.isString() && aPeek.isString())
		{
			std::string b = pop();
			std::string a = pop();
			return push(a + b);
		}
	}
	if (bPeek.isNumber() && aPeek.isNumber())
	{
		Value b = pop();
		Value a = pop();
		return push(op(a.asDouble(), b.asDouble()));
	}
	else if (bPeek.isColumn() && aPeek.isColumn())
	{
		unsigned int len = bPeek.asColumn().length;

		if (len == aPeek.asColumn().length)
		{
			Value b = pop();
			Value a = pop();

			Value result(ColumnLength{ len });
			for (unsigned int i = 0; i < len; i++)
				result.asColumn().data[i] = op(a.asColumn().data[i], b.asColumn().data[i]);

			if (a.asColumn().categorize != nullptr)
				result.asColumn().categorize = a.asColumn().categorize;
			else
				result.asColumn().categorize = b.asColumn().categorize;

			result.asColumn().isBoolean = isBooleanResult(op);
			return push(result);
		}
		else if (len == 1)
		{
			Value b = pop();
			Value a = pop();

			Value result(ColumnLength{ aPeek.asColumn().length });
			for (unsigned int i = 0; i < aPeek.asColumn().length; i++)
				result.asColumn().data[i] = op(a.asColumn().data[i], b.asColumn().data[0]);
			result.asColumn().categorize = aPeek.asColumn().categorize;
			result.asColumn().isBoolean = isBooleanResult(op);
			return push(result);
		}
		else if (aPeek.asColumn().length == 1)
		{
			Value b = pop();
			Value a = pop();

			Value result(ColumnLength{ len });
			for (unsigned int i = 0; i < len; i++)
				result.asColumn().data[i] = op(a.asColumn().data[0], b.asColumn().data[i]);
			result.asColumn().categorize = b.asColumn().categorize;
			result.asColumn().isBoolean = isBooleanResult(op);
			return push(result);
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
		for (unsigned int i = 0; i < b.asColumn().length; i++)
			result.asColumn().data[i] = op(a.asDouble(), b.asColumn().data[i]);
		result.asColumn().categorize = b.asColumn().categorize;
		result.asColumn().isBoolean = isBooleanResult(op);
		return push(result);
	}
	else if (bPeek.isNumber() && aPeek.isColumn())
	{
		Value b = pop();
		Value a = pop();

		Value result(ColumnLength{ a.asColumn().length });
		for (unsigned int i = 0; i < a.asColumn().length; i++)
			result.asColumn().data[i] = op(a.asColumn().data[i], b.asDouble());
		result.asColumn().categorize = a.asColumn().categorize;
		result.asColumn().isBoolean = isBooleanResult(op);
		return push(result);
	}
	else
	{
		if (op == reinterpret_cast<OP>(equals))
			runtimeError("Operands cannot be compared.");
		else if (op == reinterpret_cast<OP>(add))
			runtimeError("Operands must be numbers, vectors, or strings.");
		else
			runtimeError("Operands must be numbers or vectors.");
		return InterpretResult::RUNTIME_ERROR;
	}

}


InterpretResult XVM::run(bool trace) 
{
	if (trace)
		DLG()->m_executionTrace.AppendFormat("== run ==\r\n");

	for (;;)
	{
		if (trace)
		{
			DLG()->m_executionTrace.AppendFormat("          ");
			for (const auto& value : m_stack) {
				DLG()->m_executionTrace.AppendFormat("[");
				printValue(DLG()->m_executionTrace, value);
				DLG()->m_executionTrace.AppendFormat("]");
			}
			DLG()->m_executionTrace.AppendFormat("\r\n");
			chunk().disassembleInstruction(ip(), true);
		}
		XOpCode instruction = readOpCode();
		InterpretResult result = InterpretResult::OK;

		switch (instruction)
		{
		case XOpCode::CONSTANT:	result = push(readConstant());	break;
		case XOpCode::NIL:		result = push(0.0);				break;
		case XOpCode::_TRUE:	result = push(true);			break;
		case XOpCode::_FALSE:	result = push(false);			break;
		case XOpCode::POP:		pop();							break;
		case XOpCode::GET_LOCAL:
		{
			uint8_t slot = readByte();
			result = push(m_stack[m_frames.back().slots + slot]);
			break;
		}
		case XOpCode::SET_LOCAL:
		{
			uint8_t slot = readByte();
			m_stack[m_frames.back().slots + slot] = peek(0);
			break;
		}
		case XOpCode::CATEGORIZE:
		{
			uint8_t numColumns = readByte();
			Value& categories = m_stack[m_stack.size() - 1];
			if (!categories.isColumn())
			{
				runtimeError("Categorize did not evaluate to a column.");
				result = InterpretResult::RUNTIME_ERROR;
				break;
			}

			for (int i = 0; i < numColumns; i++)
			{
				Value& constant = m_stack[m_stack.size() - 2 - i];
				if (categories.asColumn().length != constant.asColumn().length)
				{
					runtimeError("Categorize length does not match.");
					result = InterpretResult::RUNTIME_ERROR;
					break;
				}

				constant.asColumn().categorize = &categories.asColumn();
			}
			break;
		}
		case XOpCode::GET_GLOBAL:
		{
			const std::string& name = readConstant();
			auto it = m_globals.find(name);
			if (it == m_globals.end())
			{
				runtimeError("Undefined variable '%s'.", name.c_str());
				return InterpretResult::RUNTIME_ERROR;
			}
			result = push(it->second);
			break;
		}
		case XOpCode::DEFINE_GLOBAL:
		{
			const Value& c = readConstant();
			if (m_globals.find(c) == m_globals.end())
				m_globals.emplace(c, peek(0));
			else
				m_globals.at(c) = peek(0);
			pop();
			break;
		}
		case XOpCode::SET_GLOBAL:
		{
			const Value& value = readConstant();
			const std::string& name(value);
			auto it = m_globals.find(name);
			if (it == m_globals.end())
			{
				runtimeError("Undefined variable '%s'.", name.c_str());
				return InterpretResult::RUNTIME_ERROR;
			}
			else
			{
				m_globals.at(name) = peek(0);
			}
			break;
		}
		case XOpCode::EQUAL:	result = binaryOp(equals);	 break;
		case XOpCode::ADD:		result = binaryOp(add);		 break;
		case XOpCode::SUBTRACT:	result = binaryOp(subtract); break;
		case XOpCode::MULTIPLY:	result = binaryOp(multiply); break;
		case XOpCode::DIVIDE:	result = binaryOp(divide);	 break;
		case XOpCode::GREATER:	result = binaryOp(greater);	 break;
		case XOpCode::LESS:		result = binaryOp(less);	 break;
		case XOpCode::NOT:		result = not();				 break;
		case XOpCode::NEGATE:	result = negate();			 break;
		case XOpCode::PRINT:
			printValue(DLG()->m_output, pop());
			DLG()->m_output.AppendFormat("\r\n");
			break;
		case XOpCode::JUMP:
		{
			uint16_t offset = readShort();
			ip() += offset;
			break;
		}
		case XOpCode::JUMP_IF_FALSE:
		{
			uint16_t offset = readShort();
			if (!peek(0))
				ip() += offset;
			break;
		}
		case XOpCode::LOOP:
		{
			uint16_t offset = readShort();
			ip() -= offset;
			break;
		}
		case XOpCode::CALL:
		{
			int argCount = readByte();
			result = callValue(peek(argCount), argCount);
			break;
		}
		case XOpCode::RETURN:
		{
			Value funResult = pop();
			int currentSlots = m_frames.back().slots;

			m_frames.pop_back();
			if (m_frames.empty())
				return InterpretResult::OK;

			m_stack.erase(m_stack.begin() + currentSlots, m_stack.end());
			result = push(funResult);
			break;
		}
		}
		
		if (result != InterpretResult::OK)
			return result;
	}
}

InterpretResult XVM::negate()
{
	if (peek(0).isNumber())
	{
		return push(-pop().asDouble());
	}
	else if (peek(0).isColumn() && !peek(0).asColumn().isBoolean)
	{
		Value a = pop();
		Value neg(ColumnLength{ a.asColumn().length });
		for (unsigned int i = 0; i < neg.asColumn().length; i++)
			neg.asColumn().data[i] = -a.asColumn().data[i];
		neg.asColumn().categorize = a.asColumn().categorize;
		return push(neg);
	}

	runtimeError("Operand must be a number or number column.");
	return InterpretResult::RUNTIME_ERROR;
}

InterpretResult XVM::not()
{
	if (peek(0).isNumber() || peek(0).isBool())
	{
		return push(!pop().asBool());
	}
	else if (peek(0).isColumn())
	{
		Value a = pop();
		Value n(ColumnLength{ a.asColumn().length });
		for (unsigned int i = 0; i < n.asColumn().length; i++)
			n.asColumn().data[i] = (a.asColumn().data[i] == 0);
		n.asColumn().isBoolean = true;
		n.asColumn().categorize = a.asColumn().categorize;
		return push(n);
	}

	runtimeError("Operand must be a number or column.");
	return InterpretResult::RUNTIME_ERROR;
}

InterpretResult XVM::callValue(Value callee, int argCount)
{
	if (callee.isObj()) 
	{
		switch (callee.obj->type) 
		{
		case ObjType::FUNCTION:
			return call(callee.asFunction(), argCount);

		case ObjType::NATIVE:
			return call(callee.asNative(), argCount);

		default:
			// Non-callable object type.                   
			break;
		}
	}

	runtimeError("Can only call functions.");
	return InterpretResult::RUNTIME_ERROR;
}

InterpretResult XVM::call(std::shared_ptr<ObjFunction> function, int argCount)
{
	if (argCount != function->m_arity) 
	{
		runtimeError("Function %s expects %d arguments but got %d.",
			function->m_name.c_str(), function->m_arity, argCount);
		return InterpretResult::RUNTIME_ERROR;
	}

	m_frames.emplace_back(function, function->m_chunk.begin(), m_stack.end() - m_stack.begin() - argCount - 1);
	return InterpretResult::OK;
}

InterpretResult XVM::call(std::shared_ptr<ObjNative> function, int argCount)
{
	if (argCount != function->m_arity) 
	{
		runtimeError("Function %s expects %d arguments but got %d.",
			function->m_name.c_str(), function->m_arity, argCount);
		return InterpretResult::RUNTIME_ERROR;
	}

	if (argCount > 0)
	{
		auto begin = m_stack.end() - argCount;
		if (begin->isColumn())
		{
			unsigned int len = begin->asColumn().length;
			for (auto i = begin + 1; i != m_stack.end(); ++i)
				if (!i->isColumn())
				{
					runtimeError("All parameters should be of the same type.");
					return InterpretResult::RUNTIME_ERROR;
				}
				else if (i->asColumn().length != len)
				{
					runtimeError("All column parameters should be of the same length.");
					return InterpretResult::RUNTIME_ERROR;
				}

			Value ret(ColumnLength{ len });
			for (unsigned int i = 0; i < len; i++)
			{
				std::vector<double> args;
				for (auto j = begin; j != m_stack.end(); ++j)
					args.push_back(j->asColumn().data[i]);

				ret.asColumn().data[i] = (*function)(args);
				if (!function->m_success)
				{
					runtimeError("Runtime error calling native function %s, on element %d of column: %s",
						function->m_name.c_str(), i, function->m_error.c_str());
					return InterpretResult::RUNTIME_ERROR;
				}
			}
			m_stack.erase(m_stack.end() - argCount - 1, m_stack.end());
			return push(ret);
		}
		else if (begin->isNumber())
		{
			for (auto i = begin + 1; i != m_stack.end(); ++i)
				if (!i->isNumber())
				{
					runtimeError("All parameters should be of the same type.");
					return InterpretResult::RUNTIME_ERROR;
				}

			std::vector<double> args(m_stack.end() - argCount, m_stack.end());
			Value result = (*function)(args);
			m_stack.erase(m_stack.end() - argCount - 1, m_stack.end());
			if (function->m_success)
				return push(result);
			else
			{
				runtimeError("Runtime error calling native function %s: %s",
					function->m_name.c_str(), function->m_error.c_str());
				return InterpretResult::RUNTIME_ERROR;
			}
		}
		else
		{
			runtimeError("Parameters to native function should be numbers or columns.");
			return InterpretResult::RUNTIME_ERROR;
		}
	}
	else
	{
		Value result = (*function)(std::vector<double>());
		m_stack.erase(m_stack.end() - argCount - 1, m_stack.end());
		if (function->m_success)
			return push(result);
		else
		{
			runtimeError("Runtime error calling native function %s: %s",
				function->m_name.c_str(), function->m_error.c_str());
			return InterpretResult::RUNTIME_ERROR;
		}
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

void XVM::printObj(CString& out, Value value) const
{
	switch (value.obj->type)
	{
	case ObjType::FUNCTION:
		if (value.asFunction()->m_name.empty())
			out.Append("<script>");
		else
			out.AppendFormat("<fn %s>", value.asFunction()->m_name.c_str());
		break;
	case ObjType::NATIVE:
		out.AppendFormat("<ntv %s>", value.asNative()->m_name.c_str());
		break;
	case ObjType::STRING:
		out.Append(value.asString().c_str());
		break;
	case ObjType::OWNING_COLUMN:
	case ObjType::SHARED_COLUMN:
		if (value.asColumn().isBoolean)
		{
			for (unsigned int i = 0; i < std::min(value.asColumn().length, 2u); i++)
				out.AppendFormat("%s ", value.asColumn().data[i] != 0 ? "true" : "false");
		}
		else
		{
			for (unsigned int i = 0; i < std::min(value.asColumn().length, 2u); i++)
				out.AppendFormat("%g ", value.asColumn().data[i]);
		}
		if (value.asColumn().length > 2)
			out.Append("...");
		if (value.asColumn().categorize != nullptr)
		{
			out.Append("{");
			if (value.asColumn().categorize->isBoolean)
			{
				for (unsigned int i = 0; i < std::min(value.asColumn().length, 2u); i++)
					out.AppendFormat("%s ", value.asColumn().categorize->data[i] != 0 ? "true" : "false");
			}
			else
			{
				for (unsigned int i = 0; i < std::min(value.asColumn().length, 2u); i++)
					out.AppendFormat("%g ", value.asColumn().categorize->data[i]);
			}
			if (value.asColumn().length > 2)
				out.Append("...");
			out.Append("}");
		}
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

	for (int i = m_frames.size()-1; i >= 0; i--) 
	{
		// -1 because the IP is sitting on the next instruction to be
		// executed.                                                 
		std::vector<uint8_t>::const_iterator instruction = m_frames[i].ip - 1;
		DLG()->m_output.AppendFormat("[line %d] in ",
			m_frames[i].function->m_chunk.getLine(instruction));
		if (m_frames[i].function->m_name.empty()) 
		{
			DLG()->m_output.AppendFormat("script\r\n");
		}
		else 
		{
			DLG()->m_output.AppendFormat("%s()\r\n", m_frames[i].function->m_name.c_str());
		}
	}
	m_stack.clear();
}

void XVM::defineNative(const std::string& name, std::shared_ptr<ObjNative> native)
{
	m_globals.emplace(name, native);
}