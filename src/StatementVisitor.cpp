#include "StatementVisitor.h"
#include "ExpressionProcessor.h"
#include "Lines.h"

StatementVisitor::StatementVisitor(const Stmt *Node) {
	Visit(Node);
}

std::list<std::string> StatementVisitor::getLines() const { 
	return lines; 
}

void StatementVisitor::Visit(const Stmt *Node) {
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
	lines.push_back("while something:");
}

void StatementVisitor::VisitCompoundStmt(const CompoundStmt* Node) {
	for (auto* s : Node->children()) {
		StatementVisitor v(s);
		auto subLines = v.getLines();
		lines.insert(lines.end(), subLines.begin(), subLines.end());
	}
}

void StatementVisitor::VisitReturnStmt(const ReturnStmt* Node) {
	lines.push_back(std::string("return ") + processExpr(Node->getRetValue()));
}

void StatementVisitor::VisitDeclStmt(const DeclStmt* Node) {
	for (const auto* d : Node->getDeclGroup()) {
		const auto* vd = dyn_cast<VarDecl>(d);
		if (vd != nullptr) {
			lines.push_back(vd->getNameAsString() + " = " + processExpr(vd->getInit()));
		}
	}
}

void StatementVisitor::VisitCXXMemberCallExpr(const CXXMemberCallExpr* Node) {
	lines.push_back(processExpr(Node));
}

void StatementVisitor::VisitBinaryOperator(const BinaryOperator* Node) {
	lines.push_back(processExpr(Node));
}