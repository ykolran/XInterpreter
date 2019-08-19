#include "stdafx.h"
#include "xcompiler.h"
#include "xscanner.h"
#include "XInterpreterDlg.h"
#include <algorithm>

#ifdef max
#undef max
#undef min
#endif

XCompiler::ParseRule XCompiler::rules[] =
{
{ &XCompiler::grouping,	&XCompiler::call,	Precedence::CALL },       // LEFT_PAREN      
{ nullptr,				nullptr,			Precedence::NONE },       // RIGHT_PAREN     
{ nullptr,				nullptr,			Precedence::NONE },       // LEFT_BRACE
{ nullptr,				nullptr,			Precedence::NONE },       // RIGHT_BRACE     
{ nullptr,				nullptr,			Precedence::NONE },       // COMMA           
{ nullptr,				nullptr,			Precedence::CALL },       // DOT             
{ &XCompiler::unary,    &XCompiler::binary,	Precedence::ADDITION },   // MINUS           
{ nullptr,				&XCompiler::binary,	Precedence::ADDITION },   // PLUS            
{ nullptr,				nullptr,			Precedence::NONE },       // SEMICOLON       
{ nullptr,				&XCompiler::binary,	Precedence::MULTIPLICATION }, // SLASH           
{ nullptr,				&XCompiler::binary,	Precedence::MULTIPLICATION }, // STAR            
{ &XCompiler::unary,	nullptr,			Precedence::NONE },       // BANG            
{ nullptr,				&XCompiler::binary,	Precedence::EQUALITY },   // BANG_EQUAL      
{ nullptr,				nullptr,			Precedence::NONE },       // EQUAL           
{ nullptr,				&XCompiler::binary,	Precedence::EQUALITY },   // EQUAL_EQUAL     
{ nullptr,				&XCompiler::binary,	Precedence::COMPARISON }, // GREATER         
{ nullptr,				&XCompiler::binary,	Precedence::COMPARISON }, // GREATER_EQUAL   
{ nullptr,				&XCompiler::binary,	Precedence::COMPARISON }, // LESS            
{ nullptr,				&XCompiler::binary,	Precedence::COMPARISON }, // LESS_EQUAL      
{ &XCompiler::variable,	nullptr,			Precedence::NONE },       // IDENTIFIER      
{ &XCompiler::string,	nullptr,			Precedence::NONE },       // STRING          
{ &XCompiler::number,   nullptr,			Precedence::NONE },       // NUMBER          
{ nullptr,				&XCompiler::and,	Precedence::AND },        // AND             
{ nullptr,				nullptr,			Precedence::NONE },       // CLASS           
{ nullptr,				nullptr,			Precedence::NONE },       // ELSE            
{ &XCompiler::literal,	nullptr,			Precedence::NONE },       // FALSE           
{ nullptr,				nullptr,			Precedence::NONE },       // FOR             
{ nullptr,				nullptr,			Precedence::NONE },       // FUN             
{ nullptr,				nullptr,			Precedence::NONE },       // IF              
{ &XCompiler::literal,	nullptr,			Precedence::NONE },       // NIL             
{ nullptr,				&XCompiler::or,		Precedence::OR },         // OR              
{ nullptr,				nullptr,			Precedence::NONE },       // PRINT           
{ nullptr,				nullptr,			Precedence::NONE },       // RETURN          
{ nullptr,				nullptr,			Precedence::NONE },       // SUPER           
{ nullptr,				nullptr,			Precedence::NONE },       // _THIS            
{ &XCompiler::literal,	nullptr,			Precedence::NONE },       // _TRUE            
{ nullptr,				nullptr,			Precedence::NONE },       // VAR             
{ nullptr,				nullptr,			Precedence::NONE },       // WHILE           
{ nullptr,				nullptr,			Precedence::NONE },       // USING           
{ nullptr,				nullptr,			Precedence::NONE },       // _ERROR           
{ nullptr,				nullptr,			Precedence::NONE },       // _EOF             
};

XCompiler::XCompiler(XScanner& scanner, std::string name, FunctionType type) :
	m_scanner(scanner),
	m_compilingFunction(std::make_shared<ObjFunction>(name, 0)),
	m_type(type),
	m_file(nullptr)
{
	m_scopeDepth = 0;
	XScanner::Token t{ TokenT::NIL, "", 0, 0 };
	m_locals.emplace_back(t, 0);
}

std::shared_ptr<ObjFunction> XCompiler::compile()
{
	m_scanner.advanceToken();
	while (!m_scanner.match(TokenT::_EOF))
	{
		usingDeclaration();
	}

	endCompiler();

	if (!m_scanner.hadError())
		return m_compilingFunction;
	else
		return nullptr;
}

void XCompiler::usingDeclaration()
{
	if (m_scanner.match(TokenT::USING))
	{
		m_scanner.consume(TokenT::IDENTIFIER, "Expected file-name after 'using'");
		std::string filename(m_scanner.previous().start, m_scanner.previous().length);
		auto it = std::find_if(DLG()->m_files.begin(), DLG()->m_files.end(), [&filename](const auto& a) {return a.first == filename; });
		if (it == DLG()->m_files.end())
		{
			error((filename + " file not found").c_str());
			return;
		}
		m_scanner.consume(TokenT::LEFT_BRACE, "Expected '{' after using file-name.");
		{
			beginScope();
			m_file = it->second;
			emitInstruction(OpCode::FILE);
			for (int i = 0; i < sizeof(intptr_t); i++)
				emitByte((reinterpret_cast<intptr_t>(m_file) >> i * 8) & 0xFF);
			block();
			endScope();
			m_file = nullptr;
		}
	}
	else
	{
		declaration();
	}

	if (m_scanner.panicMode())
		m_scanner.synchronize();
}

void XCompiler::declaration()
{
	if (m_scanner.match(TokenT::FUN))
	{
		funDeclaration();
	}
	else if (m_scanner.match(TokenT::VAR)) 
	{
		varDeclaration();
	}
	else 
	{
		statement();
	}
}

void XCompiler::function(FunctionType type) 
{
	XCompiler compiler(m_scanner, std::string(m_scanner.previous().start, m_scanner.previous().length), type);
	compiler.beginScope();

	// Compile the parameter list.                                
	m_scanner.consume(TokenT::LEFT_PAREN, "Expect '(' after function name.");
	if (!m_scanner.check(TokenT::RIGHT_PAREN)) 
	{
		do 
		{
			compiler.m_compilingFunction->m_arity++;
			if (compiler.m_compilingFunction->m_arity > 255) 
			{
				m_scanner.errorAtCurrent("Cannot have more than 255 parameters.");
			}

			uint8_t paramConstant = compiler.parseVariable("Expect parameter name.");
			compiler.defineVariable(paramConstant);
		} while (m_scanner.match(TokenT::COMMA));
	}
	m_scanner.consume(TokenT::RIGHT_PAREN, "Expect ')' after parameters.");

	// The body.                                                  
	m_scanner.consume(TokenT::LEFT_BRACE, "Expect '{' before function body.");
	compiler.block();

	// Create the function object.                                
	compiler.endCompiler();
	if (!m_scanner.hadError())
		return emitInstruction(OpCode::CONSTANT, makeConstant(compiler.m_compilingFunction));
}

void XCompiler::funDeclaration()
{
	uint8_t global = parseVariable("Expect function name.");
	markInitialized();
	function(FunctionType::FUNCTION);
	defineVariable(global);
}

void XCompiler::varDeclaration() 
{
	uint8_t global = parseVariable("Expect variable name.");
	m_scanner.consume(TokenT::EQUAL, "Expect assignment.");
	expression();
	m_scanner.consume(TokenT::SEMICOLON, "Expect ';' after variable declaration.");

	defineVariable(global);
}

void XCompiler::statement()
{
	if (m_scanner.match(TokenT::PRINT))
	{
		printStatement();
	}
	else if (m_scanner.match(TokenT::FOR))
	{
		forStatement();
	}
	else if (m_scanner.match(TokenT::IF))
	{
		ifStatement();
	}
	else if (m_scanner.match(TokenT::RETURN))
	{
		returnStatement();
	}
	else if (m_scanner.match(TokenT::WHILE)) 
	{
		whileStatement();
	}
	else if (m_scanner.match(TokenT::LEFT_BRACE))
	{
		beginScope();
		block();
		endScope();
	}
	else
	{
		expressionStatement();
	}
}

void XCompiler::printStatement() 
{
	expression();
	m_scanner.consume(TokenT::SEMICOLON, "Expect ';' after value.");
	emitInstruction(OpCode::PRINT);
}

void XCompiler::expressionStatement()
{
	expression();
	m_scanner.consume(TokenT::SEMICOLON, "Expect ';' after expression.");
	emitInstruction(OpCode::POP);
}

void XCompiler::forStatement() 
{
	beginScope();
	m_scanner.consume(TokenT::LEFT_PAREN, "Expect '(' after 'for'.");
	if (m_scanner.match(TokenT::VAR)) 
	{
		varDeclaration();
	}
	else if (m_scanner.match(TokenT::SEMICOLON)) 
	{
		// No initializer.                                 
	}
	else 
	{
		expressionStatement();
	}

	int loopStart = chunk().size();

	int exitJump = -1;
	if (!m_scanner.match(TokenT::SEMICOLON)) 
	{
		expression();
		m_scanner.consume(TokenT::SEMICOLON, "Expect ';' after loop condition.");

		// Jump out of the loop if the condition is false.           
		exitJump = emitJump(OpCode::JUMP_IF_FALSE);
		emitInstruction(OpCode::POP); // Condition.                              
	}

	if (!m_scanner.match(TokenT::RIGHT_PAREN)) 
	{
		int bodyJump = emitJump(OpCode::JUMP);

		int incrementStart = chunk().size();
		expression();
		emitInstruction(OpCode::POP);
		m_scanner.consume(TokenT::RIGHT_PAREN, "Expect ')' after for clauses.");

		emitLoop(loopStart);
		loopStart = incrementStart;
		patchJump(bodyJump);
	}

	statement();

	emitLoop(loopStart);

	if (exitJump != -1) 
	{
		patchJump(exitJump);
		emitInstruction(OpCode::POP); // Condition.
	}

	endScope();
}

void XCompiler::ifStatement() 
{
	m_scanner.consume(TokenT::LEFT_PAREN, "Expect '(' after 'if'.");
	expression();
	m_scanner.consume(TokenT::RIGHT_PAREN, "Expect ')' after condition.");

	int thenJump = emitJump(OpCode::JUMP_IF_FALSE);
	emitInstruction(OpCode::POP);
	statement();

	int elseJump = emitJump(OpCode::JUMP);
	patchJump(thenJump);
	emitInstruction(OpCode::POP);

	if (m_scanner.match(TokenT::ELSE)) 
		statement();
	patchJump(elseJump);
}

void XCompiler::returnStatement()
{
	if (m_type == FunctionType::SCRIPT) 
	{
		error("Cannot return from top-level code.");
	}

	if (m_scanner.match(TokenT::SEMICOLON))
	{
		emitReturn();
	}
	else 
	{
		expression();
		m_scanner.consume(TokenT::SEMICOLON, "Expect ';' after return value.");
		emitInstruction(OpCode::RETURN);
	}
}

void XCompiler::whileStatement()
{
	int loopStart = chunk().size();
	m_scanner.consume(TokenT::LEFT_PAREN, "Expect '(' after 'while'.");
	expression();
	m_scanner.consume(TokenT::RIGHT_PAREN, "Expect ')' after condition.");

	int exitJump = emitJump(OpCode::JUMP_IF_FALSE);

	emitInstruction(OpCode::POP);
	statement();

	emitLoop(loopStart);

	patchJump(exitJump);
	emitInstruction(OpCode::POP);
}

void XCompiler::expression()
{
	parsePrecedence(Precedence::ASSIGNMENT);
}

void XCompiler::block() 
{
	while (!m_scanner.check(TokenT::RIGHT_BRACE) && 
		   !m_scanner.check(TokenT::_EOF)) 
	{
		usingDeclaration();
	}

	m_scanner.consume(TokenT::RIGHT_BRACE, "Expect '}' after block.");
}

void XCompiler::number(bool /*canAssign*/)
{
	double value = strtod(m_scanner.previous().start, nullptr);
	emitConstant(value);
}

void XCompiler::string(bool /*canAssign*/)
{
	emitConstant(Value(m_scanner.previous().start + 1, m_scanner.previous().length - 2));
}

void XCompiler::variable(bool canAssign) 
{
	namedVariable(m_scanner.previous(), canAssign);
}

void XCompiler::namedVariable(const XScanner::Token& name, bool canAssign) 
{
	OpCode getOp = OpCode::GET_GLOBAL;
	OpCode setOp = OpCode::SET_GLOBAL;

	int arg = resolveLocal(name);
	if (arg != -1) 
	{
		getOp = OpCode::GET_LOCAL;
		setOp = OpCode::SET_LOCAL;
	}
	else if (m_file != nullptr)
	{
		auto it = find(m_file->columns.begin(), m_file->columns.end(), std::string(name.start, name.length));
		if (it != m_file->columns.end())
		{
			arg = it - m_file->columns.begin();
			getOp = OpCode::GET_COLUMN;
			canAssign = false;
		}
	}

	if (arg == -1)
	{
		arg = identifierConstant(name);
	}

	if (canAssign && m_scanner.match(TokenT::EQUAL))
	{
		expression();
		emitInstruction(setOp, static_cast<uint8_t>(arg));
	}
	else 
	{
		emitInstruction(getOp, static_cast<uint8_t>(arg));
	}
}

void XCompiler::grouping(bool /*canAssign*/) {
	expression();
	m_scanner.consume(TokenT::RIGHT_PAREN, "Expect ')' after expression.");
}

void XCompiler::unary(bool /*canAssign*/) {
	TokenT operatorType = m_scanner.previous().type;

	// Compile the operand.                        
	parsePrecedence(Precedence::UNARY);

	// Emit the operator instruction.              
	switch (operatorType) {
	case TokenT::MINUS: 
		emitInstruction(OpCode::NEGATE); break;
	case TokenT::BANG:
		emitInstruction(OpCode::NOT); break;
	default:
		return; // Unreachable.                    
	}
}

void XCompiler::binary() 
{
	// Remember the operator.                                
	TokenT operatorType = m_scanner.previous().type;

	// Compile the right operand.                            
	const ParseRule& rule = getRule(operatorType);
	parsePrecedence(static_cast<Precedence>(static_cast<int>(rule.precedence) + 1));

	// Emit the operator instruction.                        
	switch (operatorType) 
	{
	case TokenT::BANG_EQUAL:    emitInstruction(OpCode::EQUAL); emitInstruction(OpCode::NOT); break;
	case TokenT::EQUAL_EQUAL:   emitInstruction(OpCode::EQUAL); break;
	case TokenT::GREATER:       emitInstruction(OpCode::GREATER); break;
	case TokenT::GREATER_EQUAL: emitInstruction(OpCode::LESS); emitInstruction(OpCode::NOT); break;
	case TokenT::LESS:          emitInstruction(OpCode::LESS); break;
	case TokenT::LESS_EQUAL:    emitInstruction(OpCode::GREATER); emitInstruction(OpCode::NOT); break;
	case TokenT::PLUS:          emitInstruction(OpCode::ADD); break;
	case TokenT::MINUS:         emitInstruction(OpCode::SUBTRACT); break;
	case TokenT::STAR:          emitInstruction(OpCode::MULTIPLY); break;
	case TokenT::SLASH:         emitInstruction(OpCode::DIVIDE); break;
	default:
		return; // Unreachable.                              
	}
}

void XCompiler::call()
{
	uint8_t argCount = argumentList();
	emitInstruction(OpCode::CALL, argCount);
}

void XCompiler::and() 
{
	int endJump = emitJump(OpCode::JUMP_IF_FALSE);

	emitInstruction(OpCode::POP);
	parsePrecedence(Precedence::AND);

	patchJump(endJump);
}

void XCompiler::or()
{
	int elseJump = emitJump(OpCode::JUMP_IF_FALSE);
	int endJump = emitJump(OpCode::JUMP);

	patchJump(elseJump);
	emitInstruction(OpCode::POP);

	parsePrecedence(Precedence::OR);
	patchJump(endJump);
}

void XCompiler::literal(bool /*canAssign*/)
{
	switch (m_scanner.previous().type) {
	case TokenT::_FALSE: emitInstruction(OpCode::_FALSE); break;
	case TokenT::NIL: emitInstruction(OpCode::NIL); break;
	case TokenT::_TRUE: emitInstruction(OpCode::_TRUE); break;
	default:
		return; // Unreachable.                   
	}
}

void XCompiler::parsePrecedence(Precedence precedence) 
{
	m_scanner.advanceToken();
	const ParseRule& rule = getRule(m_scanner.previous().type);
	if (rule.prefix == nullptr) 
	{
		error("Expect expression.");
		return;
	}

	bool canAssign = precedence <= Precedence::ASSIGNMENT;
	(this->*rule.prefix)(canAssign);


	while (precedence <= getRule(m_scanner.current().type).precedence) 
	{
		m_scanner.advanceToken();
		const ParseRule& infixRule = getRule(m_scanner.previous().type);
		if (infixRule.infix != nullptr)
			(this->*infixRule.infix)();
	}

	if (canAssign && m_scanner.match(TokenT::EQUAL))
	{
		error("Invalid assignment target.");
		expression();
	}
}

uint8_t XCompiler::parseVariable(const char* errorMessage) 
{
	m_scanner.consume(TokenT::IDENTIFIER, errorMessage);

	declareVariable();
	if (m_scopeDepth > 0)
		return 0;

	return identifierConstant(m_scanner.previous());
}

void XCompiler::declareVariable() 
{
	// Global variables are implicitly declared.
	if (m_scopeDepth == 0) 
		return;

	for (int i = m_locals.size() - 1; i >= 0; i--) 
	{
		const Local& local = m_locals[i];
		if (local.depth != -1 && local.depth < m_scopeDepth) 
			break;
		if (identifiersEqual(m_scanner.previous(), local.token)) {
			error("Variable with this name already declared in this scope.");
		}
	}

	if (m_locals.size() == std::numeric_limits<uint8_t>::max()) 
	{
		error("Too many local variables in function.");
		return;
	}

	m_locals.emplace_back(m_scanner.previous(), -1);
}

void XCompiler::markInitialized()
{
	m_locals.back().depth = m_scopeDepth;
}

void XCompiler::defineVariable(uint8_t global) 
{ 
	if (m_scopeDepth > 0) 
	{
		markInitialized();
		return;
	}    

	emitInstruction(OpCode::DEFINE_GLOBAL, global);
}

uint8_t XCompiler::argumentList() 
{
	uint8_t argCount = 0;
	if (!m_scanner.check(TokenT::RIGHT_PAREN)) 
	{
		do 
		{
			expression();
			if (argCount == 255) 
			{
				error("Cannot have more than 255 arguments.");
			} 
			argCount++;
		} while (m_scanner.match(TokenT::COMMA));
	}

	m_scanner.consume(TokenT::RIGHT_PAREN, "Expect ')' after arguments.");
	return argCount;
}

uint8_t XCompiler::makeConstant(const Value& value) 
{
	int constant = chunk().addConstant(value);
	if (constant > std::numeric_limits<uint8_t>::max()) 
	{
		error("Too many constants in one chunk.");
		return 0;
	}

	return (uint8_t)constant;
}

void XCompiler::emitLoop(int loopStart) 
{
	emitInstruction(OpCode::LOOP);

	int offset = chunk().size() - loopStart + 2;
	if (offset > std::numeric_limits<uint16_t>::max()) 
		error("Loop body too large.");

	emitByte((offset >> 8) & 0xff);
	emitByte(offset & 0xff);
}

void XCompiler::patchJump(int offset) 
{
	// -2 to adjust for the bytecode for the jump offset itself.
	int jump = chunk().size() - offset - 2;

	if (jump > std::numeric_limits<uint16_t>::max()) 
	{
		error("Too much code to jump over.");
	}

	chunk().patch(offset, (jump >> 8) & 0xff);
	chunk().patch(offset + 1, jump & 0xff);
}

void XCompiler::endCompiler()
{
	emitReturn();
#ifdef DEBUG_PRINT_CODE
	if (!m_scanner.hadError()) 
	{
			chunk().disassemble(m_compilingFunction->m_name.empty() ? 
				"<script>" : m_compilingFunction->m_name.c_str());
	}
#endif 
}

void XCompiler::endScope() 
{
	m_scopeDepth--;   
	while (m_locals.size() > 0 && m_locals.back().depth > m_scopeDepth)
	{
		emitInstruction(OpCode::POP);
		m_locals.pop_back();
	}
}


bool XCompiler::identifiersEqual(const XScanner::Token& a, const XScanner::Token& b) const
{
	if (a.length != b.length) 
		return false;
	return memcmp(a.start, b.start, a.length) == 0;
}

int XCompiler::resolveLocal(const XScanner::Token& name)
{
	for (int i = m_locals.size() - 1; i >= 0; i--) 
	{
		if (identifiersEqual(name, m_locals[i].token)) 
		{
			if (m_locals[i].depth == -1)
			{
				error("Cannot read local variable in its own initializer.");
			}
			return i;
		}
	}

	return -1;
}