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
#include "clang/FrontendTool/Utils.h"
#include "clang/Lex/PreprocessorOptions.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Process.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/StringSaver.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/ADT/ArrayRef.h"

using namespace clang;

static void LLVMErrorHandler(void *UserData, const std::string &Message, bool GenCrashDiag)
{
    DiagnosticsEngine &Diags = *static_cast<DiagnosticsEngine *>(UserData);
    Diags.Report(diag::err_fe_error_backend) << Message;
    llvm::sys::RunInterruptHandlers();
    llvm::sys::Process::Exit(GenCrashDiag ? 70 : 1);
}

extern "C" bool compile(ArrayRef<const char *> Argv, const char *Argv0)
{
    std::unique_ptr<CompilerInstance> Clang(new CompilerInstance());
    IntrusiveRefCntPtr<DiagnosticIDs> DiagID(new DiagnosticIDs());

    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmPrinters();
    llvm::InitializeAllAsmParsers();

    IntrusiveRefCntPtr<DiagnosticOptions> DiagOpts = new DiagnosticOptions();
    TextDiagnosticBuffer *DiagsBuffer = new TextDiagnosticBuffer;
    DiagnosticsEngine Diags(DiagID, &*DiagOpts, DiagsBuffer);

    bool Success = CompilerInvocation::CreateFromArgs(Clang->getInvocation(), Argv, Diags, Argv0);

    Clang->createDiagnostics();
    if (!Clang->hasDiagnostics())
        return false;

    llvm::install_fatal_error_handler(LLVMErrorHandler,
                                      static_cast<void *>(&Clang->getDiagnostics()));

    for (auto &input : Clang->getFrontendOpts().Inputs)
    {
        std::cout << "input: '" << input.getFile().data() << "'" << std::endl;
    }

    std::cout << "output: '" << Clang->getFrontendOpts().OutputFile.data() << "'" << std::endl;

    DiagsBuffer->FlushDiagnostics(Clang->getDiagnostics());
    if (!Success)
        return false;

    Success = ExecuteCompilerInvocation(Clang.get());
    if (!Success)
        return false;

    llvm::remove_fatal_error_handler();

    return Success;
}

int main(int argc_, const char *argv_[])
{
    SmallVector<const char *, 256> argv(argv_, argv_ + argc_);

    llvm::InitLLVM X(argc_, argv_);
    if (llvm::sys::Process::FixupStandardFileDescriptors())
        return 1;

    llvm::BumpPtrAllocator A;
    llvm::StringSaver Saver(A);
    llvm::cl::ExpandResponseFiles(Saver, &llvm::cl::TokenizeGNUCommandLine, argv, /*MarkEOLs=*/false);

    for (auto &arg : argv)
    {
        std::cout << "arg: '" << arg << "'" << std::endl;
    }

    return !compile(makeArrayRef(argv).slice(1), argv[0]);
}
