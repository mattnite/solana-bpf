cmake_minimum_required(VERSION 2.8.9)
project(clang-solana-bpf)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --bind -s ENVIRONMENT=\"'web,webview,worker,node'\" -s MODULARIZE=1 -s EXPORT_ES6=1 -s USE_ES6_IMPORT_META=0  -s EXPORTED_RUNTIME_METHODS='[\"FS\"]'")
file(COPY solana/sdk/bpf/c/inc/ DESTINATION usr/include/solana)
file(COPY solana/sdk/bpf/c/bpf.ld DESTINATION usr/share)
file(COPY test/logging.c DESTINATION usr/share)
file(COPY /llvm-project/build-wasm/lib/clang/12.0.1/include/ DESTINATION usr/include/clang)
if(EXISTS /bpf-tools/rust/lib/rustlib/bpfel-unknown-unknown/lib/libcompiler_builtins-1cb126d9e305c2b2.rlib)
file(RENAME /bpf-tools/rust/lib/rustlib/bpfel-unknown-unknown/lib/libcompiler_builtins-1cb126d9e305c2b2.rlib /bpf-tools/rust/lib/rustlib/bpfel-unknown-unknown/lib/libcompiler_builtins.rlib)
endif()
file(COPY /bpf-tools/rust/lib/rustlib/bpfel-unknown-unknown/lib/libcompiler_builtins.rlib DESTINATION usr/lib)

link_directories(/llvm-project/build-wasm/lib)
add_executable(clang-solana-bpf src/clang.cpp)

target_include_directories(clang-solana-bpf
        PRIVATE /llvm-project/build-wasm/include
        PRIVATE /llvm-project/build-wasm/tools/clang/include
        PRIVATE /llvm-project/clang/include
        PRIVATE /llvm-project/llvm/include
        PRIVATE /llvm-project/lld/include
        PRIVATE usr/include)

target_compile_options(clang-solana-bpf PRIVATE
        -stdlib=libc++
        -std=c++2a
        -fcolor-diagnostics
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter)

target_link_libraries(clang-solana-bpf PRIVATE 
        LLVMAggressiveInstCombine
        LLVMAnalysis
        LLVMAsmParser
        LLVMAsmPrinter
        LLVMBPFAsmParser
        LLVMBPFCodeGen
        LLVMBPFDesc
        LLVMBPFDisassembler
        LLVMBPFInfo
        LLVMBinaryFormat
        LLVMBitReader
        LLVMBitWriter
        LLVMBitstreamReader
        LLVMCFGuard
        LLVMCFIVerify
        LLVMCodeGen
        LLVMCore
        LLVMCoroutines
        LLVMCoverage
        LLVMDWARFLinker
        LLVMDebugInfoCodeView
        LLVMDebugInfoDWARF
        LLVMDebugInfoGSYM
        LLVMDebugInfoMSF
        LLVMDebugInfoPDB
        LLVMDemangle
        LLVMDlltoolDriver
        LLVMExecutionEngine
        LLVMExegesis
        LLVMExtensions
        LLVMFileCheck
        LLVMFrontendOpenACC
        LLVMFrontendOpenMP
        LLVMFuzzMutate
        LLVMGlobalISel
        LLVMHelloNew
        LLVMIRReader
        LLVMInstCombine
        LLVMInstrumentation
        LLVMInterfaceStub
        LLVMInterpreter
        LLVMJITLink
        LLVMLTO
        LLVMLibDriver
        LLVMLineEditor
        LLVMLinker
        LLVMMC
        LLVMMCA
        LLVMMCDisassembler
        LLVMMCJIT
        LLVMMCParser
        LLVMMIRParser
        LLVMObjCARCOpts
        LLVMObject
        LLVMObjectYAML
        LLVMOption
        LLVMOrcJIT
        LLVMOrcShared
        LLVMOrcTargetProcess
        LLVMPasses
        LLVMProfileData
        LLVMRemarks
        LLVMRuntimeDyld
        LLVMScalarOpts
        LLVMSelectionDAG
        LLVMSupport
        LLVMSymbolize
        LLVMTableGen
        LLVMTableGenGlobalISel
        LLVMTarget
        LLVMTextAPI
        LLVMTransformUtils
        LLVMVectorize
        LLVMWebAssemblyAsmParser
        LLVMWebAssemblyCodeGen
        LLVMWebAssemblyDesc
        LLVMWebAssemblyDisassembler
        LLVMWebAssemblyInfo
        LLVMWindowsManifest
        LLVMXRay
        LLVMipo
        LTO
        Remarks
        clang-cpp
        clang
        clangAPINotes
        clangARCMigrate
        clangAST
        clangASTMatchers
        clangAnalysis
        clangBasic
        clangCodeGen
        clangCrossTU
        clangDependencyScanning
        clangDirectoryWatcher
        clangDriver
        clangDynamicASTMatchers
        clangEdit
        clangFormat
        clangFrontend
        clangFrontendTool
        clangHandleCXX
        clangHandleLLVM
        clangIndex
        clangIndexSerialization
        clangLex
        clangParse
        clangRewrite
        clangRewriteFrontend
        clangSema
        clangSerialization
        clangStaticAnalyzerCheckers
        clangStaticAnalyzerCore
        clangStaticAnalyzerFrontend
        clangTesting
        clangTooling
        clangToolingASTDiff
        clangToolingCore
        clangToolingInclusions
        clangToolingRefactoring
        clangToolingSyntax
        clangTransformer
        lldCOFF
        lldCommon
        lldCore
        lldDriver
        lldELF
        lldMachO
        lldMachO2
        lldMinGW
        lldReaderWriter
        lldWasm
        lldYAML
        -stdlib=libc++)
