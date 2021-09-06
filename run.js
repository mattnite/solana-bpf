const clangFactory = require('./build/clang-solana-bpf.js');
const lldFactory = require('./build/lld.js');

(async () => {
  const clang = await clangFactory({ noInitialRun: true });
  const lld = await lldFactory({ noInitialRun: true });

  clang.FS.mkdir('/work');
  clang.FS.mount(clang.NODEFS, { root: '.' }, '/work');
  lld.FS.mkdir('/clang');
  lld.FS.mount(lld.PROXYFS, {
    root: "/",
    fs: clang.FS
  }, "/clang");

  let ret = clang.callMain([
    "-Werror",
    "-O2",
    "-fno-builtin",
    "-std=c17",
    "-isystem/usr/include/clang",
    "-isystem/usr/include/solana",
    "-mrelocation-model", "pic",
    "-pic-level", "2",
    "-triple", "bpfel-unknown-unknown-bpfel+solana",
    "-emit-obj",

    "-o", "/work/sha.o",
    "/work/test/sha.c",
  ]);
  if (ret != 0) {
    console.log('compiler exited with ' + ret);
  }

  ret = lld.callMain([
    "-flavor", "ld",
    "-z", "notext",
    "-shared",
    "--Bdynamic",
    "/clang/usr/share/bpf.ld",
    "--entry", "entrypoint",
    "-o", "/clang/work/sha.so",
    "/clang/work/sha.o",
    "/clang/usr/lib/libcompiler_builtins.rlib",
  ]);
  if (ret != 0) {
    console.log('lld exited with ' + ret);
  }
})();
