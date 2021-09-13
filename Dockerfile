FROM ubuntu:20.04

# Install compiler, python and subversion.
RUN apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y \
                    git \
                    build-essential \
                    gcc \
                    g++ \
                    python \
                    python3 \
                    cmake \
                    ninja-build \
                    wget

RUN git clone --depth 1 --single-branch --branch bpf-tools-v1.15 https://github.com/solana-labs/llvm-project.git
RUN mkdir -p /bpf-tools && wget -qO- https://github.com/solana-labs/bpf-tools/releases/download/v1.15/solana-bpf-tools-linux.tar.bz2 | tar -xj -C bpf-tools
RUN git clone --depth 1 --single-branch --branch v1.7.11 https://github.com/solana-labs/solana.git
RUN git clone --depth 1 --single-branch --branch main https://github.com/emscripten-core/emsdk.git && cd emsdk && ./emsdk install 2.0.29 && ./emsdk activate latest

RUN cd llvm-project && mkdir build-host && cd build-host && \
    cmake -G "Ninja" \
        -DCMAKE_BUILD_TYPE=Release \
        -DLLVM_ENABLE_PROJECTS="lld;clang" \
        -DLLVM_ENABLE_DUMP=OFF \
        -DLLVM_ENABLE_ASSERTIONS=OFF \
        -DLLVM_ENABLE_EXPENSIVE_CHECKS=OFF \
        -DLLVM_ENABLE_LIBXML2=OFF \
        -DLLVM_ENABLE_ZLIB=OFF \
        -DLLVM_ENABLE_BACKTRACES=OFF \
        -DLLVM_TARGETS_TO_BUILD="X86;WebAssembly" \
        -DLLVM_BUILD_TOOLS=OFF \
        -DLLVM_ENABLE_THREADS=OFF \
        -DLLVM_BUILD_LLVM_DYLIB=OFF \
        -DLLVM_INCLUDE_TESTS=OFF \
        -DLLVM_INCLUDE_EXAMPLES=OFF \
        -DLLVM_INCLUDE_BENCHMARKS=OFF \
        -DLLVM_ENABLE_BINDINGS=OFF \
        -DLLVM_ENABLE_OCAMLDOC=OFF \
        -DLLVM_ENABLE_Z3_SOLVER=OFF \
        -DLLVM_ENABLE_TERMINFO=OFF \
        ../llvm && ninja

RUN mkdir -p /llvm-project/build-wasm/usr/share
RUN mkdir -p /llvm-project/build-wasm/usr/lib
RUN cp /solana/sdk/bpf/c/bpf.ld /llvm-project/build-wasm/usr/share
RUN cp /bpf-tools/rust/lib/rustlib/bpfel-unknown-unknown/lib/libcompiler_builtins-1cb126d9e305c2b2.rlib /llvm-project/build-wasm/usr/lib/libcompiler_builtins.rlib

RUN cd emsdk && . ./emsdk_env.sh && cd /llvm-project/build-wasm && \
    emcmake cmake -G "Ninja" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_EXE_LINKER_FLAGS="-s ASSERTIONS=1 -s ENVIRONMENT='web,webview,worker,node' -s MODULARIZE -s EXPORT_ES6=1 -s EXPORTED_RUNTIME_METHODS='[\"FS\",\"callMain\",\"PROXYFS\"]' -lproxyfs.js" \
        -DLLVM_ENABLE_PROJECTS="lld;clang" \
        -DLLVM_ENABLE_DUMP=OFF \
        -DLLVM_ENABLE_ASSERTIONS=OFF \
        -DLLVM_ENABLE_EXPENSIVE_CHECKS=OFF \
        -DLLVM_ENABLE_LIBXML2=OFF \
        -DLLVM_ENABLE_ZLIB=OFF \
        -DLLVM_ENABLE_BACKTRACES=OFF \
        -DLLVM_TARGETS_TO_BUILD="WebAssembly;BPF" \
        -DLLVM_BUILD_TOOLS=OFF \
        -DLLVM_ENABLE_THREADS=OFF \
        -DLLVM_BUILD_LLVM_DYLIB=OFF \
        -DLLVM_INCLUDE_TESTS=OFF \
        -DLLVM_INCLUDE_EXAMPLES=OFF \
        -DLLVM_INCLUDE_BENCHMARKS=OFF \
        -DLLVM_ENABLE_BINDINGS=OFF \
        -DLLVM_ENABLE_OCAMLDOC=OFF \
        -DLLVM_ENABLE_Z3_SOLVER=OFF \
        -DLLVM_ENABLE_TERMINFO=0 \
        -DLLVM_TABLEGEN=../build-host/bin/llvm-tblgen \
        -DCLANG_TABLEGEN=../build-host/bin/clang-tblgen \
        -DLLVM_TARGET_ARCH=wasm32 \
        ../llvm && ninja

CMD ["/work/build.sh"]
