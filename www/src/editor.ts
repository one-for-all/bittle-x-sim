import * as monaco from "monaco-editor";
import demo_ino from "./assets/OpenCatEsp32.ino";
import movement_sequences from "./assets/movement-sequences.h";
import default_ino_bin_buffer from "./assets/OpenCatEsp32.ino.bin";
import default_symbols from "./assets/symbols.txt";
import readme from "./assets/README.md";
import { getSimulator } from ".";
import AnsiToHtml from "ansi-to-html";
import JSZip from "jszip";

const default_ino_bin = new Uint8Array(default_ino_bin_buffer);

type FileEntry = {
  content: string;
  language: string;
};

let files: Record<string, FileEntry> = {
  "demo.ino": {
    content: demo_ino,
    language: "cpp",
  },
  "movement-sequences.h": {
    content: movement_sequences,
    language: "cpp",
  },
  "README.md": {
    content: readme,
    language: "markdown",
  },
};
let currentFile: string = null;

const editor = monaco.editor.create(document.getElementById("editor")!, {
  language: "cpp",
  theme: "vs-dark",
  automaticLayout: true,
  fontSize: 14,
  minimap: { enabled: false },
});

editor.onDidChangeModelContent(() => {
  if (currentFile) {
    files[currentFile].content = editor.getValue();
  }
});

renderFileBar();
openFile(Object.keys(files)[0]);

function renderFileBar() {
  const fileBar = document.getElementById("fileBar");
  const runButton = document.getElementById("runButton");
  const stopButton = document.getElementById("stopButton");
  fileBar.innerHTML = "";

  Object.keys(files).forEach((filename) => {
    const tab = document.createElement("div");
    tab.className = "file-tab";
    if (filename === currentFile) {
      tab.classList.add("active");
    }

    const icon = getFileIcon(filename);
    tab.innerHTML = `<span class="file-icon">${icon}</span><span>${filename}</span>`;
    tab.onclick = () => openFile(filename);

    fileBar.appendChild(tab);
  });

  fileBar.appendChild(runButton);
  fileBar.appendChild(stopButton);
}

function openFile(filename: string) {
  if (currentFile && files[currentFile]) {
    files[currentFile].content = editor.getValue();
  }

  currentFile = filename;
  const file = files[filename];
  monaco.editor.setModelLanguage(editor.getModel(), file.language);
  editor.setValue(file.content);

  renderFileBar();
}

function getFileIcon(filename: string) {
  const ext = filename.split(".").pop();
  const icons: Record<string, string> = {
    ino: "📝",
    h: "📝",
    md: "📋",
  };
  return icons[ext] || "📄";
}

// Build and Run the code
const url = "https://esp32-compile-api-t2qhjccmsa-uc.a.run.app";

document.getElementById("runButton").addEventListener("click", async () => {
  await runCode();
});

// console.log(stand_symbols);
// console.log(stand_ino_bin.length);

async function runCode() {
  const runButton = document.getElementById("runButton") as HTMLButtonElement;
  const stopButton = document.getElementById("stopButton") as HTMLButtonElement;
  const outputDiv = document.getElementById("buildOutput");
  const outputContent = document.getElementById("outputContent");

  // Save current file content
  if (currentFile && files[currentFile]) {
    files[currentFile].content = editor.getValue();
  }

  const ino_source = files["demo.ino"].content; // get sesame.ino file content
  const header_source = files["movement-sequences.h"].content;

  // Disable button and show loading
  runButton.disabled = true;
  runButton.innerHTML = "<span>⏳</span><span>Compiling...</span>";
  stopButton.disabled = true;

  outputDiv.classList.add("show");
  outputContent.innerHTML = "<div>Compiling esp32 project...</div>";

  try {
    const result = await compileArduinoFromStrings(
      url,
      ino_source,
      header_source,
    );

    let output = "";
    // if (result.stdout) {
    //   const ansi = new AnsiToHtml();
    //   const html = ansi.toHtml(result.stdout);
    //   output += `<div class="stdout">STDOUT:<br>${html}</div>`;
    // }

    // if (result.error) {
    //   const ansi = new AnsiToHtml();
    //   const html = ansi.toHtml(result.details);
    //   output += `<div class="stderr">STDERR:<br>${html}</div>`;
    // }

    if (result.inoBinBytes) {
      output +=
        '<div class="success">✓ Compile successful!\nbin file generated (' +
        result.inoBinBytes.length +
        " bytes)</div>";
      console.log("Compile output:", result.inoBinBytes);
    }

    outputContent.innerHTML =
      output ||
      '<div class="success">Compilation completed successfully!</div>';

    // if (!result.error) {
    //   let simulator = getSimulator();
    //   simulator.hybrid.reset();
    //   simulator.hybrid.reboot_code_controller(0, result.hex);
    //   // simulator.pendulum_raised = false;
    // }
    let simulator = getSimulator();
    simulator.hybrid.reset();
    let targets = [135, 45, 45, 135, 0, 180, 0, 180];
    for (let i = 0; i < targets.length; i++) {
      simulator.hybrid.set_joint_q(i + 1, targets[i] * (Math.PI / 180)); // skip first floating joint
    }
    simulator.hybrid.reboot_esp32_controller(
      0,
      result.inoBinBytes,
      result.symbolsText,
    );
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

    // // 4. Use a <pre> tag to preserve terminal formatting
    // outputContent.innerHTML = `
    //   <div class="error-container">
    //     <div class="error-header">Compile Error</div>
    //     <pre class="stderr">${html}</pre>
    //   </div>
    // `;
    outputContent.innerHTML = `<div class="stderr">Compile error:\n${html}</div>`;
    console.error("Compile error:", error);
  } finally {
    runButton.disabled = false;
    runButton.innerHTML = "<span>🔨</span><span>Run</span>";
    stopButton.disabled = false;
  }
}

document.getElementById("closeOutput").addEventListener("click", async () => {
  document.getElementById("buildOutput").classList.remove("show");
});

document.getElementById("stopButton").addEventListener("click", async () => {
  let simulator = getSimulator();
  simulator.hybrid.reset();
  let targets = [135, 45, 45, 135, 0, 180, 0, 180];
  for (let i = 0; i < targets.length; i++) {
    simulator.hybrid.set_joint_q(i + 1, targets[i] * (Math.PI / 180)); // skip first floating joint
  }
  simulator.hybrid.reboot_esp32_controller(0, default_ino_bin, default_symbols);
});

type CompileStringsResult = {
  inoBinBytes: Uint8Array;
  symbolsText: string;
};

async function compileArduinoFromStrings(
  apiBaseUrl: string,
  inoSource: string,
  headerSource: string,
): Promise<CompileStringsResult> {
  const form = new FormData();
  form.set(
    "ino_file",
    new Blob([inoSource], { type: "text/plain" }),
    "sesame.ino",
  );
  form.set(
    "header_file",
    new Blob([headerSource], { type: "text/plain" }),
    "movement-sequences.h",
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

// Show the output div at the beginning
const outputDiv = document.getElementById("buildOutput");
outputDiv.classList.add("show");

// Update serial monitor message periodically
const serialMonitor = document.getElementById("serialMonitor");
setInterval(() => {
  let simulator = getSimulator();
  if (simulator && simulator.hybrid) {
    serialMonitor.textContent = simulator.hybrid.get_uart();
  }
}, 100); // update every 100 ms

// ===== Serial Monitor Input ================= //
const serialInput = document.getElementById("serialInput") as HTMLInputElement;
const serialSend = document.getElementById("serialSend");

async function sendSerialData() {
  const text = serialInput.value;
  if (!text) return;

  const payload = text + "\n";
  console.log("serial tx: ", JSON.stringify(payload));
  serialInput.value = "";

  let simulator = getSimulator();
  if (simulator && simulator.hybrid) {
    simulator.hybrid.send_uart(payload);
  }
}

serialSend.addEventListener("click", sendSerialData);
serialInput.addEventListener("keydown", (e) => {
  if (e.key === "Enter") sendSerialData();
});
