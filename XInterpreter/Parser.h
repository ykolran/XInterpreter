#pragma once

#include <vector>
#include "Token.h"
#include "Expr.h"
#include "Stmt.h"
#include "XInterpreterDlg.h"

/*
program			→	declaration* EOF

declaration		→	varDecl | statement

varDecl			→	"var" IDENTIFIER ( "=" expression )? ";"
statement		→	exprStmt | printStmt | block

exprStmt		→	expression ";" 
printStmt		→	"print" expression ";" 
block			→	"{" declaration* "}"

expression		→	assignment
assignment		→	IDENTIFIER "=" assignment | equality
equality		→	comparison(("!=" | "==") comparison)*
comparison		→	addition((">" | ">=" | "<" | "<=") addition)*
addition		→	multiplication(("-" | "+") multiplication)*
multiplication	→	unary(("/" | "*") unary)*
unary			→	("!" | "-") unary | primary
primary			→	NUMBER | STRING | "false" | "true" | "(" expression ")" | IDENTIFIER
*/

class Parser 
{
public:
	Parser(std::vector<Token> tokens) :
		m_tokens(tokens)
	{
	}

	std::vector<std::unique_ptr<Stmt>> parse() 
	{
		std::vector<std::unique_ptr<Stmt>> statements;
		while (!isAtEnd()) {
			auto dec = declaration();
			if (!DLG()->m_hadError)
				statements.push_back(std::move(dec));
			else
			{
				synchronize();
				DLG()->m_hadError = false;
			}
		}

		return statements;
	}
private:
	std::unique_ptr<Stmt> declaration() 
	{
		if (match({ TokenT::VAR })) 
			return varDeclaration();

		return statement();
	}

	std::unique_ptr<Stmt> varDeclaration() 
	{
		Token name = consume(TokenT::IDENTIFIER, "Expect variable name.");

		std::unique_ptr<Expr> initializer;
		if (match({ TokenT::EQUAL })) {
			initializer = expression();
		}

		consume(TokenT::SEMICOLON, "Expect ';' after variable declaration.");
		return std::make_unique<Var>(name, std::move(initializer));
	}

	std::unique_ptr<Stmt> statement() 
	{
		if (match({ TokenT::PRINT })) 
			return printStatement();
		if (match({ TokenT::LEFT_BRACE })) 
			return std::make_unique<Block>(block());
		return expressionStatement();
	}

	std::vector<std::unique_ptr<Stmt>> block() 
	{
		std::vector<std::unique_ptr<Stmt>> statements;

		while (!check(TokenT::RIGHT_BRACE) && !isAtEnd()) 
		{
			statements.push_back(declaration());
		}

		consume(TokenT::RIGHT_BRACE, "Expect '}' after block.");
		return statements;
	}
	
	std::unique_ptr<Stmt> printStatement() 
	{
		auto value = expression();
		consume(TokenT::SEMICOLON, "Expect ';' after value.");
		return std::make_unique<Print>(std::move(value));
	}

	std::unique_ptr<Stmt> expressionStatement() {
		auto expr = expression();
		consume(TokenT::SEMICOLON, "Expect ';' after expression.");
		return std::make_unique<Expression>(std::move(expr));
	}

	std::unique_ptr<Expr> expression()
	{
		return assignment();
	}

	std::unique_ptr<Expr> assignment()
	{
		auto expr = equality();

		if (match({ TokenT::EQUAL })) 
		{
			Token equals = previous();
			auto value = assignment();

			Variable* variable = dynamic_cast<Variable*>(expr.get());
			if (variable) {
				Token name = variable->m_name;
				return std::make_unique<Assign>(name, std::move(value));
			}

			error(equals, "Invalid assignment target.");
		}

		return expr;
	}

	std::unique_ptr<Expr> equality()
	{
		auto expr = comparison();

		while (match({ TokenT::BANG_EQUAL, TokenT::EQUAL_EQUAL })) {
			Token op = previous();
			auto right = comparison();
			expr = std::make_unique<Binary>(std::move(expr), op, std::move(right));
		}

		return expr;
	} 

	std::unique_ptr<Expr> comparison() {
		auto expr = addition();

		while (match({ TokenT::GREATER, TokenT::GREATER_EQUAL, TokenT::LESS, TokenT::LESS_EQUAL })) {
			Token op = previous();
			auto right = addition();
			expr = std::make_unique<Binary>(std::move(expr), op, std::move(right));
		}

		return expr;
	}

	std::unique_ptr<Expr> addition() {
		auto expr = multiplication();

		while (match({ TokenT::MINUS, TokenT::PLUS })) {
			Token op = previous();
			auto right = multiplication();
			expr = std::make_unique<Binary>(std::move(expr), op, std::move(right));
		}

		return expr;
	}

	std::unique_ptr<Expr> multiplication() {
		auto expr = unary();

		while (match({ TokenT::SLASH, TokenT::STAR })) 
		{
			Token op = previous();
			auto right = unary();
			expr = std::make_unique<Binary>(std::move(expr), op, std::move(right));
		}

		return expr;
	}

	std::unique_ptr<Expr> unary() {
		if (match({ TokenT::BANG, TokenT::MINUS })) {
			Token op = previous();
			auto right = unary();
			return std::make_unique<Unary>(op, std::move(right));
		}

		return primary();
	}

	std::unique_ptr<Expr> primary() {
		if (match({ TokenT::_FALSE })) return std::make_unique<Literal>(0.0);
		if (match({ TokenT::_EOF })) return std::make_unique<Literal>(1.0);
		if (match({ TokenT::NUMBER })) return std::make_unique<Literal>(previous().m_value);
		if (match({ TokenT::STRING })) return std::make_unique<String>(previous().m_lexeme);
		if (match({ TokenT::LEFT_PAREN })) {
			auto expr = expression();
			consume(TokenT::RIGHT_PAREN, "Expect ')' after expression.");
			return std::make_unique<Grouping>(std::move(expr));
		}
		if (match({ TokenT::IDENTIFIER })) {
			return std::make_unique<Variable>(previous());
		}

		error(peek(), "Expect expression.");
		return nullptr;
	}

	Token consume(TokenT type, std::string message) {
		if (check(type)) 
			return advance();

		error(peek(), message);
		return peek();
	}

	std::exception error(Token token, std::string message) {
		DLG()->error(token, message);
		return std::exception();
	}

	bool match(const std::vector<TokenT>& types) {
		for (TokenT type : types) {
			if (check(type)) {
				advance();
				return true;
			}
		}

		return false;
	}

	bool check(TokenT type) {
		if (isAtEnd()) return false;
		return peek().m_type == type;
	}

	Token advance() {
		if (!isAtEnd()) current++;
		return previous();
	}

	bool isAtEnd() {
		return peek().m_type == TokenT::_EOF;
	}

	Token peek() {
		return m_tokens[current];
	}

	Token previous() {
		return m_tokens[current - 1];
	}

	void synchronize() {
		advance();

		while (!isAtEnd()) {
			if (previous().m_type == TokenT::SEMICOLON) return;

			switch (peek().m_type) {
			case TokenT::CLASS:
			case TokenT::FUN:
			case TokenT::VAR:
			case TokenT::FOR:
			case TokenT::IF:
			case TokenT::WHILE:
			case TokenT::PRINT:
			case TokenT::RETURN:
				return;
			}

			advance();
		}
	}

	const std::vector<Token> m_tokens;
	int current = 0;

};
