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
			std::stringstream sInit;
			sInit << "self." << init->getMember()->getNameAsString() <<
				" = " << processExpr(init->getInit());
			initLines.push_back(sInit.str());
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
	std::stringstream method;
	method << "def " << M->getNameAsString() << "(self";
	size_t pi = 0;
	for (const auto* p : M->parameters()) {
		method << ", " << p->getNameAsString();
	}
	method << "):";

	
	StatementVisitor body(M->getBody());

	lines.push_back(method.str());
	addLines(lines, shiftLinesRet(body.getLines()));
}

void DeclarationVisitor::_visitRecordDecl(const CXXRecordDecl * R) {
	lines.push_back(std::string("class ") + R->getNameAsString() + ":");

	std::list<std::string> classLines;
	classLines.push_back("# default implementation");
	classLines.push_back("def __init__(self):");

	for (const auto* f : R->fields()) {
		DeclarationVisitor fv(f);
		addLines(classLines, shiftLinesRet(fv.getLines()));
	}

	addLines(lines, shiftLinesRet(classLines));

	for (const auto* m : R->methods()) {
		DeclarationVisitor method(m);
		addLines(lines, shiftLinesRet(method.getLines()));
	}
}

void DeclarationVisitor::_visitClassDecl(const CXXRecordDecl * R) {
}
