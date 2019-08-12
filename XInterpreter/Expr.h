#pragma once
#include "Token.h"
#include <memory>
#include <string>

// FW declarations
struct ExprVisitor;
struct Assign;
struct Binary;
struct Grouping;
struct Literal;
struct String;
struct Unary;
struct Variable;

struct Expr
{
    virtual void accept(ExprVisitor& visitor) const = 0;
    virtual ~Expr() { }
};

struct ExprVisitor
{
    virtual void visitAssignExpr(const Assign& expr) = 0;
    virtual void visitBinaryExpr(const Binary& expr) = 0;
    virtual void visitGroupingExpr(const Grouping& expr) = 0;
    virtual void visitLiteralExpr(const Literal& expr) = 0;
    virtual void visitStringExpr(const String& expr) = 0;
    virtual void visitUnaryExpr(const Unary& expr) = 0;
    virtual void visitVariableExpr(const Variable& expr) = 0;
};

struct Assign : Expr
{
    Assign(Token name,std::unique_ptr<Expr>&& value) : 
        m_name(name),
        m_value(move(value))
        { }

    virtual void accept(ExprVisitor& visitor) const override
    {
        return visitor.visitAssignExpr(*this);
    }

    Token m_name;
    std::unique_ptr<Expr> m_value;
};

struct Binary : Expr
{
    Binary(std::unique_ptr<Expr>&& left,Token op,std::unique_ptr<Expr>&& right) : 
        m_left(move(left)),
        m_op(op),
        m_right(move(right))
        { }

    virtual void accept(ExprVisitor& visitor) const override
    {
        return visitor.visitBinaryExpr(*this);
    }

    std::unique_ptr<Expr> m_left;
    Token m_op;
    std::unique_ptr<Expr> m_right;
};

struct Grouping : Expr
{
    Grouping(std::unique_ptr<Expr>&& expression) : 
        m_expression(move(expression))
        { }

    virtual void accept(ExprVisitor& visitor) const override
    {
        return visitor.visitGroupingExpr(*this);
    }

    std::unique_ptr<Expr> m_expression;
};

struct Literal : Expr
{
    Literal(double value) : 
        m_value(value)
        { }

    virtual void accept(ExprVisitor& visitor) const override
    {
        return visitor.visitLiteralExpr(*this);
    }

    double m_value;
};

struct String : Expr
{
    String(std::string value) : 
        m_value(value)
        { }

    virtual void accept(ExprVisitor& visitor) const override
    {
        return visitor.visitStringExpr(*this);
    }

    std::string m_value;
};

struct Unary : Expr
{
    Unary(Token op,std::unique_ptr<Expr>&& right) : 
        m_op(op),
        m_right(move(right))
        { }

    virtual void accept(ExprVisitor& visitor) const override
    {
        return visitor.visitUnaryExpr(*this);
    }

    Token m_op;
    std::unique_ptr<Expr> m_right;
};

struct Variable : Expr
{
    Variable(Token name) : 
        m_name(name)
        { }

    virtual void accept(ExprVisitor& visitor) const override
    {
        return visitor.visitVariableExpr(*this);
    }

    Token m_name;
};

