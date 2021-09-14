<script>
	import { onMount } from 'svelte';
	import editorWorker from 'monaco-editor/esm/vs/editor/editor.worker?worker';

	import clang_factory from './clang-solana-bpf.js';

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
		'/data/bpf.ld',
		'--entry',
		'entrypoint',
		'/data/libcompiler_builtins.rlib'
	];

	async function loadCompiler() {
		const Module = await clang_factory({ noInitialRun: true });

		// create directory /data
		await Module.FS.mkdir('/data');

		// init clang
		window.Module = ClangModule;
		window.clang = new ClangModule.Clang()

		return [Module, clang];
	}

	async function compileAndLink(Module, clang) {
		// write out code and compile
		await Module.FS.writeFile('/data/file.c', editor.getValue());

		let compile_args = new Module.StringList();
		compile_flags.concat([
			'-emit-obj',
			'-triple',
			'bpfel-unknown-unknown-bpfel+solana',
			'-o',
			'/data/file.o',
			'/data/file.c'
		]).forEach(x => compile_args.push_back(x));

		if (!await clang.compile(compile_args)) {
			throw new Error('failed to compile');
		}

		// link object file into shared object
		console.log('Linking...');

		let link_args = new Module.StringList();
		link_flags.concat(['-o', '/data/file.so', '/data/file.o']).forEach(x => link_args.push_back(x))

		if (!await clang.linkBpf(link_args)) {
			throw new Error('failed to link');
		}

		const shared_object_file = await Module.FS.readFile('/data/file.so');
		console.log('program:', shared_object_file);
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
			language: 'c'
		});

		window.addEventListener('resize', () => editor.layout(), false);

		return () => {
			editor.dispose();
		};
	});
</script>

<div id="editor" bind:this={editor_element} />
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
