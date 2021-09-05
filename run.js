var clangFactory = require('./build/clang-solana-bpf.js');

clangFactory().then((clang) => {
  clang.FS.mkdir('/work');
  clang.FS.mount(clang.NODEFS, { root: '.' }, '/work');

  const ret = clang.callMain(['/work/test/sha.c', '/work/sha.o']);
  if (ret != 0) {
    console.log('compiler exited with ' + ret);
  }
});
