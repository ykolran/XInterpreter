#pragma once

#include <vector>
#include "xvalue.h"

#define DEBUG_TRACE_EXECUTION
#define DEBUG_PRINT_CODE

class XChunk
{
public:
	template <typename T>
	void write(T data, int line) {
		code.push_back(static_cast<uint8_t>(data));
		lines.push_back(line);
	}

	void patch(int index, uint8_t value)
	{
		code[index] = value;
	}

	using codeIterator = std::vector<uint8_t>::const_iterator;

	inline codeIterator begin() const { return code.cbegin(); }
	inline codeIterator end() const { return code.cend(); }
	inline size_t size() const { return code.size(); }

	int addConstant(const Value& value);
	inline const Value& getConstant(uint8_t index) const { return constants[index]; }

	void disassemble(const char* name) const;
	codeIterator disassembleInstruction(codeIterator it, bool trace) const;
	int getLine(codeIterator it) const { return lines[it - begin()]; }

private:
	codeIterator simpleInstruction(CString& out, const char* name, codeIterator it) const;
	codeIterator jumpInstruction(CString& out, const char* name, int sign, codeIterator it) const;
	codeIterator byteInstruction(CString& out, const char* name, codeIterator it) const;
	codeIterator constantInstruction(CString& out, const char* name, codeIterator it) const;

	std::vector<uint8_t> code;
	std::vector<int> lines;
	std::vector<Value> constants;
};
