#include "DeclarationVisitor.h"
#include "StatementVisitor.h"
#include "ExpressionProcessor.h"
#include "Lines.h"

#include "clang/AST/Type.h"

DeclarationVisitor::DeclarationVisitor(const Decl* Node) {
	Visit(Node);
}

std::list<std::string> DeclarationVisitor::getLines() const { 
	return lines; 
}

void DeclarationVisitor::Visit(const Decl *Node) {
	if (Node == nullptr) {
		lines.push_back("<empty declaration>");
		return;
	}

	// new node = new lines
	lines.clear();

	ConstDeclVisitor<DeclarationVisitor>::Visit(Node);
	// no processed lines for this node
	if (lines.empty()) {
		lines.push_back(std::string("# cannot processing declaration: ") + Node->getDeclKindName());

		Node->dumpColor();
	}
}

void DeclarationVisitor::VisitFunctionDecl(const FunctionDecl* F) {
	std::stringstream str;
	str << "def " << F->getNameAsString() << "(";

	size_t i = 0;
	for (auto* p : F->parameters()) {
		str << (i > 0 ? ", " : "") << p->getNameAsString();
		i++;
	}
	str << "):";
	lines.push_back(str.str());

	lines.push_back(std::string("# Body statement type: ") + F->getBody()->getStmtClassName());

	StatementVisitor visitor(F->getBody());
	auto bodyLines = visitor.getLines();
	shiftLines(bodyLines);
	lines.insert(lines.end(), bodyLines.begin(), bodyLines.end());
}

void DeclarationVisitor::VisitCXXRecordDecl(const CXXRecordDecl* R) {
	if (R->isStruct()) {
		_visitRecordDecl(R);
	}
	else if (R->isClass()) {
		_visitRecordDecl(R);
	}
	else {
		lines.push_back(std::string("# unsupported cxxstruct ") + R->getNameAsString());
	}
}

void DeclarationVisitor::VisitEnumDecl(const EnumDecl * D) {
	size_t idx = 0;
	for (const auto* i : D->enumerators()) {
		std::stringstream str;
		str << i->getNameAsString() << " = ";
		if (const auto* expr = i->getInitExpr(); expr != nullptr) {
			str << processExpr(expr);
		}
		else {
			str << idx++;
		}
		lines.push_back(str.str());
	}
}

void DeclarationVisitor::VisitFieldDecl(const FieldDecl* F) {
	std::stringstream str;
	str << "self." << F->getNameAsString();
	if (F->hasInClassInitializer()) {
		str << " = " << processExpr(F->getInClassInitializer());
	}
	else {
		str << " = None";
	}
	
	Qualifiers qf;
	LangOptions lo;
	PrintingPolicy pp(lo);
	auto t = F->getType();

	lines.push_back("# field type: " + QualType::getAsString(t.getTypePtr(), qf, pp));
	lines.push_back(std::string("# access: ") + (F->getAccess() == AS_public ? "public" : "non-public"));
	auto access = F->getAccess();
	lines.push_back(str.str());
}

void DeclarationVisitor::VisitCXXConstructorDecl(const CXXConstructorDecl* C) {
	if (C->getBody() != nullptr) {
		std::stringstream head;
		head << "def __init__(self";
		for (const auto* p : C->parameters()) {
			head << ", " << p->getNameAsString();
		}
		head << "):";
		lines.push_back("# user constructor");
		lines.push_back(head.str());

		std::list<std::string> initLines;
		for (const auto* init : C->inits()) {
			if (init->getMember() != nullptr) {
				std::stringstream sInit;
				sInit << "self." << init->getMember()->getNameAsString() <<
					" = " << processExpr(init->getInit());
				initLines.push_back(sInit.str());
			}
		}
		addLines(lines, shiftLinesRet(initLines));

		StatementVisitor body(C->getBody());
		addLines(lines, shiftLinesRet(body.getLines()));
	}
	else {
		lines.push_back("# ignore default constructor");
	}
}

void DeclarationVisitor::VisitCXXMethodDecl(const CXXMethodDecl* M) {
	if (!M->isCanonicalDecl()) {
		lines.push_back("# external declaration of " + M->getQualifiedNameAsString());
		return;
	}

	std::stringstream comment;
	//comment << "# isDefined: " << M->isDefined();
	//comment << " isCanonicalDecl: " << M->isCanonicalDecl();
	//comment << " hasSkippedBody: " << M->hasSkippedBody();
	//comment << " willHaveBody: " << M->willHaveBody();

	std::stringstream method;
	method << "def " << M->getNameAsString() << "(self";
	size_t pi = 0;
	for (const auto* p : M->parameters()) {
		method << ", " << p->getNameAsString();
	}
	method << "):";

	
	lines.push_back(comment.str());
	lines.push_back(method.str());
	if (M->isPure()) {
		addLines(lines, shiftLinesRet(std::list<std::string>{ "None" }));
	}
	else {
		StatementVisitor body(M->getBody());
		addLines(lines, shiftLinesRet(body.getLines()));
	}
}

void DeclarationVisitor::VisitVarDecl(const VarDecl * D)
{
	lines.push_back(D->getNameAsString() + " = " + processExpr(D->getInit()));
}

void DeclarationVisitor::_visitRecordDecl(const CXXRecordDecl * R) {
	if (!R->isThisDeclarationADefinition()) {
		lines.push_back("# forward declaration of " + R->getNameAsString());
		return;
	}

	std::stringstream head;
	head << "class " << R->getNameAsString();
	if (R->getNumBases() > 0) {
		size_t idx = 0;
		head << "(";
		for (auto b : R->bases()) {
			head << (idx++ > 0 ? ", " : "") << b.getType()->getAsCXXRecordDecl()->getNameAsString();
		}
		head << "):";
	}
	else {
		head << ":";
	}
	lines.push_back(head.str());

	std::list<std::string> classLines;
	classLines.push_back("# default implementation");
	classLines.push_back("def __init__(self):");

	for (const auto* f : R->fields()) {
		DeclarationVisitor fv(f);
		addLines(classLines, shiftLinesRet(fv.getLines()));
	}

	addLines(lines, shiftLinesRet(classLines));

	bool isAllBodies = true;
	for (const auto* m : R->methods()) {
		// is this a method, not constructor
		bool isMethod = !isa<CXXConstructorDecl>(m) && !isa<CXXDestructorDecl>(m)
			&& !m->isCopyAssignmentOperator() && !m->isMoveAssignmentOperator() && !m->isDestroyingOperatorDelete();

		// add constructor with body
		isMethod |= (m->getBody() != nullptr);

		if (isMethod) {
			DeclarationVisitor method(m);
			addLines(lines, shiftLinesRet(method.getLines()));
		}
		else {
			addLines(lines, shiftLinesRet(LinesList{ "# skip " + m->getQualifiedNameAsString() }));
		}

		// do not check of pure virtual methods
		if (isMethod && !m->isPure()) {
			isAllBodies &= (m->getBody() != nullptr);
		}
	}

	if (!isAllBodies) {
		lines.clear();
		lines.push_back("# skipped declaration of " + R->getQualifiedNameAsString());
	}
}

void DeclarationVisitor::_visitClassDecl(const CXXRecordDecl * R) {
}
