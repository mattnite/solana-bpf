<script>
	import { onMount } from 'svelte';
	import editorWorker from 'monaco-editor/esm/vs/editor/editor.worker?worker';

	import clang_factory from './clang-solana-bpf.js';
	import lld_factory from './lld.js';

	console.log(clang_factory);
	console.log(lld_factory);

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

	let clang_flags = [
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

	let lld_flags = [
		'-flavor',
		'ld',
		'-z',
		'notext',
		'-shared',
		'--Bdynamic',
		'/data/bpf.ld',
		'--entry',
		'entrypoint',
		'/data/compiler_builtins.rlib'
	];

	async function loadCompiler() {
		const [clang, lld] = await Promise.all([
			clang_factory({ noInitialRun: true }),
			lld_factory({ noInitialRun: true })
		]);

		// move linker scripts from clang to lld

		const linker_script = await clang.FS.readFile('/usr/share/bpf.ld');
		const compiler_builtins = await clang.FS.readFile('/usr/lib/libcompiler_builtins.rlib');

		await lld.FS.mkdir('/data');
		await clang.FS.mkdir('/data');

		await lld.FS.writeFile('/data/bpf.ld', linker_script);
		await lld.FS.writeFile('/data/compiler_builtins.rlib', compiler_builtins);

		return [clang, lld];
	}

	async function compileAndLink(clang, lld) {
		// write out code and compile
		await clang.FS.writeFile('/data/file.c', editor.getValue());
    let args = new clang.StringList();
    clang_flags.concat([
      "-emit-obj",
      "-triple",
      "bpfel-unknown-unknownbpefel+solana",
      "-o",
      "/data/file.o",
      "/data/file.c",
    ]).forEach(x => args.push_back(x));

    const clang_result = await clang.run(args);
		if (clang_result != 0) {
			throw new Error(`clang exited with error code: ${clang_result}`);
		}

		// link object file into shared object
		console.log('Linking...');

		const object_file = await clang.FS.readFile('/data/file.o');
		await lld.FS.writeFile('/data/file.o', object_file);

		const lld_result = await lld.callMain(
			lld_flags.concat(['-o', '/data/file.so', '/data/file.o'])
		);
		if (lld_result != 0) {
			throw new Error(`lld exited with error code: ${lld_result}`);
		}

		const shared_object_file = await lld.FS.readFile('/data/file.so');
		console.log('program:', shared_object_file);
	}

	let compiler_promise = loadCompiler();
	let compiling_promise = Promise.all([compiler_promise, Promise.resolve()]);

	let editor_element;
	let editor;

	onMount(async () => {
		const monaco = await import('monaco-editor');

		self.MonacoEnvironment = {
			getWorker: function (_module_id, label) {
				return new editorWorker();
			}
		};

		editor = monaco.editor.create(editor_element, {
			value: code,
			language: 'c'
		});

		window.addEventListener('resize', () => editor.layout(), false);

		return () => {
			editor.dispose();
		};
	});
</script>

<div id="editor" bind:this={editor_element} />
<input type="text" value={clang_flags.join(' ')} />
<input type="text" value={lld_flags.join(' ')} />

{#await compiling_promise}
	<button disabled>Build</button>
{:then [[clang, lld], _]}
	<button
		on:click|once={() =>
			(compiling_promise = Promise.all([compiler_promise, compileAndLink(clang, lld)]))}
		>Build</button
	>
{/await}

<style>
	#editor {
		width: 100%;
		height: 50%;
	}

	input {
		width: 100%;
	}
</style>
