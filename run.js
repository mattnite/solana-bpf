const clangFactory = require('./build/clang-solana-bpf.js');

(async () => {
  const clang = await clangFactory({ noInitialRun: true });

  clang.FS.mkdir('/work');
  clang.FS.mount(clang.NODEFS, { root: '.' }, '/work');

  const ret = clang.callMain([
    // "--linker-option=-z notext",
    // "--linker-option=--shared",
    // "--linker-option=--Bdynamic /usr/share/bpf.ld",
    // "--linker-option=--entry=entrypoint",

    "-Werror",
    "-O2",
    "-fno-builtin",
    "-std=c17",

    "-std=c++17",
    // "-fPIC",
    // "-fomit-frame-pointer",
    // "-fno-exceptions",
    // "-fno-asynchronous-unwind-tables",
    // "-fno-unwind-tables",

    "-I/usr/include/clang",
    "-I/usr/include/solana",

    "-triple", "bpfel-unknown-unknown-bpfel+solana",
    "-emit-obj",

    "-o", "/work/sha.so",
    "-version",

    "-x", "c++",
    "/work/test/sha.c",
  ]);
  if (ret != 0) {
    console.log('compiler exited with ' + ret);
  }
})();