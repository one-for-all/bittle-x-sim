import { reset_simulator } from "./sim";
import AnsiToHtml from "ansi-to-html";
import JSZip from "jszip";
import { currentFile, files, inoFileName } from "./files";
import { editor } from "./editor";

// Build and Run the code
// const url = "http://localhost:8081";
const url =
  "https://esp32-compile-api-v2-0-12-452188812531.us-central1.run.app";

document.getElementById("runButton").addEventListener("click", async () => {
  await compileCode();
});

async function buildZipBuffer(): Promise<ArrayBuffer> {
  const zip = new JSZip();

  // Do not zip the following files inside src directory
  let zip_file_names = Object.keys(files).filter(
    (key) => key != "README.md" && !key.endsWith(".ino"),
  );

  zip_file_names.forEach((zip_file_name) => {
    zip.file(zip_file_name, files[zip_file_name].content);
  });

  // Generate the zip as an ArrayBuffer
  return await zip.generateAsync({ type: "arraybuffer" });
}

async function compileCode() {
  const runButton = document.getElementById("runButton") as HTMLButtonElement;
  const outputDiv = document.getElementById("buildOutput");
  outputDiv.scrollTop = 0; // scroll to top
  const outputContent = document.getElementById("outputContent");

  // Save current file content
  if (currentFile && files[currentFile]) {
    files[currentFile].content = editor.getValue();
  }

  const ino_source = files[inoFileName()].content;

  // Disable button and show loading
  runButton.disabled = true;
  runButton.innerHTML = "<span>⏳</span><span>Compiling...</span>";

  outputDiv.classList.add("show");
  outputContent.innerHTML =
    '<div class="success">Compiling esp32 project...</div>';

  try {
    const zipBuffer = await buildZipBuffer();

    const result = await compileArduinoFromStrings(url, ino_source, zipBuffer);

    let output = "";

    if (result.inoBinBytes) {
      outputDiv.scrollTop = 0; // scroll to top
      output +=
        '<div class="success">✓ Compile successful!\nbin file generated (' +
        result.inoBinBytes.length +
        " bytes)</div>";
      console.log("Compile output:", result.inoBinBytes);
    }

    outputContent.innerHTML =
      output ||
      '<div class="success">Compilation completed successfully!</div>';

    reset_simulator(result.inoBinBytes, result.symbolsText);
  } catch (error) {
    const ansi = new AnsiToHtml();
    let cleanLog = error.message;

    try {
      // 1. Extract the JSON part from the error message string
      const jsonStartIndex = error.message.indexOf("{");
      if (jsonStartIndex !== -1) {
        const jsonStr = error.message.substring(jsonStartIndex);
        const parsed = JSON.parse(jsonStr);

        // 2. Target the specific log property
        cleanLog =
          parsed.detail?.compile_log || parsed.detail?.message || error.message;
      }
    } catch (e) {
      // If parsing fails, we just fall back to the raw message
      console.error("Could not parse error JSON", e);
    }

    // 3. Convert ANSI to HTML
    const html = ansi.toHtml(cleanLog);

    outputContent.innerHTML = `<div class="stderr">Compile error:\n${html}</div>`;
    console.error("Compile error:", error);
  } finally {
    runButton.disabled = false;
    runButton.innerHTML = "<span>🔨</span><span>Compile</span>";
  }
}

type CompileStringsResult = {
  inoBinBytes: Uint8Array;
  symbolsText: string;
};

async function compileArduinoFromStrings(
  apiBaseUrl: string,
  inoSource: string,
  zipBuffer: ArrayBuffer,
): Promise<CompileStringsResult> {
  const form = new FormData();
  form.set(
    "ino_file",
    new Blob([inoSource], { type: "text/plain" }),
    "main.ino",
  );
  form.set(
    "zip_bundle",
    new Blob([zipBuffer], { type: "application/zip" }),
    "src.zip",
  );

  const response = await fetch(`${apiBaseUrl.replace(/\/+$/, "")}/compile`, {
    method: "POST",
    body: form,
  });

  if (!response.ok) {
    const errorText = await response.text();
    throw new Error(`Compile API failed (${response.status}): ${errorText}`);
  }

  const zipBytes = await response.arrayBuffer();
  const zip = await JSZip.loadAsync(zipBytes);

  const inoBinFile = Object.values(zip.files).find(
    (entry) => !entry.dir && entry.name.endsWith(".ino.bin"),
  );
  const symbolsFile = zip.file("symbols.txt");

  if (!inoBinFile) {
    throw new Error("Response zip missing .ino.bin artifact");
  }
  if (!symbolsFile) {
    throw new Error("Response zip missing symbols.txt");
  }

  const inoBinBytes = await inoBinFile.async("uint8array");
  const symbolsText = await symbolsFile.async("string");

  return { inoBinBytes, symbolsText };
}
