#include "clang/AST/DeclVisitor.h"
#include <sstream>

using namespace clang;

class DeclarationVisitor : public ConstDeclVisitor<DeclarationVisitor> {
public:
	DeclarationVisitor(const Decl* Node);
	std::list<std::string> getLines() const;

	void Visit(const Decl *Node);
	void VisitFunctionDecl(const FunctionDecl* F);
	void VisitCXXRecordDecl(const CXXRecordDecl* R);
	void VisitEnumDecl(const EnumDecl* D);
	void VisitFieldDecl(const FieldDecl* F);
	void VisitCXXConstructorDecl(const CXXConstructorDecl* C);
	void VisitCXXMethodDecl(const CXXMethodDecl* M);

private:
	std::list<std::string> lines;

	void _visitRecordDecl(const CXXRecordDecl* R);
	void _visitClassDecl(const CXXRecordDecl* R);
};