#pragma once
#include "XToken.h"

class XScanner
{
public:
	XScanner(const char* source);

	inline const XTokenData& current() const { return m_currentToken; }
	inline const XTokenData& previous() const { return m_previousToken; }

	void advanceToken();
	bool check(XToken type) const { return m_currentToken.type == type; }
	bool match(XToken type);
	void consume(XToken type, const char* message);
	void synchronize();

	bool hadError() const { return m_hadError; }
	bool panicMode() const { return m_panicMode; }

	void errorAt(const XTokenData& token, const char* message);
	void errorAtCurrent(const char* message) { errorAt(m_currentToken, message); }

private:

	inline bool isAtEnd() const { return *m_current == '\0'; }
	inline char advance() { m_current++; return m_current[-1]; }
	inline char peek() { return *m_current; }
	inline char peekNext() { if (isAtEnd()) return '\0'; return m_current[1]; }
	inline char peekNextNext() { if (*m_current == '\0' || *(m_current+1) == '\0') return '\0'; return m_current[2]; }
	inline bool isDigit(char c) { return c >= '0' && c <= '9'; }
	inline bool isAlpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
	XToken identifierType();
	template <int N>
	XToken checkKeyword(int start, const char(& rest)[N] , XToken type)
	{
		// N contains the '\0' character, we check the keyword without it
		if (m_current - m_start == start + N - 1 &&
			memcmp(m_start + start, rest, N - 1) == 0)
		{
			return type;
		}

		return XToken::IDENTIFIER;
	}

	XTokenData scanToken();

	bool match(char expected);
	void skipWhitespace();
	XTokenData makeToken(XToken type);
	XTokenData errorToken(const char* message);
	XTokenData stringToken();
	XTokenData number();
	XTokenData identifier();

	const char* m_start;
	const char* m_current;
	int m_line;

	XTokenData m_currentToken;
	XTokenData m_previousToken;

	bool m_hadError;
	bool m_panicMode;
};