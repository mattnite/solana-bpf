const clangFactory = require('./build/clang-solana-bpf.js');
const lldFactory = require('./build/lld.js');

export async function createProgram(sources) {
  const objects = await Promise.all(sources.map(file => compile(file.contents)));
  console.log(objects);

  const clang = await clangFactory({ noInitialRun: true });
  const linkerscript = await clang.FS.readFile('/usr/share/bpf.ld');
  const compiler_builtins = await clang.FS.readFile('/usr/lib/libcompiler_builtins.rlib');

  console.log('about to link');
  return link(linkerscript, compiler_builtins, objects);
}

export async function compile(source) {
  const clang = await clangFactory({ noInitialRun: true });
  clang.FS.mkdir('/data');
  clang.FS.writeFile('/data/file.c', source);
  const ret = clang.callMain([
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

    "-o", "/data/out.o",
    "/data/file.c",
  ]);
  if (ret != 0) {
    console.log('compiler exited with ' + ret);
    throw 'failed to compile';
  }

  const file = clang.FS.readFile('/data/out.o');
  console.log(file);
  return file;
}

export async function link(linkerscript, compiler_builtins, objects) {
  const lld = await lldFactory({ noInitialRun: true });
  lld.FS.mkdir('/data');

  lld.FS.writeFile('/data/bpf.ld', linkerscript);
  lld.FS.writeFile('/data/compiler_builtins.rlib', compiler_builtins);
  const file_paths = objects.map((obj, i) => {
    const path = '/data/file' + i + '.o';
    lld.FS.writeFile(path, obj);
    return path;
  });

  console.log(file_paths);
  const args = [
    "-flavor", "ld",
    "-z", "notext",
    "-shared",
    "--Bdynamic",
    "/data/bpf.ld",
    "--entry", "entrypoint",
    "-o", "/data/out.so",
  ];
  
  const ret = lld.callMain(args.concat(file_paths.concat(['/data/compiler_builtins.rlib'])));
  if (ret != 0) {
    console.log('lld exited with ' + ret);
    throw 'failed to link';
  }

  return lld.FS.readFile('/data/out.so');
}
