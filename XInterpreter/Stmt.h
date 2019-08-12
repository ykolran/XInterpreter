#pragma once
#include "Token.h"
#include <memory>
#include <string>
#include "Expr.h"

// FW declarations
struct StmtVisitor;
struct Block;
struct Expression;
struct Print;
struct Var;

struct Stmt
{
    virtual void accept(StmtVisitor& visitor) const = 0;
    virtual ~Stmt() { }
};

struct StmtVisitor
{
    virtual void visitBlockStmt(const Block& stmt) = 0;
    virtual void visitExpressionStmt(const Expression& stmt) = 0;
    virtual void visitPrintStmt(const Print& stmt) = 0;
    virtual void visitVarStmt(const Var& stmt) = 0;
};

struct Block : Stmt
{
    Block(std::vector<std::unique_ptr<Stmt>>&& statements) : 
        m_statements(move(statements))
        { }

    virtual void accept(StmtVisitor& visitor) const override
    {
        return visitor.visitBlockStmt(*this);
    }

    std::vector<std::unique_ptr<Stmt>> m_statements;
};

struct Expression : Stmt
{
    Expression(std::unique_ptr<Expr>&& expression) : 
        m_expression(move(expression))
        { }

    virtual void accept(StmtVisitor& visitor) const override
    {
        return visitor.visitExpressionStmt(*this);
    }

    std::unique_ptr<Expr> m_expression;
};

struct Print : Stmt
{
    Print(std::unique_ptr<Expr>&& expression) : 
        m_expression(move(expression))
        { }

    virtual void accept(StmtVisitor& visitor) const override
    {
        return visitor.visitPrintStmt(*this);
    }

    std::unique_ptr<Expr> m_expression;
};

struct Var : Stmt
{
    Var(Token name,std::unique_ptr<Expr>&& initializer) : 
        m_name(name),
        m_initializer(move(initializer))
        { }

    virtual void accept(StmtVisitor& visitor) const override
    {
        return visitor.visitVarStmt(*this);
    }

    Token m_name;
    std::unique_ptr<Expr> m_initializer;
};

