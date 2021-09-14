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
#include <emscripten.h>
#include <emscripten/bind.h>
#include <exception>

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
#include "lld/Common/Driver.h"

struct LinkResult {
    bool result;
    std::string out;
    std::string err;
};

static void LLVMErrorHandler(void *UserData, const std::string &Message, bool GenCrashDiag)
{
    clang::DiagnosticsEngine &Diags = *static_cast<clang::DiagnosticsEngine *>(UserData);
    Diags.Report(clang::diag::err_fe_error_backend) << Message;
    llvm::sys::RunInterruptHandlers();
    llvm::sys::Process::Exit(GenCrashDiag ? 70 : 1);
}

struct Clang {
    Clang() {
        llvm::InitializeAllTargets();
        llvm::InitializeAllTargetMCs();
        llvm::InitializeAllAsmPrinters();
        llvm::InitializeAllAsmParsers();

        if (llvm::sys::Process::FixupStandardFileDescriptors())
            throw std::runtime_error("failed to fixup std file descriptors");
    }

    bool compile(const std::vector<std::string>& args) {
        std::vector<std::string> args_copy;
        for (auto &arg : args) {
            std::string str = arg;
            args_copy.push_back(str);
            emscripten_log(EM_LOG_DEBUG, "arg: %s", arg.c_str());
        }

        auto c_args = std::vector<const char *>();
        for (auto &str : args_copy)
            c_args.push_back(str.c_str());

        std::unique_ptr<clang::CompilerInstance> compiler(new clang::CompilerInstance());
        clang::IntrusiveRefCntPtr<clang::DiagnosticIDs> diag_id(new clang::DiagnosticIDs());
        clang::IntrusiveRefCntPtr<clang::DiagnosticOptions> diag_opts = new clang::DiagnosticOptions();
        clang::TextDiagnosticBuffer *diags_buffer = new clang::TextDiagnosticBuffer;
        clang::DiagnosticsEngine Diags(diag_id, &*diag_opts, diags_buffer);

        bool success = clang::CompilerInvocation::CreateFromArgs(compiler->getInvocation(), llvm::makeArrayRef(c_args), Diags);
        compiler->createDiagnostics();
        if (!compiler->hasDiagnostics())
            return false;

        llvm::install_fatal_error_handler(LLVMErrorHandler, static_cast<void *>(&compiler->getDiagnostics()));
        for (auto &input : compiler->getFrontendOpts().Inputs)
            emscripten_log(EM_LOG_DEBUG, "input: '%s'", input.getFile().data());

        emscripten_log(EM_LOG_DEBUG, "output: '%s'", compiler->getFrontendOpts().OutputFile.data());
        diags_buffer->FlushDiagnostics(compiler->getDiagnostics());
        if (!success)
            return false;

        if (!ExecuteCompilerInvocation(compiler.get()))
            return false;

        llvm::remove_fatal_error_handler();
        return true;
    }

    LinkResult link_bpf(const std::vector<std::string>& args) {
        std::vector<std::string> args_copy;
        for (auto &arg : args) {
            std::string str = arg;
            args_copy.push_back(str);
            emscripten_log(EM_LOG_DEBUG, "arg: %s", arg.c_str());
        }

        auto c_args = std::vector<const char *>();
        for (auto &str : args_copy)
            c_args.push_back(str.c_str());

        std::string out, err;
        llvm::raw_string_ostream stdout_stream(out);
        llvm::raw_string_ostream stderr_stream(err);

        return LinkResult{
            lld::elf::link(llvm::makeArrayRef(c_args), false, stdout_stream, stderr_stream),
            out,
            err,
        };
    }

    LinkResult link_wasm(const std::vector<std::string>& args) {
        std::vector<std::string> args_copy;
        for (auto &arg : args) {
            std::string str = arg;
            args_copy.push_back(str);
            emscripten_log(EM_LOG_DEBUG, "arg: %s", arg.c_str());
        }

        auto c_args = std::vector<const char *>();
        for (auto &str : args_copy)
            c_args.push_back(str.c_str());

        std::string out, err;
        llvm::raw_string_ostream stdout_stream(out);
        llvm::raw_string_ostream stderr_stream(err);

        return LinkResult{
            lld::wasm::link(llvm::makeArrayRef(c_args), false, stdout_stream, stderr_stream),
            out,
            err,
        };
    }
};


EMSCRIPTEN_BINDINGS(Clang) {
    emscripten::register_vector<std::string>("StringList");
    emscripten::class_<Clang>("Clang")
        .constructor<>()
        .function("compile", &Clang::compile)
        .function("linkBpf", &Clang::link_bpf)
        .function("linkWasm", &Clang::link_wasm)
    ;
}
