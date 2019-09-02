#include "stdafx.h"
#include "XCompiler.h"
#include "XScanner.h"
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
{ nullptr,				nullptr,			Precedence::NONE },       // LEFT_BRACKET
{ nullptr,				nullptr,			Precedence::NONE },       // RIGHT_BRACKET    
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

XCompiler::XCompiler(XScanner& scanner, std::string name, FunctionType type, bool printByteCode) :
	m_scanner(scanner),
	m_compilingFunction(std::make_shared<ObjFunction>(name, 0)),
	m_type(type),
	m_printByteCode(printByteCode)
{
	m_scopeDepth = 0;
//	XTokenData t{ XToken::NIL, "", 0, 0 };
//	m_locals.emplace_back(t, 0);
}

std::shared_ptr<ObjFunction> XCompiler::compile()
{
	m_scanner.advanceToken();
	while (!m_scanner.match(XToken::_EOF))
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
	if (m_scanner.match(XToken::USING))
	{
		m_scanner.consume(XToken::IDENTIFIER, "Expected file-name after 'using'");
		std::string filename(m_scanner.previous().start, m_scanner.previous().length);
		auto it = std::find_if(DLG()->m_files.begin(), DLG()->m_files.end(), [&filename](const auto& a) {return a.first == filename; });
		if (it == DLG()->m_files.end())
		{
			error((filename + " file not found").c_str());
			return;
		}
		m_scanner.consume(XToken::LEFT_BRACE, "Expected '{' after using file-name.");
		{
			beginScope();
			for (int i = 0; i < it->second->numColumns; i++)
			{
				XTokenData token{ XToken::IDENTIFIER, it->second->columns[i].c_str(), it->second->columns[i].size(), m_scanner.previous().line };
				m_locals.emplace_back(token, m_scopeDepth);
				emitConstant(Value(ColumnLength{ it->second->m_sizeFilled }, it->second->m_data[i]));
			}
			block();
			endScope();
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
	if (m_scanner.match(XToken::FUN))
	{
		funDeclaration();
	}
	else if (m_scanner.match(XToken::VAR)) 
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
	XCompiler compiler(m_scanner, std::string(m_scanner.previous().start, m_scanner.previous().length), type, m_printByteCode);
	compiler.beginScope();

	// Compile the parameter list.                                
	m_scanner.consume(XToken::LEFT_PAREN, "Expect '(' after function name.");
	if (!m_scanner.check(XToken::RIGHT_PAREN)) 
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
		} while (m_scanner.match(XToken::COMMA));
	}
	m_scanner.consume(XToken::RIGHT_PAREN, "Expect ')' after parameters.");

	// The body.                                                  
	m_scanner.consume(XToken::LEFT_BRACE, "Expect '{' before function body.");
	compiler.block();

	// Create the function object.                                
	compiler.endCompiler();
	if (!m_scanner.hadError())
		return emitInstruction(XOpCode::CONSTANT, makeConstant(compiler.m_compilingFunction));
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
	m_scanner.consume(XToken::EQUAL, "Expect assignment.");
	expression();
	m_scanner.consume(XToken::SEMICOLON, "Expect ';' after variable declaration.");

	defineVariable(global);
}

void XCompiler::statement()
{
	if (m_scanner.match(XToken::PRINT))
	{
		printStatement();
	}
	else if (m_scanner.match(XToken::FOR))
	{
		forStatement();
	}
	else if (m_scanner.match(XToken::IF))
	{
		ifStatement();
	}
	else if (m_scanner.match(XToken::RETURN))
	{
		returnStatement();
	}
	else if (m_scanner.match(XToken::WHILE)) 
	{
		whileStatement();
	}
	else if (m_scanner.match(XToken::LEFT_BRACE))
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
	m_scanner.consume(XToken::SEMICOLON, "Expect ';' after value.");
	emitInstruction(XOpCode::PRINT);
}

void XCompiler::expressionStatement()
{
	expression();
	m_scanner.consume(XToken::SEMICOLON, "Expect ';' after expression.");
	emitInstruction(XOpCode::POP);
}

void XCompiler::forStatement() 
{
	beginScope();
	m_scanner.consume(XToken::LEFT_PAREN, "Expect '(' after 'for'.");
	if (m_scanner.match(XToken::VAR)) 
	{
		varDeclaration();
	}
	else if (m_scanner.match(XToken::SEMICOLON)) 
	{
		// No initializer.                                 
	}
	else 
	{
		expressionStatement();
	}

	int loopStart = chunk().size();

	int exitJump = -1;
	if (!m_scanner.match(XToken::SEMICOLON)) 
	{
		expression();
		m_scanner.consume(XToken::SEMICOLON, "Expect ';' after loop condition.");

		// Jump out of the loop if the condition is false.           
		exitJump = emitJump(XOpCode::JUMP_IF_FALSE);
		emitInstruction(XOpCode::POP); // Condition.                              
	}

	if (!m_scanner.match(XToken::RIGHT_PAREN)) 
	{
		int bodyJump = emitJump(XOpCode::JUMP);

		int incrementStart = chunk().size();
		expression();
		emitInstruction(XOpCode::POP);
		m_scanner.consume(XToken::RIGHT_PAREN, "Expect ')' after for clauses.");

		emitLoop(loopStart);
		loopStart = incrementStart;
		patchJump(bodyJump);
	}

	statement();

	emitLoop(loopStart);

	if (exitJump != -1) 
	{
		patchJump(exitJump);
		emitInstruction(XOpCode::POP); // Condition.
	}

	endScope();
}

void XCompiler::ifStatement() 
{
	m_scanner.consume(XToken::LEFT_PAREN, "Expect '(' after 'if'.");
	expression();
	m_scanner.consume(XToken::RIGHT_PAREN, "Expect ')' after condition.");

	int thenJump = emitJump(XOpCode::JUMP_IF_FALSE);
	emitInstruction(XOpCode::POP);
	statement();

	int elseJump = emitJump(XOpCode::JUMP);
	patchJump(thenJump);
	emitInstruction(XOpCode::POP);

	if (m_scanner.match(XToken::ELSE)) 
		statement();
	patchJump(elseJump);
}

void XCompiler::returnStatement()
{
	if (m_type == FunctionType::SCRIPT) 
	{
		error("Cannot return from top-level code.");
	}

	if (m_scanner.match(XToken::SEMICOLON))
	{
		emitReturn();
	}
	else 
	{
		expression();
		m_scanner.consume(XToken::SEMICOLON, "Expect ';' after return value.");
		emitInstruction(XOpCode::RETURN);
	}
}

void XCompiler::whileStatement()
{
	int loopStart = chunk().size();
	m_scanner.consume(XToken::LEFT_PAREN, "Expect '(' after 'while'.");
	expression();
	m_scanner.consume(XToken::RIGHT_PAREN, "Expect ')' after condition.");

	int exitJump = emitJump(XOpCode::JUMP_IF_FALSE);

	emitInstruction(XOpCode::POP);
	statement();

	emitLoop(loopStart);

	patchJump(exitJump);
	emitInstruction(XOpCode::POP);
}

void XCompiler::expression()
{
	parsePrecedence(Precedence::ASSIGNMENT);
}

void XCompiler::block() 
{
	while (!m_scanner.check(XToken::RIGHT_BRACE) && 
		   !m_scanner.check(XToken::_EOF)) 
	{
		usingDeclaration();
	}

	m_scanner.consume(XToken::RIGHT_BRACE, "Expect '}' after block.");
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

void XCompiler::namedVariable(const XTokenData& name, bool canAssign) 
{
	XOpCode getOp = XOpCode::GET_GLOBAL;
	XOpCode setOp = XOpCode::SET_GLOBAL;

	int arg = resolveLocal(name);
	if (arg != -1) 
	{
		getOp = XOpCode::GET_LOCAL;
		setOp = XOpCode::SET_LOCAL;
	}

	if (arg == -1)
	{
		arg = identifierConstant(name);
	}

	if (canAssign && m_scanner.match(XToken::EQUAL))
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
	m_scanner.consume(XToken::RIGHT_PAREN, "Expect ')' after expression.");
}

void XCompiler::unary(bool /*canAssign*/) {
	XToken operatorType = m_scanner.previous().type;

	// Compile the operand.                        
	parsePrecedence(Precedence::UNARY);

	// Emit the operator instruction.              
	switch (operatorType) {
	case XToken::MINUS: 
		emitInstruction(XOpCode::NEGATE); break;
	case XToken::BANG:
		emitInstruction(XOpCode::NOT); break;
	default:
		return; // Unreachable.                    
	}
}

void XCompiler::binary() 
{
	// Remember the operator.                                
	XToken operatorType = m_scanner.previous().type;

	// Compile the right operand.                            
	const ParseRule& rule = getRule(operatorType);
	parsePrecedence(static_cast<Precedence>(static_cast<int>(rule.precedence) + 1));

	// Emit the operator instruction.                        
	switch (operatorType) 
	{
	case XToken::BANG_EQUAL:    emitInstruction(XOpCode::EQUAL); emitInstruction(XOpCode::NOT); break;
	case XToken::EQUAL_EQUAL:   emitInstruction(XOpCode::EQUAL); break;
	case XToken::GREATER:       emitInstruction(XOpCode::GREATER); break;
	case XToken::GREATER_EQUAL: emitInstruction(XOpCode::LESS); emitInstruction(XOpCode::NOT); break;
	case XToken::LESS:          emitInstruction(XOpCode::LESS); break;
	case XToken::LESS_EQUAL:    emitInstruction(XOpCode::GREATER); emitInstruction(XOpCode::NOT); break;
	case XToken::PLUS:          emitInstruction(XOpCode::ADD); break;
	case XToken::MINUS:         emitInstruction(XOpCode::SUBTRACT); break;
	case XToken::STAR:          emitInstruction(XOpCode::MULTIPLY); break;
	case XToken::SLASH:         emitInstruction(XOpCode::DIVIDE); break;
	default:
		return; // Unreachable.                              
	}
}

void XCompiler::call()
{
	uint8_t argCount = argumentList();
	emitInstruction(XOpCode::CALL, argCount);
}

void XCompiler::and() 
{
	int endJump = emitJump(XOpCode::JUMP_IF_FALSE);

	emitInstruction(XOpCode::POP);
	parsePrecedence(Precedence::AND);

	patchJump(endJump);
}

void XCompiler::or()
{
	int elseJump = emitJump(XOpCode::JUMP_IF_FALSE);
	int endJump = emitJump(XOpCode::JUMP);

	patchJump(elseJump);
	emitInstruction(XOpCode::POP);

	parsePrecedence(Precedence::OR);
	patchJump(endJump);
}

void XCompiler::literal(bool /*canAssign*/)
{
	switch (m_scanner.previous().type) {
	case XToken::_FALSE: emitInstruction(XOpCode::_FALSE); break;
	case XToken::NIL: emitInstruction(XOpCode::NIL); break;
	case XToken::_TRUE: emitInstruction(XOpCode::_TRUE); break;
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

	if (canAssign && m_scanner.match(XToken::EQUAL))
	{
		error("Invalid assignment target.");
		expression();
	}
}

uint8_t XCompiler::parseVariable(const char* errorMessage) 
{
	m_scanner.consume(XToken::IDENTIFIER, errorMessage);

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
	if (!m_locals.empty())
		m_locals.back().depth = m_scopeDepth;
}

void XCompiler::defineVariable(uint8_t global) 
{ 
	if (m_scopeDepth > 0) 
	{
		markInitialized();
		return;
	}    

	emitInstruction(XOpCode::DEFINE_GLOBAL, global);
}

uint8_t XCompiler::argumentList() 
{
	uint8_t argCount = 0;
	if (!m_scanner.check(XToken::RIGHT_PAREN)) 
	{
		do 
		{
			expression();
			if (argCount == 255) 
			{
				error("Cannot have more than 255 arguments.");
			} 
			argCount++;
		} while (m_scanner.match(XToken::COMMA));
	}

	m_scanner.consume(XToken::RIGHT_PAREN, "Expect ')' after arguments.");
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
	emitInstruction(XOpCode::LOOP);

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
	if (!m_scanner.hadError() && m_printByteCode) 
	{
			chunk().disassemble(m_compilingFunction->m_name.empty() ? 
				"<script>" : m_compilingFunction->m_name.c_str());
	}
}

void XCompiler::endScope() 
{
	m_scopeDepth--;   
	while (m_locals.size() > 0 && m_locals.back().depth > m_scopeDepth)
	{
		emitInstruction(XOpCode::POP);
		m_locals.pop_back();
	}
}


bool XCompiler::identifiersEqual(const XTokenData& a, const XTokenData& b) const
{
	if (a.length != b.length) 
		return false;
	return memcmp(a.start, b.start, a.length) == 0;
}

int XCompiler::resolveLocal(const XTokenData& name)
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