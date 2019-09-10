#pragma once

enum class XToken {
	// Single-character tokens.                      
	LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,
	LEFT_BRACKET, RIGHT_BRACKET,
	COMMA, DOT, MINUS, PLUS, SEMICOLON, SLASH, STAR,

	// One or two character tokens.                  
	BANG, BANG_EQUAL,
	EQUAL, EQUAL_EQUAL,
	GREATER, GREATER_EQUAL,
	LESS, LESS_EQUAL, 
	AND, OR,

	// Literals.                                     
	IDENTIFIER, STRING, NUMBER,

	// Keywords.                                     
	CLASS, ELSE, _FALSE, FUN, FOR, IF, NIL, 
	PRINT, RETURN, SUPER, _THIS, _TRUE, VAR, WHILE,
	USING,

	_ERROR,
	_EOF
};

struct XTokenData
{
	XToken type;
	const char* start;
	unsigned int length;
	unsigned int line;
};