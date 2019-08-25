#pragma once
#include "XChunk.h"
#include "XScanner.h"
#include "XObject.h"
#include "XOpCode.h"

#ifdef max
#undef max
#undef min
#endif

// fw declaration
class CLogDataFile;

enum class FunctionType 
{
	FUNCTION,
	SCRIPT
};

class XCompiler
{
public:
	XCompiler(XScanner& scanner, std::string name, FunctionType type);
	std::shared_ptr<ObjFunction> compile();

private:
	enum class Precedence
	{
		NONE,
		ASSIGNMENT,  // =        
		OR,          // or       
		AND,         // and      
		EQUALITY,    // == !=    
		COMPARISON,  // < > <= >=
		ADDITION,    // + -      
		MULTIPLICATION, // * /      
		UNARY,       // ! -      
		CALL,        // . () []  
		PRIMARY
	};

	typedef void (XCompiler::*PrefixMethodPtr)(bool);
	typedef void (XCompiler::*InfixMethodPtr)();

	struct ParseRule
	{
		PrefixMethodPtr prefix;
		InfixMethodPtr infix;
		Precedence precedence;
	};

	void usingDeclaration();
	void declaration();
	void funDeclaration();
	void function(FunctionType type);
	void varDeclaration();
	void statement();
	void printStatement();
	void expressionStatement();
	void forStatement();
	void ifStatement();
	void returnStatement();
	void whileStatement();
	void expression();
	void block();
	void number(bool canAssign);
	void grouping(bool canAssign);
	void unary(bool canAssign);
	void binary();
	void call();
	void and();
	void or();

	void literal(bool canAssign);
	void string(bool canAssign);
	void variable(bool canAssign);
	void namedVariable(const XTokenData& name, bool canAssign);

	void parsePrecedence(Precedence precedence);
	uint8_t identifierConstant(const XTokenData& name) { return makeConstant(Value(name.start, name.length)); } 	
	uint8_t parseVariable(const char* errorMessage);
	void declareVariable();
	void markInitialized();
	void defineVariable(uint8_t global);
	uint8_t argumentList();
	const ParseRule& getRule(XToken type) const { return rules[static_cast<size_t>(type)]; }

	void emitByte(uint8_t byte) { chunk().write(byte, m_scanner.previous().line); }
	void emitInstruction(XOpCode op) { chunk().write(op, m_scanner.previous().line); }
	void emitInstruction(XOpCode op, uint8_t byte) { emitInstruction(op); emitByte(byte); }
	void emitInstruction(XOpCode op, uint8_t byte1, uint8_t byte2) { emitInstruction(op, byte1); emitByte(byte2); }
	int emitJump(XOpCode op) { emitInstruction(op, 0xff, 0xff);	return chunk().size() - 2;	}
	void emitLoop(int loopStart);
	void patchJump(int offset);
	void emitConstant(const Value& value) { emitInstruction(XOpCode::CONSTANT, makeConstant(value)); }
	void endCompiler();
	void beginScope() { m_scopeDepth++;	}
	void endScope();
	void emitReturn() { emitInstruction(XOpCode::NIL); emitInstruction(XOpCode::RETURN); };
	uint8_t makeConstant(const Value& value);

	void error(const char* message) { m_scanner.errorAt(m_scanner.previous(), message); }
	bool identifiersEqual(const XTokenData& a, const XTokenData& b) const;
	int resolveLocal(const XTokenData& name);

	XScanner& m_scanner;
	std::shared_ptr<ObjFunction> m_compilingFunction;
	FunctionType m_type;

	XChunk& chunk() { return m_compilingFunction->m_chunk;  }

	struct Local
	{
		Local(const XTokenData& t, int d) : token(t), depth(d) {}

		XTokenData token;
		int depth;
	};

	std::vector<Local> m_locals;
	CLogDataFile* m_file;
	int m_scopeDepth;

	// rules per token
	static ParseRule rules[static_cast<size_t>(XToken::_EOF) + 1];
};