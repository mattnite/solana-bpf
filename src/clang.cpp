// Copyright 2017-2018 Todd Fleming
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#include <stdio.h>
#include <iostream>

#include "clang/CodeGen/CodeGenAction.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendDiagnostic.h"
#include "clang/Frontend/TextDiagnosticBuffer.h"
#include "clang/Lex/PreprocessorOptions.h"
#include "llvm/Support/TargetSelect.h"

using namespace clang;

static auto triple = "bpfel-unknown-unknown-bpfel+solana";
static auto host_triple = "wasm32-unknown-unknown-wasm";

extern "C" bool compile(
        const char* inputFilename,
        const char* outputFilename
) {
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmPrinters();
    llvm::InitializeAllAsmParsers();

    auto compiler = std::make_unique<CompilerInstance>();
    compiler->createDiagnostics();

    std::vector<std::string> includes{};
    CompilerInvocation::setLangDefaults(
        compiler->getLangOpts(),
        InputKind(Language::C),
        llvm::Triple{triple},
        includes,
        LangStandard::Kind::lang_c17);

    compiler->getFrontendOpts().Inputs.push_back(
            FrontendInputFile{inputFilename, InputKind(Language::C)});
    compiler->getFrontendOpts().OutputFile = outputFilename;

    auto& sOpts = compiler->getHeaderSearchOpts();
    sOpts.UseBuiltinIncludes = false;
    sOpts.UseStandardSystemIncludes = false;

    sOpts.AddPath("/usr/include/clang", frontend::System, false, true);
    sOpts.AddPath("/usr/include/solana", frontend::System, false, true);

    compiler->getCodeGenOpts().CodeModel = "default";
    //compiler->getCodeGenOpts().RelocationModel = "static";
    //compiler->getCodeGenOpts().ThreadModel = "single";
    compiler->getCodeGenOpts().OptimizationLevel = 2; // -Os
    compiler->getCodeGenOpts().OptimizeSize = 1;      // -Os
    compiler->getCodeGenOpts().setDebugInfo(codegenoptions::DebugInfoKind::FullDebugInfo);

    // Linker options:
    //compiler->getCodeGenOpts().LinkerOptions.push_back("-z");
    //compiler->getCodeGenOpts().LinkerOptions.push_back("notext");
    //compiler->getCodeGenOpts().LinkerOptions.push_back("-shared");
    //compiler->getCodeGenOpts().LinkerOptions.push_back("--Bdynamic");
    //compiler->getCodeGenOpts().LinkerOptions.push_back("/usr/share/bpf.ld");
    //compiler->getCodeGenOpts().LinkerOptions.push_back("--entry");
    //compiler->getCodeGenOpts().LinkerOptions.push_back("entrypoint");

    compiler->getLangOpts().Optimize = 1;
    compiler->getLangOpts().OptimizeSize = 1;

    compiler->getTargetOpts().Triple = triple;
    compiler->getTargetOpts().HostTriple = host_triple;

    auto act = std::make_unique<EmitObjAction>();
    return compiler->ExecuteAction(*act);
}

int main(int argc, const char* argv[]) {
    if (argc == 3)
        return !compile(argv[1], argv[2]);
    if (argc > 1) {
        fprintf(stderr, "Usage: input_file.cpp output_file.wasm\n");
        return 1;
    }
    return 0;
}
