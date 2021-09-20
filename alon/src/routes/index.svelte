<script>
	import { onMount } from 'svelte';
	import editorWorker from 'monaco-editor/esm/vs/editor/editor.worker?worker';
	import clang_factory from './clang-solana-bpf.js';
	import Convert from 'ansi-to-html';

	var convert = new Convert({ newline: true });
	const toHtml = text => `${convert.toHtml(text)}`;

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
		'/usr/lib/libcompiler_builtins.rlib'
	];

	let compiler_text = "starting text";

	async function loadCompiler() {
		const compiler = await clang_factory({ noInitialRun: true });

		// create directory /data
		await compiler.FS.mkdir('/data');

		// init clang
		window.compiler = compiler;
		window.clang = new compiler.Clang();

		return [compiler, clang];
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
				'/data/file.c'
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
				<textarea />
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
