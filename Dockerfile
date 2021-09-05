FROM ubuntu:20.04

RUN apt-get update && apt-get install -y git
RUN git clone --single-branch --branch bpf-tools-v1.15 https://github.com/solana-labs/llvm-project.git
# Install compiler, python and subversion.
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y \
                    build-essential \
                    gcc \
                    g++ \
                    python \
                    python3 \
                    cmake \
                    ninja-build

RUN git clone https://github.com/emscripten-core/emsdk.git && cd emsdk && ./emsdk install 2.0.29 && ./emsdk activate latest

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

RUN cd emsdk && . ./emsdk_env.sh && cd /llvm-project && mkdir build-wasm && cd build-wasm && \
    emcmake cmake -G "Ninja" \
        -DCMAKE_BUILD_TYPE=Release \
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

RUN apt-get install wget
RUN mkdir -p /bpf-tools && wget -qO- https://github.com/solana-labs/bpf-tools/releases/download/v1.15/solana-bpf-tools-linux.tar.bz2 | tar -xj -C bpf-tools

CMD ["/work/build.sh"]
