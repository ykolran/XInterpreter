#pragma once
#include "xchunk.h"
#include "xobject.h"
#include <vector>
#include <unordered_map>
#include <string>

enum class InterpretResult 
{
	OK,
	COMPILE_ERROR,
	RUNTIME_ERROR
};

struct CallFrame
{
	std::shared_ptr<ObjFunction>	function;
	XChunk::codeIterator			ip;		// instruction pointer
	std::vector<Value>::iterator	slots;	// stack slot
};

// fw declaration
class CLogDataFile;

class XVM
{
public:
	InterpretResult interpret(const char* source);
private:
	InterpretResult run();
	XChunk::codeIterator& ip() { return m_frames.back().ip; }
	XChunk& chunk() { return m_frames.back().function->m_chunk; }
	inline OpCode readOpCode() { return static_cast<OpCode>(*ip()++); }
	inline uint8_t readByte() { return *ip()++; }
	inline uint16_t readShort() { ip() += 2;  return static_cast<uint16_t>((ip()[-2] << 8) | ip()[-1]); }

	inline const Value& readConstant() { return chunk().getConstant(*ip()++); }
	
	InterpretResult negate();

	inline const Value& peek(int distance) { return m_stack[m_stack.size() - 1 - distance]; }  	
	inline void push(const Value& value) { m_stack.push_back(value); }
	inline Value pop() { Value value = m_stack.back(); m_stack.pop_back(); return value; }
	void printValue(CString& out, Value value) const; 
	void printObj(CString& out, Value value) const;

	template<typename OP>
	InterpretResult binaryOp(OP op);

	void runtimeError(const char* format, ...);

	std::vector<Value> m_stack;
	std::vector<CallFrame> m_frames;
	std::unordered_map<std::string, Value> globals;
	CLogDataFile* m_file;
};