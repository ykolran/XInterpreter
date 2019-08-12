#include "stdafx.h"
#include "xchunk.h"
#include "XInterpreterDlg.h"
#include "xobject.h"

int XChunk::addConstant(const Value& value) 
{
	constants.push_back(value);
	return constants.size() - 1;
}

void XChunk::disassemble(const char* name) const
{
	DLG()->m_byteCode.AppendFormat("== %s ==\r\n", name);

	for (codeIterator it = begin(); it != end(); )
	{
		it = disassembleInstruction(it, false);
	}
}

XChunk::codeIterator XChunk::disassembleInstruction(codeIterator it, bool trace) const
{
	CString& out = trace ? DLG()->m_executionTrace : DLG()->m_byteCode;
	auto offset = it - begin();
	out.AppendFormat("%04d ", offset);
	if (offset > 0 && lines[offset] == lines[offset - 1]) 
	{
		out.AppendFormat("   | ");
	}
	else 
	{
		out.AppendFormat("%4d ", lines[offset]);
	}

	OpCode instruction = static_cast<OpCode>(*it);
	switch (instruction)
	{
	case OpCode::CONSTANT:
		return constantInstruction(out, "OP_CONSTANT", it);
	case OpCode::_TRUE:
		return simpleInstruction(out, "OP_TRUE", it);
	case OpCode::_FALSE:
		return simpleInstruction(out, "OP_FALSE", it);
	case OpCode::POP:
		return simpleInstruction(out, "OP_POP", it);
	case OpCode::GET_LOCAL:
		return byteInstruction(out, "OP_GET_LOCAL", it);
	case OpCode::SET_LOCAL:
		return byteInstruction(out, "OP_SET_LOCAL", it);	
	case OpCode::GET_GLOBAL:
		return constantInstruction(out, "OP_GET_GLOBAL", it);
	case OpCode::DEFINE_GLOBAL:
		return constantInstruction(out, "OP_DEFINE_GLOBAL", it);
	case OpCode::SET_GLOBAL:
		return constantInstruction(out, "OP_SET_GLOBAL", it);
	case OpCode::FILE:
		return fileInstruction(out, "OP_FILE", it);
	case OpCode::GET_COLUMN:
		return byteInstruction(out, "OP_GET_COLUMN", it);
	case OpCode::EQUAL:
		return simpleInstruction(out, "OP_EQUAL", it);
	case OpCode::GREATER:
		return simpleInstruction(out, "OP_GREATER", it);
	case OpCode::LESS:
		return simpleInstruction(out, "OP_LESS", it);
	case OpCode::ADD:
		return simpleInstruction(out, "OP_ADD", it);
	case OpCode::SUBTRACT:
		return simpleInstruction(out, "OP_SUBTRACT", it);
	case OpCode::MULTIPLY:
		return simpleInstruction(out, "OP_MULTIPLY", it);
	case OpCode::DIVIDE:
		return simpleInstruction(out, "OP_DIVIDE", it);
	case OpCode::NOT:
		return simpleInstruction(out, "OP_NOT", it);
	case OpCode::NEGATE:
		return simpleInstruction(out, "OP_NEGATE", it);
	case OpCode::PRINT:
		return simpleInstruction(out, "OP_PRINT", it);
	case OpCode::JUMP:
		return jumpInstruction(out, "OP_JUMP", 1, it);
	case OpCode::JUMP_IF_FALSE:
		return jumpInstruction(out, "OP_JUMP_IF_FALSE", 1, it);
	case OpCode::LOOP:
		return jumpInstruction(out, "OP_LOOP", -1, it);
	case OpCode::RETURN:
		return simpleInstruction(out, "OP_RETURN", it);
	default:
		out.AppendFormat("Unknown opcode %d\r\n", instruction);
		return it + 1;
	}
}

XChunk::codeIterator XChunk::simpleInstruction(CString& out, const char* name, codeIterator it) const
{
	out.AppendFormat("%s\r\n", name);
	return it + 1;
}

XChunk::codeIterator XChunk::jumpInstruction(CString& out, const char* name, int sign, codeIterator it) const
{
	uint16_t jump = (uint16_t)(*(it+1) << 8);
	jump |= *(it+2);
	out.AppendFormat("%-16s %4d -> %d\r\n", name, it - begin(), it - begin() + 3 + sign * jump);
	return it + 3;
}

XChunk::codeIterator XChunk::byteInstruction(CString& out, const char* name, codeIterator it) const
{
	uint8_t slot = *(it+1);
	out.AppendFormat("%-16s %4d\r\n", name, slot);
	return it + 2;
}

XChunk::codeIterator XChunk::fileInstruction(CString& out, const char* name, codeIterator it) const
{
	intptr_t filePtr = 0;
	for (int i=0; i<sizeof(intptr_t); i++)
		filePtr |= static_cast<intptr_t>(*(it + 1 + i)) << i*8;
	out.AppendFormat("%-16s 0x%0*x\r\n", name, sizeof(intptr_t)*2 ,filePtr);
	return it + sizeof(intptr_t) + 1;
}

XChunk::codeIterator XChunk::constantInstruction(CString& out, const char* name, codeIterator it) const
{
	uint8_t constant = *(it + 1);
	out.AppendFormat("%-16s %4d '", name, constant);
	if (constants[constant].isNumber())
		out.AppendFormat("%g", constants[constant].asDouble());
	else if (constants[constant].isBool())
		out.AppendFormat(constants[constant].asBool() ? "true" : "false");
	else if (constants[constant].isString())
		out.Append(constants[constant].asString().c_str());
	else if (constants[constant].isColumn())
	{
		const ObjColumn& col(constants[constant].asColumn());
		out.Append("[");
		int i = 0;
		for (; i < col.length && i < 3; i++)
		{
			out.AppendFormat("%g", col.data[i]);
		}
		if (i != col.length)
			out.Append("...");
		out.Append("]");
	}
	out.AppendFormat("'\r\n");
	return it + 2;
}

