#pragma once
#include <vector>
#include <unordered_map>
#include <string>
#include <algorithm>
#include "Token.h"

class Scanner
{
public:
	Scanner(std::string source) {
		m_source = source;
	}
	const std::vector<Token>& scanTokens();

private:
	void scanToken();
	wchar_t advance();
	bool isAtEnd();
	void addToken(TokenT type, double value = 0.0);
	void addString();
	void addNumber();
	void addIdentifier();
	bool match(wchar_t expected);
	wchar_t peek();
	wchar_t peekNext();
	wchar_t peekNextNext();
	bool isDigit(wchar_t c);
	bool isAlpha(wchar_t c);
	bool isAlphaNumeric(wchar_t c);

	std::string m_source;
	std::vector<Token> tokens;
	static std::unordered_map<std::string, TokenT> keywords;
	std::size_t start = 0;
	std::size_t current = 0;
	std::size_t line = 1;
};

