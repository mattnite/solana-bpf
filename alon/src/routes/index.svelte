<script>
	import { onMount } from 'svelte';
	import editorWorker from 'monaco-editor/esm/vs/editor/editor.worker?worker';

	import clang_factory from './clang-solana-bpf.js';
	import lld_factory from './lld.js';

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
		'/clang/usr/share/bpf.ld',
		'--entry',
		'entrypoint',
		'/clang/usr/lib/libcompiler_builtins.rlib'
	];

	async function loadCompiler() {
		const [clang, lld] = await Promise.all([
			clang_factory({ noInitialRun: true }),
			lld_factory({ noInitialRun: true })
		]);

		// init clang

		await clang.init();

		window.clang = clang;
		window.lld = lld;

		// mount fs

		await lld.FS.mkdir('/clang');
		await lld.FS.mount(lld.PROXYFS, { root: '/', fs: clang.FS }, '/clang');

		// create directory /data

		await clang.FS.mkdir('/data');

		return [clang, lld];
	}

	async function compileAndLink(clang, lld) {
		// write out code and compile
		await clang.FS.writeFile('/data/file.c', editor.getValue());

		let args = new clang.StringList();
		clang_flags
			.concat([
				'-emit-obj',
				'-triple',
				'bpfel-unknown-unknown-bpfel+solana',
				'-o',
				'/data/file.o',
				'/data/file.c'
			])
			.forEach((x) => args.push_back(x));

		const clang_result = await clang.run(args);
		if (clang_result != 0) {
			throw new Error(`clang exited with error code: ${clang_result}`);
		}

		// sync fs

		await new Promise((resolve, reject) =>
			clang.FS.syncfs(false, (err) => {
				if (err) reject(err);
				resolve();
			})
		);

		await new Promise((resolve, reject) =>
			clang.FS.syncfs(true, (err) => {
				if (err) reject(err);
				resolve();
			})
		);

		// link object file into shared object

		console.log('Linking...');

		const lld_result = await lld.callMain(
			lld_flags.concat(['-o', '/clang/data/file.so', '/clang/data/file.o'])
		);
		if (lld_result != 0) {
			throw new Error(`lld exited with error code: ${lld_result}`);
		}

		const shared_object_file = await lld.FS.readFile('/clang/data/file.so');
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
