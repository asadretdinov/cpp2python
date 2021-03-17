#include <clang-c/Index.h>
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"

#include "StatementVisitor.h"
#include "DeclarationVisitor.h"
#include "Lines.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <optional>

using namespace clang;

class TranslationUnitVisitor
	: public RecursiveASTVisitor<TranslationUnitVisitor> {
public:
	explicit TranslationUnitVisitor(ASTContext *Context)
		: Context(Context) {}

	bool VisitDecl(Decl* D) {
		if (D->getKind() == Decl::Kind::TranslationUnit) {
			llvm::outs() << "Transation unit found\n";
			auto* unit = D->getTranslationUnitDecl();
			if (!unit) return true;

			for (auto* d : unit->decls()) {
				std::optional<std::string> position;
				FullSourceLoc FullLocation = Context->getFullLoc(d->getBeginLoc());
				if (FullLocation.isValid() && !FullLocation.isInSystemHeader()) {
					std::stringstream str;
					str << FullLocation.getSpellingLineNumber() << ":"
					<< FullLocation.getSpellingColumnNumber();
					position = str.str();
				}
				else {
					continue;
				}

				llvm::outs() << "# Declated something at " << *position << " with type " << d->getDeclKindName() << "\n";
				DeclarationVisitor v(d);
				printLines(v.getLines());
				std::cout << std::endl;
			}
		}
		return true;
	}
private:
	ASTContext *Context;
};

class TranslationUnitConsumer : public clang::ASTConsumer {
public:
	explicit TranslationUnitConsumer(ASTContext *Context)
		: Visitor(Context) {}

	virtual void HandleTranslationUnit(clang::ASTContext &Context) {
		Visitor.TraverseDecl(Context.getTranslationUnitDecl());
	}
private:
	TranslationUnitVisitor Visitor;
};

class TranslationUnitAction : public clang::ASTFrontendAction {
public:
	virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &Compiler, llvm::StringRef InFile) {
		return std::make_unique<TranslationUnitConsumer>(&Compiler.getASTContext());
	}
};

int main(int argc, char **argv) {
	if (argc > 1) {
		std::stringstream str;
		std::ifstream fin(argv[1]);
		std::string s;
		while (!fin.eof()) {
			std::getline(fin, s);
			str << s << "\n";
		}
		clang::tooling::runToolOnCode(std::make_unique<TranslationUnitAction>(), str.str(), argv[1]);
	}
	else {
		llvm::outs() << "need to set file name";
	}
}