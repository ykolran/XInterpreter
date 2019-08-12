#pragma once
#include "TokenType.h"

class XScanner
{
public:
	struct Token
	{
		TokenT type;
		const char* start;
		int length;
		int line;
	};

	XScanner(const char* source);

	inline const Token& current() const { return m_currentToken; }
	inline const Token& previous() const { return m_previousToken; }

	void advanceToken();
	bool check(TokenT type) const { return m_currentToken.type == type; }
	bool match(TokenT type);
	void consume(TokenT type, const char* message);
	void synchronize();

	bool hadError() const { return m_hadError; }
	bool panicMode() const { return m_panicMode; }

	void errorAt(const XScanner::Token& token, const char* message);

private:
	void errorAtCurrent(const char* message) { errorAt(m_currentToken, message); }

	inline bool isAtEnd() const { return *m_current == '\0'; }
	inline char advance() { m_current++; return m_current[-1]; }
	inline char peek() { return *m_current; }
	inline char peekNext() { if (isAtEnd()) return '\0'; return m_current[1]; }
	inline char peekNextNext() { if (*m_current == '\0' || *(m_current+1) == '\0') return '\0'; return m_current[2]; }
	inline bool isDigit(char c) { return c >= '0' && c <= '9'; }
	inline bool isAlpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
	TokenT identifierType();
	template <int N>
	TokenT checkKeyword(int start, const char(& rest)[N] , TokenT type)
	{
		// N contains the '\0' character, we check the keyword without it
		if (m_current - m_start == start + N - 1 &&
			memcmp(m_start + start, rest, N - 1) == 0)
		{
			return type;
		}

		return TokenT::IDENTIFIER;
	}

	Token scanToken();

	bool match(char expected);
	void skipWhitespace();
	Token makeToken(TokenT type);
	Token errorToken(const char* message);
	Token stringToken();
	Token number();
	Token identifier();

	const char* m_start;
	const char* m_current;
	int m_line;

	Token m_currentToken;
	Token m_previousToken;

	bool m_hadError;
	bool m_panicMode;
};