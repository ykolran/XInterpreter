#pragma once

#include "Expr.h"
#include <string>
#include <sstream>
#include <vector>

// Creates an unambiguous, if ugly, string representation of AST nodes.
class AstPrinter : ExprVisitor 
{
public:
	std::string print(Expr& expr) 
	{
		m_str = "";
		expr.accept(*this);
		return m_str;
	}

private:

	std::string m_str;

	virtual void visitBinaryExpr(const Binary& expr) override
	{
		parenthesize(expr.m_op.m_lexeme, { expr.m_left.get(), expr.m_right.get() });
	}

	virtual void visitGroupingExpr(const Grouping& expr) override
	{
		parenthesize("group", { expr.m_expression.get() });
	}

	virtual void visitLiteralExpr(const Literal& expr) override
	{
		std::ostringstream oss;
		oss << expr.m_value;
		m_str += oss.str();
	}

	virtual void visitStringExpr(const String& expr) override
	{
		m_str += expr.m_value;
	}

	virtual void visitUnaryExpr(const Unary& expr) override
	{
		parenthesize(expr.m_op.m_lexeme, { expr.m_right.get() });
	}

	void parenthesize(std::string name, std::vector<const Expr*> exprs) 
	{
		m_str += "(" + name;
		for (const Expr* expr : exprs) {
			m_str += " ";
			expr->accept(*this);
		}
		m_str += ")";
	}
};