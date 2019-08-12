#pragma once

#include "Expr.h"
#include <sstream>
#include "Environment.h"

class Interpreter : ExprVisitor, StmtVisitor
{
public:
	Interpreter() : 
		m_environment(std::make_unique<Environment>(nullptr))
	{

	}

	void interpret(const std::vector<std::unique_ptr<Stmt>>& program)
	{
		for (const auto& stmt : program)
		{
			DLG()->m_hadRuntimeError = false;
			execute(stmt);
		}
	}

private:
	void execute(const std::unique_ptr<Stmt>& stmt)
	{
		if (stmt)
			stmt->accept(*this);
	}

	virtual void visitBlockStmt(const Block& stmt) override
	{
		m_environment = std::make_unique<Environment>(std::move(m_environment));

		for (const auto& statement : stmt.m_statements)
		{
			execute(statement);
		}

		m_environment = std::move(m_environment->m_parent);
	}

	virtual void visitExpressionStmt(const Expression& stmt) override
	{
		evaluate(*stmt.m_expression);
	}

	virtual void visitPrintStmt(const Print& stmt) override
	{
		evaluate(*stmt.m_expression);

		if (!DLG()->m_hadRuntimeError)
		{
			if (m_type == TokenT::NUMBER)
			{
				std::ostringstream oss;
				oss << m_numberValue;
				DLG()->m_output += oss.str().c_str();
				DLG()->m_output += "\r\n";
			}
			else
			{
				DLG()->m_output += m_stringValue.c_str();
				DLG()->m_output += "\r\n";
			}
		}
	}

	virtual void visitVarStmt(const Var& stmt) override
	{
		if (stmt.m_initializer) 
		{
			evaluate(*stmt.m_initializer);
			if (m_type != TokenT::NUMBER)
				m_numberValue = 0.0;
		}
		else
			m_numberValue = 0.0;

		m_environment->define(stmt.m_name.m_lexeme, m_numberValue);
	}

	virtual void visitAssignExpr(const Assign& expr) override
	{
		evaluate(*expr.m_value);

		if (m_type == TokenT::NUMBER)
		{
			bool assigned = m_environment->assign(expr.m_name, m_numberValue);
			if (!assigned)
				DLG()->runTimeError(expr.m_name,
					"Undefined variable '" + expr.m_name.m_lexeme + "'.");
		}
		else
			DLG()->runTimeError(expr.m_name, "can only assign number");
	}

	virtual void visitBinaryExpr(const Binary& expr) override
	{
		evaluate(*expr.m_left);
		if (m_type != TokenT::NUMBER)
			DLG()->runTimeError(expr.m_op, "left operand must be a number");
		double left = m_numberValue;

		evaluate(*expr.m_right);
		if (m_type != TokenT::NUMBER)
			DLG()->runTimeError(expr.m_op, "right operand must be a number");
		double right = m_numberValue;

		switch (expr.m_op.m_type) {
		case TokenT::PLUS:
			m_numberValue = left + right;
			break;
		case TokenT::MINUS:
			m_numberValue = left - right;
			break;
		case TokenT::SLASH:
			m_numberValue = left / right;
			break;
		case TokenT::STAR:
			m_numberValue = left * right;
			break;
		case TokenT::GREATER:
			m_numberValue = left > right;
			break;
		case TokenT::GREATER_EQUAL:
			m_numberValue = left >= right;
			break;
		case TokenT::LESS:
			m_numberValue = left < right;
			break;
		case TokenT::LESS_EQUAL:
			m_numberValue = left <= right;
			break;
		case TokenT::BANG_EQUAL:
			m_numberValue = left != right;
			break;
		case TokenT::EQUAL_EQUAL:
			m_numberValue = left == right;
			break;
		}
	}

	virtual void visitGroupingExpr(const Grouping& expr) override
	{
		evaluate(*expr.m_expression);
	}

	virtual void visitLiteralExpr(const Literal& expr) override
	{
		m_type = TokenT::NUMBER;
		m_numberValue = expr.m_value;
	}

	virtual void visitStringExpr(const String& expr) override
	{
		m_type = TokenT::STRING;
		m_stringValue = expr.m_value;
	}

	virtual void visitUnaryExpr(const Unary& expr) override
	{
		evaluate(*expr.m_right);
		if (m_type != TokenT::NUMBER)
			DLG()->runTimeError(expr.m_op, "operand must be number");

		switch (expr.m_op.m_type) {
		case TokenT::MINUS:
			m_numberValue = -m_numberValue;
			break;
		case TokenT::BANG:
			m_numberValue = !m_numberValue;
			break;
		}
	}

	virtual void visitVariableExpr(const Variable& expr) override
	{
		 auto var = m_environment->get(expr.m_name);
		 if (var.first)
		 {
			 m_type = TokenT::NUMBER;
			 m_numberValue = var.second;
		 }
		 else
		 {
			 DLG()->runTimeError(expr.m_name, std::string("Undefined variable \"") + expr.m_name.m_lexeme + "\"");
		 }
	}

	void evaluate(const Expr& expr)
	{
		expr.accept(*this);
	}

	bool m_printStatement = false;
	TokenT m_type = TokenT::NIL;
	double m_numberValue = 0.0;
	std::string m_stringValue;
	std::unique_ptr<Environment> m_environment;
};
