#pragma once
#include "Token.h"
#include <memory>
#include <string>

// FW declarations
struct Visitor;
struct Binary;
struct Grouping;
struct Literal;
struct Unary;

struct Expr
{
    virtual std::string accept(Visitor& visitor) const = 0;
};

struct Visitor
{
    virtual std::string visitBinaryExpr(const Binary& expr) = 0;
    virtual std::string visitGroupingExpr(const Grouping& expr) = 0;
    virtual std::string visitLiteralExpr(const Literal& expr) = 0;
    virtual std::string visitUnaryExpr(const Unary& expr) = 0;
};

struct Binary : Expr
{
    Binary(Expr* left,Token* op,Expr* right) : 
        m_left(left),
        m_op(op),
        m_right(right)
        { }

    virtual std::string accept(Visitor& visitor) const override
    {
        return visitor.visitBinaryExpr(*this);
    }

    std::unique_ptr<Expr> m_left;
    std::unique_ptr<Token> m_op;
    std::unique_ptr<Expr> m_right;
};

struct Grouping : Expr
{
    Grouping(Expr* expression) : 
        m_expression(expression)
        { }

    virtual std::string accept(Visitor& visitor) const override
    {
        return visitor.visitGroupingExpr(*this);
    }

    std::unique_ptr<Expr> m_expression;
};

struct Literal : Expr
{
    Literal(double* value) : 
        m_value(value)
        { }

    virtual std::string accept(Visitor& visitor) const override
    {
        return visitor.visitLiteralExpr(*this);
    }

    std::unique_ptr<double> m_value;
};

struct Unary : Expr
{
    Unary(Token* op,Expr* right) : 
        m_op(op),
        m_right(right)
        { }

    virtual std::string accept(Visitor& visitor) const override
    {
        return visitor.visitUnaryExpr(*this);
    }

    std::unique_ptr<Token> m_op;
    std::unique_ptr<Expr> m_right;
};

