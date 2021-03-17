#include "clang/AST/StmtVisitor.h"
#include <sstream>

using namespace clang;

class StatementVisitor : public ConstStmtVisitor<StatementVisitor> {
public:
	StatementVisitor(const Stmt *Node);

	std::list<std::string> getLines() const;

	void Visit(const Stmt *Node);
	void VisitIfStmt(const IfStmt *Node);
	void VisitWhileStmt(const WhileStmt *Node);
	void VisitCompoundStmt(const CompoundStmt* Node);
	void VisitReturnStmt(const ReturnStmt* Node);
	void VisitDeclStmt(const DeclStmt* Node);
	void VisitCXXMemberCallExpr(const CXXMemberCallExpr* Node);
	void VisitBinaryOperator(const BinaryOperator* Node);
private:
	std::list<std::string> lines;
};
