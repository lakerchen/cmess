#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <vector>

#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FileSystem.h"

#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/Lexer.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Rewrite/Frontend/Rewriters.h"
#include "clang/Rewrite/Core/Rewriter.h"

using namespace std;
using namespace clang;

class CMessRecursiveASTVisitor : public RecursiveASTVisitor<CMessRecursiveASTVisitor>
{
	public :
		CMessRecursiveASTVisitor(Rewriter &r) : rewrite(r) { }
		Expr *VisitBinaryOperator(BinaryOperator *BO);
		Rewriter rewrite;
};

Expr *CMessRecursiveASTVisitor::VisitBinaryOperator(BinaryOperator *BO)
{
	//llvm:outs() << BO->getOpcodeStr() << "\n";
	 if (BO->isLogicalOp()){
		 rewrite.InsertText(BO->getLHS()->getExprLoc(),BO->getOpcode() == BO_LAnd ? "AND(" : "OR(", true);
		 rewrite.ReplaceText(BO->getOperatorLoc(), BO->getOpcodeStr().size(), ",");
		 rewrite.InsertTextAfterToken(BO->getRHS()->getLocEnd(), ")");
	 }
	return BO;
}

class CMessASTConsumer : public ASTConsumer
{
	CMessASTConsumer(Rewriter &R) : crv(R) { }
	virtual bool HandleTopLevelDecl(DeclGroupRef d);
	CMessRecursiveASTVisitor crv;
};

bool CMessASTConsumer::HandleTopLevelDecl(DeclGroupRef d)
{
	  typedef DeclGroupRef::iterator iter;

	  for (iter b = d.begin(), e = d.end(); b != e; ++b){
		  crv.TraverseDecl(*b);
	  }
	  return true; // keep going
}


int main(int argc , char **argv)
{
	struct stat sb;
	if(argc < 2){
		llvm::errs() << "The right usage is ./bin/cmess <options> <filename>" << "\n";
		return 1;
	}
	
	vector<char> vc;
	for(unsigned int i = 0; i < strlen(argv[1]); i++ ){
		if(argv[1][i] != '-'){
			vc.push_back(argv[1][i]);
		}
	}

	for(unsigned int i = 0; i < vc.size(); i++){
		switch(vc[i]){
			case 'a' : {
			//	llvm::outs() << "a opt" << "\n";
				break;
			}
			case 'b' : {
				break;
			}
			case 's' : {
				break;
			}
			default : break;
		}
	}


	std::string fileName(argv[argc - 1]);
	
	if (stat(fileName.c_str(), &sb) == -1) {
		perror(fileName.c_str());
		exit(EXIT_FAILURE);
	}
	CompilerInstance compiler;
	DiagnosticOptions diagnosticOptions;
	compiler.createDiagnostics();

	CompilerInvocation *Invocation = new CompilerInvocation;
	CompilerInvocation::CreateFromArgs(*Invocation, argv + 1, argv + argc,
	compiler.getDiagnostics());
	compiler.setInvocation(Invocation);

	llvm::IntrusiveRefCntPtr<TargetOptions> pto( new TargetOptions());
	pto->Triple = llvm::sys::getDefaultTargetTriple();
	llvm::IntrusiveRefCntPtr<TargetInfo>
	pti(TargetInfo::CreateTargetInfo(compiler.getDiagnostics(),pto.getPtr()));
	compiler.setTarget(pti.getPtr());

	compiler.createFileManager();
	compiler.createSourceManager(compiler.getFileManager());

	HeaderSearchOptions &headerSearchOptions = compiler.getHeaderSearchOpts();
	
	/*@TUDO why  headerSearchOptions.AddPath() is needed here*/

	LangOptions langOpts;
	langOpts.GNUMode = 1; 
	langOpts.CXXExceptions = 1; 
	langOpts.RTTI = 1; 
	langOpts.Bool = 1; 
	langOpts.CPlusPlus = 1; 
	Invocation->setLangDefaults(langOpts,clang::IK_CXX,clang::LangStandard::lang_cxx0x);
	compiler.createPreprocessor(clang::TU_Complete);
	compiler.getPreprocessorOpts().UsePredefines = false;
	compiler.createASTContext();

	Rewriter Rewrite;
	Rewrite.setSourceMgr(compiler.getSourceManager(), compiler.getLangOpts());

	const FileEntry *pFile = compiler.getFileManager().getFile(fileName);
	compiler.getSourceManager().createMainFileID(pFile);
	compiler.getDiagnosticClient().BeginSourceFile(compiler.getLangOpts(),&compiler.getPreprocessor());
	CMessASTConsumer astConsumer(Rewrite);

	std::string outName (fileName);
	size_t ext = outName.rfind(".");
	if (ext == std::string::npos)
		ext = outName.length();
	outName.insert(ext, ".mess");

	llvm::errs() << "Output to: " << outName << "\n";
	std::string OutErrorInfo;
	llvm::raw_fd_ostream outFile(outName.c_str(), OutErrorInfo, llvm::sys::fs::F_None);

	if (OutErrorInfo.empty(){
		ParseAST(compiler.getPreprocessor(), &astConsumer, compiler.getASTContext());
		compiler.getDiagnosticClient().EndSourceFile();
												     
		outFile << "#define AND(a, b) a && b\n";
		outFile << "#define OR(a, b) a || b\n\n";
		const RewriteBuffer *RewriteBuf =
		Rewrite.getRewriteBufferFor(compiler.getSourceManager().getMainFileID());
		outFile << std::string(RewriteBuf->begin(), RewriteBuf->end());
	}else{
		llvm::errs() << "Cannot open " << outName << " for writing\n";
	}

	outFile.close();
	return 0;
}











