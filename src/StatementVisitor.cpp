#include "StatementVisitor.h"
#include "ExpressionProcessor.h"
#include "Lines.h"

std::map<std::string, std::string> getVarsFromDecl(const DeclStmt* Node) {
	std::map<std::string, std::string> vars;
	if (Node != nullptr) {
		for (const auto* d : Node->getDeclGroup()) {
			const auto* vd = dyn_cast<VarDecl>(d);
			if (vd != nullptr) {
				vars[vd->getNameAsString()] = processExpr(vd->getInit());
			}
		}
	}
	return vars;
}

////////////////////////////////////////////////////////////////////////////////

StatementVisitor::StatementVisitor(const Stmt *Node) {
	Visit(Node);
}

std::list<std::string> StatementVisitor::getLines() const { 
	return lines; 
}

void StatementVisitor::Visit(const Stmt *Node) {
	if (Node == nullptr) {
		lines.push_back("<empty statement>");
		return;
	}

	// new node = new lines
	lines.clear();

	ConstStmtVisitor<StatementVisitor>::Visit(Node);
	// no processed lines for this node
	if (lines.empty()) {
		lines.push_back(std::string("# cannot processing statement: ") + Node->getStmtClassName());

		Node->dumpColor();
	}
}

void StatementVisitor::VisitIfStmt(const IfStmt *Node) {
	auto* exprs = Node->getCond();
	std::stringstream str;
	str << "if " << processExpr(exprs) << ":";
	lines.push_back(str.str());

	StatementVisitor thenStmtVis(Node->getThen());
	auto ThenLines = thenStmtVis.getLines();
	shiftLines(ThenLines);
	lines.insert(lines.end(), ThenLines.begin(), ThenLines.end());
	if (Node->getElse() != nullptr) {
		lines.push_back("else:");
		StatementVisitor elseStmtVis(Node->getElse());

		auto elseLines = elseStmtVis.getLines();
		shiftLines(elseLines);
		lines.insert(lines.end(), elseLines.begin(), elseLines.end());
	}
}

void StatementVisitor::VisitWhileStmt(const WhileStmt *Node) {
	lines.push_back("while " + processExpr(Node->getCond()) + ":");
	StatementVisitor body(Node->getBody());
	addLines(lines, shiftLinesRet(body.getLines()));
}

void StatementVisitor::VisitCompoundStmt(const CompoundStmt* Node) {
	if (Node->children().empty()) {
		lines.push_back("# <empty CompoundStmt>");
		return;
	}
	for (auto* s : Node->children()) {
		StatementVisitor v(s);
		auto subLines = v.getLines();
		addLines(lines, subLines);
	}
}

void StatementVisitor::VisitReturnStmt(const ReturnStmt* Node) {
	lines.push_back(std::string("return ") + processExpr(Node->getRetValue()));
}

void StatementVisitor::VisitDeclStmt(const DeclStmt* Node) {
	auto vars = getVarsFromDecl(Node);
	for (const auto&[v, e] : vars) {
		lines.push_back(v + " = " + e);
	}
}

void StatementVisitor::VisitCXXMemberCallExpr(const CXXMemberCallExpr* Node) {
	lines.push_back(processExpr(Node));
}

void StatementVisitor::VisitBinaryOperator(const BinaryOperator* Node) {
	lines.push_back(processExpr(Node));
}

std::optional<std::string> processForStatement(std::map<std::string, std::string>& vars, std::optional<ParsedBinaryExpr>& cond, std::optional<ParsedUnaryExpr>& inc) {
	if (vars.empty() || !cond || !inc) return std::nullopt;

	auto incVar = std::get<0>(*inc);
	auto incOp = std::get<1>(*inc);
	// has simple increment
	if (vars.find(incVar) != end(vars) && (incOp == "++")) {
		auto startValue = vars[incVar];

		// if inc var is compared with '<'
		auto condOp = std::get<1>(*cond);
		bool isGoodCondOp = (condOp == "<") || (condOp == "<=");
		if (std::get<0>(*cond) == incVar && isGoodCondOp) {
			auto endExpr = std::get<2>(*cond);
			if (vars.find(endExpr) != end(vars)) {
				endExpr = vars[endExpr];
			}
			if (condOp == "<=") endExpr = endExpr + " + 1";

			// build 'for' operator
			std::stringstream str;
			str << "for " << incVar << " in range(" << startValue << ", " << endExpr << "):";
			return str.str();
		}
		else {
			return std::nullopt;
		}
	}
	else {
		return std::nullopt;
	}
}

void StatementVisitor::VisitForStmt(const ForStmt * Node) {
	const auto* init = Node->getInit();

	// get variables from init, condition, inc
	auto vars = getVarsFromDecl(dyn_cast<DeclStmt>(init));
	auto cond = getParsedBinaryExpr(Node->getCond());
	auto inc = getParsedUnaryExpr(Node->getInc());

	auto forStmt = processForStatement(vars, cond, inc);
	if (forStmt) {
		lines.push_back(*forStmt);
	}
	else {
		if (init != nullptr) {
			StatementVisitor initV(init);
			addLines(lines, initV.getLines());
		}

		std::stringstream stmt;
		stmt << "while " << processExpr(Node->getCond()) << ":";
		lines.push_back(stmt.str());

		addLines(lines, shiftLinesRet(LinesList{ processExpr(Node->getInc()) }));
	}
	StatementVisitor body(Node->getBody());
	addLines(lines, shiftLinesRet(body.getLines()));
}

void StatementVisitor::VisitCXXForRangeStmt(const CXXForRangeStmt * Node) {
	auto vars = getVarsFromDecl(dyn_cast<DeclStmt>(Node->getLoopVarStmt()));
	auto containers = getVarsFromDecl(dyn_cast<DeclStmt>(Node->getRangeStmt()));

	std::stringstream forStr;
	forStr << "for ";
	size_t idx = 0;
	for (auto& p: vars) {
		forStr << (idx++ > 0 ? ", " : "") << p.first;
	}

	forStr << " in ";

	idx = 0;
	for (auto& p : containers) {
		forStr << (idx++ > 0 ? ", " : "") << p.second;
	}

	forStr << ":";
	lines.push_back(forStr.str());

	StatementVisitor body(Node->getBody());
	addLines(lines, shiftLinesRet(body.getLines()));
}

void StatementVisitor::VisitBreakStmt(const BreakStmt * Node) {
	lines.push_back("break");
}

void StatementVisitor::VisitContinueStmt(const ContinueStmt * Node) {
	lines.push_back("continue");
}

void StatementVisitor::VisitUnaryOperator(const UnaryOperator * Node) {
	lines.push_back(processExpr(Node));
}
