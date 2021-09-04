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

#include "clang/CodeGen/CodeGenAction.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendDiagnostic.h"
#include "clang/Frontend/TextDiagnosticBuffer.h"
#include "clang/Lex/PreprocessorOptions.h"
#include "llvm/Support/TargetSelect.h"

#include "wasm-tools.h"

using namespace llvm;
using namespace clang;
using namespace std::literals;

#define STRX(s) STR(s)
#define STR(s) #s

template <typename T, typename... A> IntrusiveRefCntPtr<T> make_intr(A&&... a) {
    return {new T{std::forward<A>(a)...}};
}

/*
 * 
C_FLAGS := \
  -Werror \
  -O2 \
  -fno-builtin \
  -std=c17 \
  $(addprefix -isystem,$(SYSTEM_INC_DIRS)) \
  $(addprefix -I,$(INC_DIRS))

  BPF_C_FLAGS := \
  $(C_FLAGS) \
  -target bpf \
  -fPIC \
  -march=bpfel+solana

  BPF_LLD_FLAGS := \
  -z notext \
  -shared \
  --Bdynamic \
  $(LOCAL_PATH)bpf.ld \
  --entry entrypoint \
 */

static auto triple = "wasm32-unknown-unknown-bpf+solana";
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
        Triple{triple},
        includes,
        LangStandard::Kind::lang_c17);

    compiler->getFrontendOpts().Inputs.push_back(
            FrontendInputFile{inputFilename, InputKind(Language::C)});
    compiler->getFrontendOpts().OutputFile = outputFilename;

    auto& sOpts = compiler->getHeaderSearchOpts();
    sOpts.UseBuiltinIncludes = false;
    sOpts.UseStandardSystemIncludes = false;

    sOpts.AddPath("/usr/include", frontend::System, false, true);

    //compiler->getPreprocessorOpts().addMacroDef("__EMSCRIPTEN__");

    compiler->getCodeGenOpts().CodeModel = "default";
    //compiler->getCodeGenOpts().RelocationModel = "static";
    //compiler->getCodeGenOpts().ThreadModel = "single";
    compiler->getCodeGenOpts().OptimizationLevel = 2; // -Os
    compiler->getCodeGenOpts().OptimizeSize = 1;      // -Os
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
