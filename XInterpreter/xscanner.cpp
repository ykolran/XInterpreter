#include "stdafx.h"
#include "xscanner.h"
#include "XInterpreterDlg.h"

XScanner::XScanner(const char* source)
{
	m_start = source;
	m_current = source;
	m_line = 1;
	m_hadError = false;
	m_panicMode = false;
}

XScanner::Token XScanner::scanToken()
{
	skipWhitespace();

	m_start = m_current;

	if (isAtEnd()) 
		return makeToken(TokenT::_EOF);

	char c = advance();

	if (isAlpha(c)) return identifier();
	if (isDigit(c)) return number();

	switch (c) {
	case '(': return makeToken(TokenT::LEFT_PAREN);
	case ')': return makeToken(TokenT::RIGHT_PAREN);
	case '{': return makeToken(TokenT::LEFT_BRACE);
	case '}': return makeToken(TokenT::RIGHT_BRACE);
	case ';': return makeToken(TokenT::SEMICOLON);
	case ',': return makeToken(TokenT::COMMA);
	case '.': return makeToken(TokenT::DOT);
	case '-': return makeToken(TokenT::MINUS);
	case '+': return makeToken(TokenT::PLUS);
	case '/': return makeToken(TokenT::SLASH);
	case '*': return makeToken(TokenT::STAR);
	case '!':
		return makeToken(match('=') ? TokenT::BANG_EQUAL : TokenT::BANG);
	case '=':
		return makeToken(match('=') ? TokenT::EQUAL_EQUAL : TokenT::EQUAL);
	case '<':
		return makeToken(match('=') ? TokenT::LESS_EQUAL : TokenT::LESS);
	case '>':
		return makeToken(match('=') ?
			TokenT::GREATER_EQUAL : TokenT::GREATER);
	case '"': return stringToken();
	case '&': if (match('&')) return makeToken(TokenT::AND); else break;
	case '|': if (match('|')) return makeToken(TokenT::OR); else break;
	}

	return errorToken("Unexpected character.");
}

XScanner::Token XScanner::makeToken(TokenT type)
{
	Token token;
	token.type = type;
	token.start = m_start;
	token.length = (int)(m_current - m_start);
	token.line = m_line;

	return token;
}

XScanner::Token XScanner::errorToken(const char* message)
{
	Token token;
	token.type = TokenT::_ERROR;
	token.start = message;
	token.length = (int)strlen(message);
	token.line = m_line;

	return token;
}

bool XScanner::match(char expected) 
{
	if (isAtEnd()) return false;
	if (*m_current != expected) return false;

	m_current++;
	return true;
}

void XScanner::skipWhitespace() 
{
	for (;;) {
		char c = peek();
		switch (c) {
		case ' ':
		case '\r':
		case '\t':
			advance();
			break;
		case '\n':
			m_line++;
			advance();
			break;
		case '/':
			if (peekNext() == '/') {
				// A comment goes until the end of the line.   
				while (peek() != '\n' && !isAtEnd()) advance();
			}
			else {
				return;
			}
			break;
		default:
			return;
		}
	}
}

XScanner::Token XScanner::stringToken()
{
	while (peek() != '"' && !isAtEnd()) {
		if (peek() == '\n') m_line++;
		advance();
	}

	if (isAtEnd()) return errorToken("Unterminated string.");

	// The closing ".                                        
	advance();
	return makeToken(TokenT::STRING);
}

XScanner::Token XScanner::number()
{
	while (isDigit(peek())) 
		advance();

	// Look for a fractional part.             
	if (peek() == '.' && isDigit(peekNext())) {
		// Consume the "."                       
		advance();

		while (isDigit(peek())) advance();
	}

	if (peek() == 'e' || peek() == 'E')
	{
		if (isDigit(peekNext()))
		{
			// consume the 'e'
			advance();

			while (isDigit(peek()))
				advance();
		}
		else if ((peekNext() == '-' || peekNext() == '+') &&
			isDigit(peekNextNext()))
		{
			// consume the 'e' and the sign
			advance();
			advance();

			while (isDigit(peek()))
				advance();
		}
	}

	return makeToken(TokenT::NUMBER);
}

XScanner::Token XScanner::identifier()
{
	while (isAlpha(peek()) || isDigit(peek())) advance();

	return makeToken(identifierType());
}

TokenT XScanner::identifierType()
{
	switch (m_start[0]) {
	case 'a': return checkKeyword(1, "nd", TokenT::AND);
	case 'c': return checkKeyword(1, "lass", TokenT::CLASS);
	case 'e': return checkKeyword(1, "lse", TokenT::ELSE);
	case 'f':
		if (m_current - m_start > 1)
		{
			switch (m_start[1])
			{
			case 'a': return checkKeyword(2, "lse", TokenT::_FALSE);
			case 'o': return checkKeyword(2, "r", TokenT::FOR);
			case 'u': return checkKeyword(2, "n", TokenT::FUN);
			}
		}
		break;
	case 'i': return checkKeyword(1, "f", TokenT::IF);
	case 'n': return checkKeyword(1, "il", TokenT::NIL);
	case 'o': return checkKeyword(1, "r", TokenT::OR);
	case 'p': return checkKeyword(1, "rint", TokenT::PRINT);
	case 'r': return checkKeyword(1, "eturn", TokenT::RETURN);
	case 's': return checkKeyword(1, "uper", TokenT::SUPER);
	case 't':
		if (m_current - m_start > 1) {
			switch (m_start[1]) {
			case 'h': return checkKeyword(2, "is", TokenT::_THIS);
			case 'r': return checkKeyword(2, "ue", TokenT::_TRUE);
			}
		}
		break;
	case 'u': return checkKeyword(1, "sing", TokenT::USING);
	case 'v': return checkKeyword(1, "ar", TokenT::VAR);
	case 'w': return checkKeyword(1, "hile", TokenT::WHILE);
	}

	return TokenT::IDENTIFIER;
}

void XScanner::errorAt(const Token& token, const char* message)
{
	if (m_panicMode) return;
	m_panicMode = true;

	DLG()->m_output.AppendFormat("[line %d] Error", token.line);

	if (token.type == TokenT::_EOF)
	{
		DLG()->m_output.AppendFormat(" at end");
	}
	else if (token.type == TokenT::_ERROR)
	{
		// Nothing.                                                
	}
	else
	{
		DLG()->m_output.AppendFormat(" at '%.*s'", token.length, token.start);
	}

	DLG()->m_output.AppendFormat(": %s\n", message);
	m_hadError = true;
}

void XScanner::synchronize()
{
	m_panicMode = false;

	while (m_currentToken.type != TokenT::_EOF)
	{
		if (m_previousToken.type == TokenT::SEMICOLON)
			return;

		switch (m_currentToken.type) {
		case TokenT::CLASS:
		case TokenT::FUN:
		case TokenT::VAR:
		case TokenT::USING:
		case TokenT::FOR:
		case TokenT::IF:
		case TokenT::WHILE:
		case TokenT::PRINT:
		case TokenT::RETURN:
			return;

		default:
			// Do nothing.                                  
			;
		}

		advanceToken();
	}
}

void XScanner::advanceToken()
{
	m_previousToken = m_currentToken;

	for (;;) {
		m_currentToken = scanToken();
		if (m_currentToken.type != TokenT::_ERROR) break;

		errorAtCurrent(m_currentToken.start);
	}
}

void XScanner::consume(TokenT type, const char* message)
{
	if (m_currentToken.type == type)
	{
		advanceToken();
		return;
	}

	errorAtCurrent(message);
}

bool XScanner::match(TokenT type)
{
	if (!check(type))
		return false;
	advanceToken();
	return true;
}
