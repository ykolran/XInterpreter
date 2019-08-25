#include "stdafx.h"
#include "XScanner.h"
#include "XInterpreterDlg.h"

XScanner::XScanner(const char* source)
{
	m_start = source;
	m_current = source;
	m_line = 1;
	m_hadError = false;
	m_panicMode = false;
}

XTokenData XScanner::scanToken()
{
	skipWhitespace();

	m_start = m_current;

	if (isAtEnd()) 
		return makeToken(XToken::_EOF);

	char c = advance();

	if (isAlpha(c)) return identifier();
	if (isDigit(c)) return number();

	switch (c) {
	case '(': return makeToken(XToken::LEFT_PAREN);
	case ')': return makeToken(XToken::RIGHT_PAREN);
	case '{': return makeToken(XToken::LEFT_BRACE);
	case '}': return makeToken(XToken::RIGHT_BRACE);
	case ';': return makeToken(XToken::SEMICOLON);
	case ',': return makeToken(XToken::COMMA);
	case '.': return makeToken(XToken::DOT);
	case '-': return makeToken(XToken::MINUS);
	case '+': return makeToken(XToken::PLUS);
	case '/': return makeToken(XToken::SLASH);
	case '*': return makeToken(XToken::STAR);
	case '!':
		return makeToken(match('=') ? XToken::BANG_EQUAL : XToken::BANG);
	case '=':
		return makeToken(match('=') ? XToken::EQUAL_EQUAL : XToken::EQUAL);
	case '<':
		return makeToken(match('=') ? XToken::LESS_EQUAL : XToken::LESS);
	case '>':
		return makeToken(match('=') ?
			XToken::GREATER_EQUAL : XToken::GREATER);
	case '"': return stringToken();
	case '&': if (match('&')) return makeToken(XToken::AND); else break;
	case '|': if (match('|')) return makeToken(XToken::OR); else break;
	}

	return errorToken("Unexpected character.");
}

XTokenData XScanner::makeToken(XToken type)
{
	XTokenData token;
	token.type = type;
	token.start = m_start;
	token.length = (int)(m_current - m_start);
	token.line = m_line;

	return token;
}

XTokenData XScanner::errorToken(const char* message)
{
	XTokenData token;
	token.type = XToken::_ERROR;
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

XTokenData XScanner::stringToken()
{
	while (peek() != '"' && !isAtEnd()) {
		if (peek() == '\n') m_line++;
		advance();
	}

	if (isAtEnd()) return errorToken("Unterminated string.");

	// The closing ".                                        
	advance();
	return makeToken(XToken::STRING);
}

XTokenData XScanner::number()
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

	return makeToken(XToken::NUMBER);
}

XTokenData XScanner::identifier()
{
	while (isAlpha(peek()) || isDigit(peek())) advance();

	return makeToken(identifierType());
}

XToken XScanner::identifierType()
{
	switch (m_start[0]) {
	case 'a': return checkKeyword(1, "nd", XToken::AND);
	case 'c': return checkKeyword(1, "lass", XToken::CLASS);
	case 'e': return checkKeyword(1, "lse", XToken::ELSE);
	case 'f':
		if (m_current - m_start > 1)
		{
			switch (m_start[1])
			{
			case 'a': return checkKeyword(2, "lse", XToken::_FALSE);
			case 'o': return checkKeyword(2, "r", XToken::FOR);
			case 'u': return checkKeyword(2, "n", XToken::FUN);
			}
		}
		break;
	case 'i': return checkKeyword(1, "f", XToken::IF);
	case 'n': return checkKeyword(1, "il", XToken::NIL);
	case 'o': return checkKeyword(1, "r", XToken::OR);
	case 'p': return checkKeyword(1, "rint", XToken::PRINT);
	case 'r': return checkKeyword(1, "eturn", XToken::RETURN);
	case 's': return checkKeyword(1, "uper", XToken::SUPER);
	case 't':
		if (m_current - m_start > 1) {
			switch (m_start[1]) {
			case 'h': return checkKeyword(2, "is", XToken::_THIS);
			case 'r': return checkKeyword(2, "ue", XToken::_TRUE);
			}
		}
		break;
	case 'u': return checkKeyword(1, "sing", XToken::USING);
	case 'v': return checkKeyword(1, "ar", XToken::VAR);
	case 'w': return checkKeyword(1, "hile", XToken::WHILE);
	}

	return XToken::IDENTIFIER;
}

void XScanner::errorAt(const XTokenData& token, const char* message)
{
	if (m_panicMode) return;
	m_panicMode = true;

	DLG()->m_output.AppendFormat("[line %d] Error", token.line);

	if (token.type == XToken::_EOF)
	{
		DLG()->m_output.AppendFormat(" at end");
	}
	else if (token.type == XToken::_ERROR)
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

	while (m_currentToken.type != XToken::_EOF)
	{
		if (m_previousToken.type == XToken::SEMICOLON)
			return;

		switch (m_currentToken.type) {
		case XToken::CLASS:
		case XToken::FUN:
		case XToken::VAR:
		case XToken::USING:
		case XToken::FOR:
		case XToken::IF:
		case XToken::WHILE:
		case XToken::PRINT:
		case XToken::RETURN:
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
		if (m_currentToken.type != XToken::_ERROR) break;

		errorAtCurrent(m_currentToken.start);
	}
}

void XScanner::consume(XToken type, const char* message)
{
	if (m_currentToken.type == type)
	{
		advanceToken();
		return;
	}

	errorAtCurrent(message);
}

bool XScanner::match(XToken type)
{
	if (!check(type))
		return false;
	advanceToken();
	return true;
}
