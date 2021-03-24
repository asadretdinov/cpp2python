#include "clang/AST/Expr.h"
#include <string>
#include <optional>

using namespace clang;

// get python string from given expression. No multiline formating
std::string processExpr(const Expr* E);

// get L(R)HS and opcode as strings for BinaryOperator
typedef std::tuple<std::string, std::string, std::string> ParsedBinaryExpr;
std::optional<ParsedBinaryExpr> getParsedBinaryExpr(const Expr* E);

// get L(R)HS and opcode as strings for UnaryOperator
typedef std::tuple<std::string, std::string> ParsedUnaryExpr;
std::optional<ParsedUnaryExpr> getParsedUnaryExpr(const Expr* E);