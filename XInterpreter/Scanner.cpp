#include "stdafx.h"
#include "Scanner.h"
#include "XInterpreterDlg.h"

using namespace std;

std::unordered_map<std::string, TokenT> Scanner::keywords
{
	{"and", TokenT::AND},
	{"class", TokenT::CLASS},
	{"else", TokenT::ELSE},
	{"false", TokenT::_FALSE},
	{"for", TokenT::FOR},
	{"fun", TokenT::FUN},
	{"if", TokenT::IF},
	{"nil", TokenT::NIL},
	{"or", TokenT::OR},
	{"print", TokenT::PRINT},
	{"return", TokenT::RETURN},
	{"super", TokenT::SUPER},
	{"this", TokenT::_THIS},
	{"true", TokenT::_TRUE},
	{"var", TokenT::VAR},
	{"while", TokenT::WHILE}
};

const vector<Token>& Scanner::scanTokens()
{
	while (!isAtEnd()) {
		// We are at the beginning of the next lexeme.
		start = current;
		scanToken();
	}

	tokens.emplace_back(TokenT::_EOF, "", 0.0, line);
	return tokens;
}

void Scanner::scanToken() {
	wchar_t c = advance();
	switch (c) {
	case '(': addToken(TokenT::LEFT_PAREN); break;
	case ')': addToken(TokenT::RIGHT_PAREN); break;
	case '{': addToken(TokenT::LEFT_BRACE); break;
	case '}': addToken(TokenT::RIGHT_BRACE); break;
	case ',': addToken(TokenT::COMMA); break;
	case '.': addToken(TokenT::DOT); break;
	case '-': addToken(TokenT::MINUS); break;
	case '+': addToken(TokenT::PLUS); break;
	case ';': addToken(TokenT::SEMICOLON); break;
	case '*': addToken(TokenT::STAR); break;
	case '!': addToken(match('=') ? TokenT::BANG_EQUAL : TokenT::BANG); break;
	case '=': addToken(match('=') ? TokenT::EQUAL_EQUAL : TokenT::EQUAL); break;
	case '<': addToken(match('=') ? TokenT::LESS_EQUAL : TokenT::LESS); break;
	case '>': addToken(match('=') ? TokenT::GREATER_EQUAL : TokenT::GREATER); break;
	case '"': addString(); break;
	case '\n': line++; break;
	case '/':
		if (match('/')) {
			// A comment goes until the end of the line.                
			while (peek() != '\n' && !isAtEnd()) 
				advance();
		}
		else {
			addToken(TokenT::SLASH);
		}
		break;      	
	case ' ':
	case '\r':
	case '\t':
		// Ignore whitespace.                      
		break;

	default:
		if (isDigit(c)) {
			addNumber();
		}
		else if (isAlpha(c)) {
			addIdentifier();
		}
		else {
			DLG()->error(line, "Unexpected character.");
		}
		break;
	}
}

bool Scanner::isDigit(wchar_t c) {
	return c >= '0' && c <= '9';
}

bool Scanner::isAlpha(wchar_t c) {
	return (c >= 'a' && c <= 'z') ||
		(c >= 'A' && c <= 'Z') ||
		c == '_';
}

bool Scanner::isAlphaNumeric(wchar_t c)
{
	return isAlpha(c) || isDigit(c);
}
wchar_t Scanner::advance() {
	current++;
	return m_source[current - 1];
}

wchar_t Scanner::peek() {
	if (isAtEnd()) return '\0';
	return m_source[current];
}

wchar_t Scanner::peekNext() {
	if (current + 1 >= m_source.size()) return '\0';
	return m_source[current + 1];
}

wchar_t Scanner::peekNextNext() {
	if (current + 2 >= m_source.size()) return '\0';
	return m_source[current + 2];
}

bool Scanner::isAtEnd() {
	return current >= m_source.size();
}

void Scanner::addToken(TokenT type, double value) {
	int removeChars = type == TokenT::STRING ? 1 : 0;
	std::string text = m_source.substr(start + removeChars, current - start - 2*removeChars);
	tokens.emplace_back(type, text, value, line);
}

bool Scanner::match(wchar_t expected) {
	if (isAtEnd()) return false;
	if (m_source[current] != expected) return false;

	current++;
	return true;
}

void Scanner::addString() {
	while (peek() != '"' && !isAtEnd() && peek() != '\n') {
		advance();
	}

	// Unterminated string.                                 
	if (peek() != '"') {
		DLG()->error(line, "Unterminated string.");
		return;
	}

	// The closing ".                                       
	advance();

	// Trim the surrounding quotes.                         
	addToken(TokenT::STRING);
}

void Scanner::addIdentifier()
{
	while (isAlphaNumeric(peek())) advance();

	// See if the identifier is a reserved word.   
	std::string text = m_source.substr(start, current - start);

	auto it = keywords.find(text);
	TokenT type = TokenT::IDENTIFIER;
	if (it != keywords.end())
		type = it->second;

	addToken(type);
}

void Scanner::addNumber()
{
	while (isDigit(peek()))
		advance();

	// Look for a fractional part.                            
	if (peek() == '.' && isDigit(peekNext())) {
		// Consume the "."                                      
		advance();

		while (isDigit(peek()))
			advance();
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

	addToken(TokenT::NUMBER,
		atof(m_source.substr(start, current - start).c_str()));
}