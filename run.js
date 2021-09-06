var clangFactory = require('./build/clang-solana-bpf.js');

clangFactory().then((clang) => {
  clang.FS.mkdir('/work');
  clang.FS.mount(clang.NODEFS, { root: '.' }, '/work');

  const ret = clang.callMain([
    // "clang",

    "--linker-option=-z notext",
    "--linker-option=-shared",
    "--linker-option=--Bdynamic /usr/share/bpf.ld",
    "--linker-option=--entry entrypoint",

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

    "-o", "/work/test/sha.so",

    "-x", "c++",
    "/work/test/sha.c",
  ]);
  if (ret != 0) {
    console.log('compiler exited with ' + ret);
  }
});
