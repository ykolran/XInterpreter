#pragma once
#include "TokenType.h"
#include <string>

class Token
{
public:
	TokenT		m_type;
	std::string	m_lexeme;
	double		m_value;
	int			m_line;

	Token(TokenT type, std::string lexeme, double value, int line) {
		m_type = type;
		m_lexeme = lexeme;
		m_value = value;
		m_line = line;
	}

	CString toString() {
		CString str;
		str.Format("%d %s %lf", m_type, m_lexeme.c_str(), m_value);
		return str;
	}
};

