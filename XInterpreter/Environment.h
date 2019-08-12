#pragma once

#include <unordered_map>
#include <string>
#include "Token.h"

class Environment
{
public:
	Environment(std::unique_ptr<Environment> parent) : 
		m_parent(std::move(parent))
	{

	}

	void define(std::string name, double value) {
		m_values[name] = value;
	}

	std::pair<bool, double> get(Token name) const
	{
		if (m_values.find(name.m_lexeme) != m_values.end())
			return std::make_pair(true, m_values.at(name.m_lexeme));
		else if (m_parent != nullptr)
			return m_parent->get(name);
		else
			return std::make_pair(false, 0.0);
	}

	bool assign(Token name, double value) 
	{
		if (m_values.find(name.m_lexeme) != m_values.end()) 
		{
			m_values[name.m_lexeme] = value;
			return true;
		}

		if (m_parent)
			return m_parent->assign(name, value);

		return false;
	}

	std::unique_ptr<Environment> m_parent;
private:
	std::unordered_map<std::string, double> m_values;
};