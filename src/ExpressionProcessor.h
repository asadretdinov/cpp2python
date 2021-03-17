#include "clang/AST/Expr.h"

using namespace clang;

// get python string from given expression. No multiline formating
std::string processExpr(const Expr* E);