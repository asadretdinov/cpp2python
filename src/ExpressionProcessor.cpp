#include "ExpressionProcessor.h"
#include "StatementVisitor.h"
#include "clang/AST/ExprCXX.h"
#include <sstream>

typedef std::pair<std::string, size_t> ExprResult;
ExprResult processExprInt(const Expr* E);

std::map<BinaryOperator::Opcode, std::string> strs4opcode = {
	{ BO_Add, "+"},
	{ BO_Sub, "-"},
	{ BO_Div, "/"},
	{ BO_Mul, "*"},
	{ BO_GT, ">" },
	{ BO_GE, ">=" },
	{ BO_LT, "<" },
	{ BO_LE, "<=" },
	{ BO_EQ, "==" },
	{ BO_Assign, "=" },
	{ BO_And, " and "},
	{ BO_LAnd, " and "},
	{ BO_LOr, " or "},
	{ BO_Or, " or "},
	{ BO_MulAssign, " *= "},
	{ BO_AddAssign, " += "},
	{ BO_DivAssign, " /= "},
	{ BO_SubAssign, " -= "}
};

std::map<UnaryOperator::Opcode, std::string> strs4uopcode = {
	{ UO_PostDec, "--"},
	{ UO_PreDec, "--"},
	{ UO_PostInc, "++"},
	{ UO_PreInc, "++"}
};

std::string opcode2Str(BinaryOperator::Opcode code) {
	auto it = strs4opcode.find(code);
	if (it != strs4opcode.end()) {
		return it->second;
	}
	else {
		return "<opcode>";
	}
}

std::string opcode2Str(UnaryOperator::Opcode code) {
	auto it = strs4uopcode.find(code);
	if (it != strs4uopcode.end()) {
		return it->second;
	}
	else {
		return "<uopcode>";
	}
}

std::string getExpResultAsStr(const ExprResult& res, BinaryOperator::Opcode code) {
	if (res.second == 1) {
		return res.first;
	}
	else {
		return std::string("(") + res.first + std::string(")");
	}
}

ExprResult processBinaryOperator(const BinaryOperator* B) {
	auto code = B->getOpcode();

	auto left = processExprInt(B->getLHS());
	auto right = processExprInt(B->getRHS());
	std::string res = getExpResultAsStr(left, code) + opcode2Str(code) + getExpResultAsStr(right, code);

	size_t level = std::max(left.second, right.second);
	if (code != BO_Mul)
		return { res , level + 1 };
	else 
		return { res , level };
}

ExprResult processImplicitCast(const ImplicitCastExpr* E) {
	const auto* ee = E->getSubExpr();
	//TODO check types
	return processExprInt(ee);
}

ExprResult processFloatingLiteral(const FloatingLiteral* F) {
	std::stringstream str;
	str << F->getValueAsApproximateDouble();
	return { str.str(), 1 };
}

ExprResult processIntegerLiteral(const IntegerLiteral* F) {
	return { F->getValue().toString(10, false), 1 };
}

ExprResult processBoolLiteral(const CXXBoolLiteralExpr* B) {
	return { B->getValue() ? "True" : "False", 1 };
}

ExprResult processDeclRef(const DeclRefExpr* D) {
	const auto* v = D->getDecl();
	if (v != nullptr) {
		return { v->getNameAsString(), 1 };
	}
	else {
		return {"<unknown variable>", 1 };
	}
}

ExprResult processCall(const CallExpr* C) {
	const auto* d = C->getCalleeDecl();
	if (const auto* f = dyn_cast<FunctionDecl>(d); f != nullptr) {
		std::stringstream str;
		auto fName = f->getNameAsString();
		if (fName == "operator[]") {
			auto first = processExpr(C->getArg(0));
			auto second = processExpr(C->getArg(1));
			str << first << "[" << second << "]";
		}
		else if (fName == "operator()") {
			auto first = processExpr(C->getArg(0));
			str << first << "(";
			for (size_t i = 1; i < C->getNumArgs(); ++i) {
				if (i > 1) str << ", ";
				str << processExprInt(C->getArg(i)->getExprStmt()).first;
			}
			str << ")";
		}
		else {
			str << fName + "(";
			for (size_t i = 0; i < C->getNumArgs(); ++i) {
				if (i > 0) str << ", ";
				str << processExprInt(C->getArg(i)->getExprStmt()).first;
			}
			str << ")";
		}
		return { str.str(), 1 };
	}
	else {
		return { "<unknown call>", 1 };
	}
}

ExprResult processMember(const MemberExpr* M) {
	const auto* member = M->getMemberDecl();
	const auto* object = M->getBase();

	std::stringstream str;
	str << processExpr(object) << "." << member->getNameAsString();
	return { str.str(), 1 };
}

ExprResult processMemberCall(const CXXMemberCallExpr* M) {
	std::stringstream str;

	const auto* member = M->getMethodDecl();
	const Expr* object = M->getImplicitObjectArgument();

	auto mName = member->getNameAsString();
	// replace size() -> len()
	if (mName == "size" && M->getNumArgs() == 0) {
		str << "len(" << processExpr(object) << ")";
	}
	else if (mName == "operator[]") {
		auto first = processExpr(M->getArg(0));
		auto second = processExpr(M->getArg(1));
		str << first << "[" << second << "]";
	}
	else {
		str << processExpr(object) << "." << member->getNameAsString() << "(";

		size_t idx = 0;
		for (const auto* p : M->arguments()) {
			str << (idx++ > 0 ? ", " : "") << processExpr(p);
		}
		str << ")";
	}

	return { str.str(), 1 };
}

ExprResult processThis(const CXXThisExpr* Th) {
	return { "self", 1 };
}

ExprResult processDefaultInit(const CXXDefaultInitExpr* Ie) {
	return { processExpr(Ie->getExpr()) , 1 };
}

ExprResult processCXXConstruct(const CXXConstructExpr* C) {
	std::string typeName(C->getConstructor()->getParent()->getNameAsString());
	std::stringstream str;
	str << typeName << "(";
	size_t idx = 0;
	for (const auto* p : C->arguments()) {
		if (p->isDefaultArgument()) {
			continue;
		}
		str << (idx++ > 0 ? ", " : "") << processExpr(p);
	}
	str << ")";

	return { str.str(), 1 };
}

ExprResult processConstant(const ConstantExpr* C) {
	return { processExpr(C->getSubExpr()) , 1 };
}

ExprResult processExprWithCleanups(const ExprWithCleanups* E) {
	return { processExpr(E->getSubExpr()) , 1 };
}

ExprResult processCXXStdInitializerList(const CXXStdInitializerListExpr* L) {
	return { processExpr(L->getSubExpr()), 1 };
}

ExprResult processMaterializeTemporary(const MaterializeTemporaryExpr* E) {
	return { processExpr(E->getSubExpr()), 1 };
}

ExprResult processInitList(const InitListExpr* L) {
	std::stringstream str;
	str << "[";
	for (size_t i = 0; i < L->getNumInits(); i++) {
		str << (i > 0 ? ", " : "") << processExpr(L->getInit(i));
	}
	str << "]";
	return { str.str(), 1 };
}

ExprResult processParenExpr(const ParenExpr* E) {
	return processExprInt(E->getSubExpr());
}

ExprResult processUnaryOperator(const UnaryOperator* O) {
	// copy from StatementVisitor::VisitUnaryOperator()
	auto expr = processExpr(O->getSubExpr());
	auto code = O->getOpcode();
	switch (code)
	{
	case UnaryOperator::Opcode::UO_PostDec:
	case UnaryOperator::Opcode::UO_PreDec:
		return { expr + " = " + expr + " - 1", 1 };
	case UnaryOperator::Opcode::UO_PostInc:
	case UnaryOperator::Opcode::UO_PreInc:
		return { expr + " = " + expr + " + 1", 1 };
	case UnaryOperator::Opcode::UO_Not:
	case UnaryOperator::Opcode::UO_LNot:
		return { "not " + expr, 1 };
	case UnaryOperator::Opcode::UO_Minus:
		return { "-(" + expr + ")", 1 };
	default:
		return { "<unknown type of unary statement>" , 1 };
	}
}

ExprResult processCXXFunctionalCast(const CXXFunctionalCastExpr* E) {
	auto subExpr = processExpr(E->getSubExpr());

	auto type = E->getTypeInfoAsWritten()->getType().getAsString();
	// c++ -> python type conversion
	//@TODO fix
	if (type == "double") type = "float";

	return { type + "(" + subExpr + ")", 1 };
}

ExprResult processConditionalOperator(const ConditionalOperator* O) {
	auto cond = processExpr(O->getCond());
	auto trueExpr = processExpr(O->getTrueExpr());
	auto falseExpr = processExpr(O->getFalseExpr());
	return { trueExpr + " if " + cond + " else " + falseExpr , 1 };
}

ExprResult processLambdaExpression(const LambdaExpr* L) {
	if (L->getBody()) {
		StatementVisitor v(L->getBody());
		auto body = v.getLines();

		if (body.size() == 1) {
			std::stringstream head;
			head << "lambda ";
			const auto* lambdaClass = L->getLambdaClass();
			for (const auto* m : lambdaClass->methods()) {
				auto name = m->getNameAsString();
				
				if (name == "operator()") {
					size_t idx = 0;
					for (const auto* p : m->parameters()) {
						head << (idx++ > 0 ? "," : "") << p->getNameAsString();
					}
				}
			}
			head << ": ";

			return { head.str() + *body.begin(), 1 };
		}
		else {
			return { "<multiline_lambda>", 1 };
		}
	}
}

ExprResult processExprInt(const Expr* E) {
	if (E == nullptr) {
		return { "<null expression>", 1 };
	}

	if (isa<ConstantExpr>(E)) {
		return processConstant(dyn_cast<ConstantExpr>(E));
	}
	if (isa<ExprWithCleanups>(E)) {
		return processExprWithCleanups(dyn_cast<ExprWithCleanups>(E));
	}
	if (isa<CXXConstructExpr>(E)) {
		return processCXXConstruct(dyn_cast<CXXConstructExpr>(E));
	}
	if (isa<CXXStdInitializerListExpr>(E)) {
		return processCXXStdInitializerList(dyn_cast<CXXStdInitializerListExpr>(E));
	}
	if (isa<MaterializeTemporaryExpr>(E)) {
		return processMaterializeTemporary(dyn_cast<MaterializeTemporaryExpr>(E));
	}
	if (isa<InitListExpr>(E)) {
		return processInitList(dyn_cast<InitListExpr>(E));
	}
	if (isa<ParenExpr>(E)) {
		return processParenExpr(dyn_cast<ParenExpr>(E));
	}
	if (isa<UnaryOperator>(E)) {
		return processUnaryOperator(dyn_cast<UnaryOperator>(E));
	}
	if (isa<LambdaExpr>(E)) {
		return processLambdaExpression(dyn_cast<LambdaExpr>(E));
	}
	if (const auto* b = dyn_cast<BinaryOperator>(E); b != nullptr) {
		return processBinaryOperator(b);
	}
	if (isa<ImplicitCastExpr>(E)) {
		return processImplicitCast(dyn_cast<ImplicitCastExpr>(E));
	}
	if (isa<ConditionalOperator>(E)) {
		return processConditionalOperator(dyn_cast<ConditionalOperator>(E));
	}
	if (const auto* l = dyn_cast<FloatingLiteral>(E); l != nullptr) {
		return processFloatingLiteral(l);
	}
	if (const auto* l = dyn_cast<IntegerLiteral>(E); l != nullptr) {
		return processIntegerLiteral(l);
	}
	if (const auto* b = dyn_cast<CXXBoolLiteralExpr>(E); b != nullptr) {
		return processBoolLiteral(b);
	}
	if (const auto* d = dyn_cast<DeclRefExpr>(E); d != nullptr) {
		return  processDeclRef(d);
	}
	// Тут есть нюанс при использовании dyn_cast<>. Если предок идет в списке условий раньше наследника, то до наследника дело не дойдет. Надо переделать на isa<>
	if (const auto* m = dyn_cast<CXXMemberCallExpr>(E); m != nullptr) {
		return processMemberCall(m);
	}
	if (const auto* c = dyn_cast<CallExpr>(E); c != nullptr) {
		return  processCall(c);
	}
	if (const auto* m = dyn_cast<MemberExpr>(E); m != nullptr) {
		return processMember(m);
	}
	if (const auto* th = dyn_cast<CXXThisExpr>(E); th != nullptr) {
		return processThis(th);
	}
	if (const auto* ie = dyn_cast<CXXDefaultInitExpr>(E); ie != nullptr) {
		return processDefaultInit(ie);
	}
	if (isa<CXXFunctionalCastExpr>(E)) {
		return processCXXFunctionalCast(dyn_cast<CXXFunctionalCastExpr>(E));
	}

	E->dumpColor();
	return { "<unknown expression>", 1 };
}

std::string processExpr(const Expr* E) {
	return processExprInt(E).first;
}

std::optional<ParsedBinaryExpr> getParsedBinaryExpr(const Expr* E)
{
	const auto* B = dyn_cast<BinaryOperator>(E);
	if (B == nullptr) {
		return std::nullopt;
	}

	auto code = B->getOpcode();
	auto left = processExprInt(B->getLHS());
	auto right = processExprInt(B->getRHS());

	return ParsedBinaryExpr{ getExpResultAsStr(left, code), opcode2Str(code), getExpResultAsStr(right, code) };
}

std::optional<ParsedUnaryExpr> getParsedUnaryExpr(const Expr * E)
{
	const auto* B = dyn_cast<UnaryOperator>(E);
	if (B == nullptr) {
		return std::nullopt;
	}

	auto code = B->getOpcode();
	auto expr = processExprInt(B->getSubExpr());

	return ParsedUnaryExpr{ expr.first, opcode2Str(code) };
}
