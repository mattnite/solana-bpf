<script>
	import { onMount } from 'svelte';
	import editorWorker from 'monaco-editor/esm/vs/editor/editor.worker?worker';
	import clang_factory from './clang-solana-bpf.js';
	import Convert from 'ansi-to-html';

	var convert = new Convert({ newline: true });
	const toHtml = text => `${convert.toHtml(text)}`;
  const wasi_snapshot_preview1 = {
    args_get: (one, two) => 0,
    args_sizes_get: (one, two) => 0,
    environ_get: (one, two) => 0,
    environ_sizes_get: (one, two) => 0,
    clock_res_get: (one, two) => 0,
    clock_time_get: (one, two, three) => 0,
    fd_advise: (one, two, three, four) => 0,
    fd_allocate: (one, two, three) => 0,
    fd_close: one => 0,
    fd_datasync: one => 0,
    fd_fdstat_get: (one, two) => 0,
    fd_fdstat_set_flags: (one, two) => 0,
    fd_fdstat_set_rights: (one, two, three) => 0,
    fd_filestat_get: (one, two) => 0,
    fd_filestat_set_size: (one, two) => 0,
    fd_filestat_set_times: (one, two, three, four) => 0,
    fd_pread: (a, b, c, d, e) => 0,
    fd_prestat_get: (one, two) => 0,
    fd_prestat_dir_name: (a, b, c) => 0,
    fd_pwrite: (a, b, c, d, e) => 0,
    fd_read: (a, b, c, d) => 0,
    fd_readdir: (a, b, c, d, e) => 0,
    fd_renumber: (one, two) => 0,
    fd_seek: (a, b, c, d) => 0,
    fd_sync: one => 0,
    fd_tell: (one, two) => 0,
    fd_write: (a, b, c, d) => 0,
    path_create_directory: (a, b, c) => 0,
    path_filestat_get: (a, b, c, d, e) => 0,
    path_filestat_set_times: (a, b, c, d, e, f, g) => 0,
    path_link: (a, b, c, d, e, f, g) => 0,
    path_open: (a, b, c, d, e, f, g, h, i) => 0,
    path_readlink: (a, b, c, d, e, f) => 0,
    path_remove_directory: (a, b, c) => 0,
    path_rename: (a, b, c, d, e, f) => 0,
    path_symlink: (a, b, c, d, e) => 0,
    path_unlink_file: (a, b, c) => 0,
    poll_oneoff: (a, b, c, d) => 0,
    proc_exit: a => {},
    proc_raise: one => 0,
    sched_yield: () => 0,
    random_get: (one, two) => 0,
    sock_recv: (a, b, c, d, e, f) => 0,
    sock_send: (a, b, c, d, e) => 0,
    sock_shutdown: (one, two) => 0,
  }

  let test_code = `
/**
 * @brief C-based Helloworld BPF program
 */
#include <solana_sdk.h>
#include <criterion/criterion.h>

uint64_t helloworld(SolParameters *params) {

  if (params->ka_num < 1) {
    sol_log("Greeted account not included in the instruction");
    return ERROR_NOT_ENOUGH_ACCOUNT_KEYS;
  }

  // Get the account to say hello to
  SolAccountInfo *greeted_account = &params->ka[0];

  // The account must be owned by the program in order to modify its data
  if (!SolPubkey_same(greeted_account->owner, params->program_id)) {
    sol_log("Greeted account does not have the correct program id");
    return ERROR_INCORRECT_PROGRAM_ID;
  }

  // The data must be large enough to hold an uint32_t value
  if (greeted_account->data_len < sizeof(uint32_t)) {
    sol_log("Greeted account data length too small to hold uint32_t value");
    return ERROR_INVALID_ACCOUNT_DATA;
  }

  // Increment and store the number of times the account has been greeted
  uint32_t *num_greets = (uint32_t *)greeted_account->data;
  *num_greets += 1;

  sol_log("Hello!");

  return SUCCESS;
}

extern uint64_t entrypoint(const uint8_t *input) {
  sol_log("Helloworld C program entrypoint");

  SolAccountInfo accounts[1];
  SolParameters params = (SolParameters){.ka = accounts};

  if (!sol_deserialize(input, &params, SOL_ARRAY_SIZE(accounts))) {
    return ERROR_INVALID_ARGUMENT;
  }

  return helloworld(&params);
}

Test(hello, sanity) {
  uint8_t instruction_data[] = {};
  SolPubkey program_id = {.x = {
                              1,
                          }};
  SolPubkey key = {.x = {
                       2,
                   }};
  uint64_t lamports = 1;
  uint8_t data[] = {0, 0, 0, 0};
  SolAccountInfo accounts[] = {{
      &key,
      &lamports,
      sizeof(data),
      data,
      &program_id,
      0,
      true,
      true,
      false,
  }};
  SolParameters params = {accounts, sizeof(accounts), instruction_data,
                          sizeof(instruction_data), &program_id};

  cr_assert(0 == *(uint32_t *)data);
  cr_assert(SUCCESS == helloworld(&params));
  cr_assert(1 == *(uint32_t *)data);
  cr_assert(SUCCESS == helloworld(&params));
  cr_assert(2 == *(uint32_t *)data);
}
`;

	let code = `/**
 * @brief A program demonstrating logging
 */
#include <solana_sdk.h>

extern uint64_t logging(SolParameters *params) {
  // Log a string
  sol_log("static string");

  // Log 5 numbers as u64s in hexadecimal format
  sol_log_64(params->data[0], params->data[1], params->data[2], params->data[3],
             params->data[4]);

  // Log a slice
  sol_log_array(params->data, params->data_len);

  // Log a public key
  sol_log_pubkey(params->program_id);

  // Log all the program's input parameters
  sol_log_params(params);

  // Log the number of compute units remaining that the program can consume.
  sol_log_compute_units();

  return SUCCESS;
}

extern uint64_t entrypoint(const uint8_t *input) {
  SolAccountInfo accounts[0];
  SolParameters params = (SolParameters){.ka = accounts};

  if (!sol_deserialize(input, &params, SOL_ARRAY_SIZE(accounts))) {
    return ERROR_INVALID_ARGUMENT;
  }

  return logging(&params);
}`;

	let compile_flags = [
		'-Werror',
		'-O2',
		'-fno-builtin',
		'-std=c17',
		'-isystem/usr/include/clang',
		'-isystem/usr/include/solana',
		'-mrelocation-model',
		'pic',
		'-pic-level',
		'2'
	];

	let link_flags = [
		'-z',
		'notext',
		'-shared',
		'--Bdynamic',
		'/usr/share/bpf.ld',
		'--entry',
		'entrypoint',
    '/usr/lib/libcompiler_builtins.rlib',
	];

	let compiler_text = "starting text";
  let test_text = "test text";

	async function loadCompiler() {
    var compiler = undefined;
    try {
		  compiler = await clang_factory({ noInitialRun: true });
    } catch (e) {
      console.log(e);
      return
    }

		// create directory /data
		await compiler.FS.mkdir('/data');

		// init clang
		window.compiler = compiler;
		window.clang = new compiler.Clang();

		return [compiler, clang];
	}
	async function compileTestAndLink(Module, clang) {
    console.log('compile test mock');
    await Module.FS.writeFile('/data/test.c', test_code);
    let compile_args = new Module.StringList();
    [
      '-emit-obj',
      '-triple', 'wasm32-unknown-wasi',
      '-isysroot', '/usr/share/wasi-sysroot',
      '-internal-isystem', '/usr/share/wasi-sysroot/include',
      '-isystem', '/usr/include/clang',
      '-isystem', '/usr/include/solana',
      '-isystem', '/usr/include/test',
      '-DALON_TEST',
      '-o', '/data/test.o',
      '/data/test.c',
    ].forEach(x => compile_args.push_back(x));

		let compile_result = await clang.compile(compile_args);
		if (!compile_result.success) {
			test_text = compile_result.diags; 
			return;
		}

    compile_args = new Module.StringList();
    [
      '-emit-obj',
      '-triple', 'wasm32-unknown-wasi',
      '-isysroot', '/usr/share/wasi-sysroot',
      '-internal-isystem', '/usr/share/wasi-sysroot/include',
      '-isystem', '/usr/include/clang',
      '-isystem', '/usr/include/solana',
      '-isystem', '/usr/include/test',
      '-DALON_TEST',
      '-o', '/data/solana_test.o',
      '/usr/share/solana_test.c',
    ].forEach(x => compile_args.push_back(x));

		compile_result = await clang.compile(compile_args);
		if (!compile_result.success) {
			test_text = compile_result.diags; 
			return;
		}

		const test_object = await Module.FS.readFile('/data/test.o');
    console.log(test_object);

		let link_args = new Module.StringList();
    [
      '--no-entry',
      '--import-memory',
      '--export-all',
      '-L', '/usr/share/wasi-sysroot/lib/wasm32-wasi',
      '-lc',
      '-o', '/data/test.wasm',
      '/data/test.o',
      '/data/solana_test.o'
    ].forEach(x => link_args.push_back(x));

		const link_result = await clang.linkWasm(link_args);
		if (!link_result.success) {
			test_text = link_result.err;
			return;
		}

		const wasm_bytes = await Module.FS.readFile('/data/test.wasm');
		await Module.FS.unlink('/data/test.wasm');
		console.log('program:', wasm_bytes);
    test_text = '';

    try {
      const memory = new WebAssembly.Memory({ initial: 2 });
      const { instance } = await WebAssembly.instantiate(wasm_bytes, {
        env: { memory },
        wasi_snapshot_preview1,
      });

      var total = 0;
      for (const [key, _] of Object.entries(instance.exports)) {
        if (key.startsWith("test_")) {
          total += 1;
        }
      };

      var count = 1;
      for (const [key, test_func] of Object.entries(instance.exports)) {
        console.log(`key: ${key}`);
        if (!key.startsWith("test_"))
          continue;

        let name = key.substring("test_".length, key.length).replace("__", "::");

        try {
          test_func();
          test_text = test_text + `\u001b[32m[${count}/${total}] ${name} ✓ success!\u001b[0m\n`;
        } catch (e) {
          test_text = test_text + `\u001b[31m[${count}/${total}] ${name} ✗ failed\u001b[0m\n${e.stack}\n\n`;
        }

        count += 1
      }
    } catch (e) {
      test_text = test_text + 'failed to load wasm module:\n' + e;
    }

    console.log('done tests');
  }

	async function compileAndLink(Module, clang) {
		// write out code and compile
		await Module.FS.writeFile('/data/file.c', editor.getValue());

		let compile_args = new Module.StringList();
		compile_flags
			.concat([
				'-emit-obj',
				'-triple',
				'bpfel-unknown-unknown-bpfel+solana',
				'-o',
				'/data/file.o',
				'/data/file.c',
			])
			.forEach(x => compile_args.push_back(x));

		const compile_result = await clang.compile(compile_args);
		if (!compile_result.success) {
			compiler_text = compile_result.diags; 
			return;
		}

		// link object file into shared object
		console.log('Linking...');

		let link_args = new Module.StringList();
		link_flags
			.concat(['-o', '/data/file.so', '/data/file.o'])
			.forEach(x => link_args.push_back(x));

		const link_result = await clang.linkBpf(link_args);
		if (!link_result.success) {
			compiler_text = link_result.err;
			return;
		}

		const shared_object_file = await Module.FS.readFile('/data/file.so');
		await Module.FS.unlink('/data/file.so');
		console.log('program:', shared_object_file);
		compiler_text = `success! binary size: ${shared_object_file.byteLength} bytes`;
	}

	let compiler_promise = loadCompiler();
	let compiling_promise = Promise.all([compiler_promise, Promise.resolve()]);

	let editor_element;
	let editor;

	onMount(async () => {
		const monaco = await import('monaco-editor');

		self.MonacoEnvironment = {
			getWorker: function (_module_id, _label) {
				return new editorWorker();
			}
		};

		editor = monaco.editor.create(editor_element, {
			value: code,
			language: 'c',
			theme: 'vs-dark',
			scrollbar: {
				horizontal: 'auto',
				vertical: 'visible'
			}
			// automaticLayout: true
		});

		window.addEventListener('resize', () => editor.layout(), false);

		return () => {
			editor.dispose();
		};
	});
</script>

<div id="container">
	<div id="left">
		<h2>ALON</h2>
	</div>
	<div id="main">
		<div id="options">
			<input type="text" value={compile_flags.join(' ')} />
			<input type="text" value={link_flags.join(' ')} />

			{#await compiling_promise}
				<button disabled>Build</button>
			{:then [[Module, clang], _]}
				<button
					on:click|once={() =>
						(compiling_promise = Promise.all([compiler_promise, compileAndLink(Module, clang)]))}
					>Build</button
				>
			{:catch}
				{#await compiler_promise then [Module, clang]}
					<button
						on:click|once={() =>
							(compiling_promise = Promise.all([compiler_promise, compileAndLink(Module, clang)]))}
						>Build</button
					>
				{/await}
			{/await}

			{#await compiling_promise}
				<button disabled>Test</button>
			{:then [[Module, clang], _]}
				<button
					on:click|once={() =>
						(compiling_promise = Promise.all([compiler_promise, compileTestAndLink(Module, clang)]))}
					>Test</button
				>
			{:catch}
				{#await compiler_promise then [Module, clang]}
					<button
						on:click|once={() =>
							(compiling_promise = Promise.all([compiler_promise, compileTestAndLink(Module, clang)]))}
						>Test</button
					>
				{/await}
			{/await}

		</div>

		<div id="editor" bind:this={editor_element} />
	</div>
	<div id="right">
		<div class="console">
			<b>compiler</b>
			<div class="output">
				{@html toHtml(compiler_text)}
			</div>
		</div>

		<div class="console">
			<b>test</b>
			<div class="output">
				{@html toHtml(test_text)}
			</div>
		</div>

		<div class="console">
			<b>solana</b>
			<div class="output">
				<textarea />
			</div>
		</div>
	</div>
</div>

<style>
	* {
		font-family: 'Droid Sans Mono', monospace, monospace, 'Droid Sans Fallback';
	}

	h1,
	h2,
	h3,
	h4 {
		padding: 0;
		margin: 0;
	}

	#container {
		display: flex;
		width: 100%;
		height: 100%;
		flex-direction: row;
		justify-content: space-between;
	}

	#container > #left {
		display: flex;
		flex-direction: column;
		width: 300px;
	}

	#container > #left > h2 {
		padding: 0.5em;
		width: 100%;
	}

	#container > #right {
		display: flex;
		flex-direction: column;
		width: 400px;
	}

	#container > #right > .console {
		display: flex;
		flex-direction: column;
		align-items: flex-start;
		flex: 1;
	}

	#container > #right > .console > b {
		padding-top: 0.5em;
		padding-left: 0.5em;
		padding-right: 0.5em;
		padding-bottom: 0;
		margin: 0;
	}

	#container > #right > .console > .output {
		flex: 1;
		width: 100%;
		padding-top: 0.5em;
		overflow: auto;
		white-space: nowrap;
		background-color: black;
		color: grey;
	}

	#container > #right > .console > .output > textarea {
		width: 100%;
		height: 100%;
		background-color: black;
		padding: 0;
		border: 0;
	}

	#container > #main {
		display: flex;
		width: calc(100% - 700px);
		height: 100%;
		flex-direction: column;
	}

	#container > #main > #editor {
		width: 100%;
		height: 100%;
		overflow: hidden;
	}

	#container > #main > #options {
		display: flex;
		flex-direction: row;
	}

	#container > #main > #options > input {
		width: 100%;
	}
</style>
