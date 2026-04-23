import * as monaco from "monaco-editor";
import demo_ino from "./assets/OpenCatEsp32.ino";
// import movement_sequences from "./assets/movement-sequences.h";
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
  scrollTop?: number; // Added for scroll position
  scrollLeft?: number; // Added for scroll position
};

let files: Record<string, FileEntry> = {
  "demo.ino": {
    content: demo_ino,
    language: "cpp",
  },
  "README.md": {
    content: readme,
    language: "markdown",
  },
};
let currentFile: string = null;

// 1. Create the Webpack context
// Arguments: (Directory, Search Subdirectories?, Regex to match files)
const filesContext = require.context(
  "./assets/src",
  true,
  /\.(cpp|c|h|hpp|ino)$/i,
);

// 2. Iterate through all the matched files
filesContext.keys().forEach((key) => {
  // 'key' looks like: "./main.cpp" or "./utils/config.h"
  // filesContext(key) returns the raw string content (thanks to your asset/source rule)
  const fileContent = filesContext(key);

  // add to files record
  files[key.replace(/^\.\//, "")] = {
    content: fileContent,
    language: "cpp",
  };
});

const editor = monaco.editor.create(document.getElementById("editor")!, {
  language: "cpp",
  theme: "vs-dark",
  automaticLayout: true,
  fontSize: 14,
  minimap: { enabled: false },
});

editor.onDidChangeModelContent(() => {
  if (currentFile && files[currentFile]) {
    files[currentFile].content = editor.getValue();
  }
});

renderFileBar();
renderSidebar();
openFile(Object.keys(files)[0]);

document.getElementById("toggleSidebar")!.addEventListener("click", () => {
  document.getElementById("sidebar")!.classList.toggle("hidden");

  window.dispatchEvent(new Event("resize"));
});

const mainResizeHandle = document.getElementById("mainResizeHandle")!;
const container = document.querySelector(".container") as HTMLElement;

mainResizeHandle.addEventListener("mousedown", (e) => {
  e.preventDefault();
  document.addEventListener("mousemove", resizeMain);
  document.addEventListener("mouseup", stopResizeMain);
});

function resizeMain(e: MouseEvent) {
  const containerRect = container.getBoundingClientRect();
  const newWidth = e.clientX - containerRect.left;
  const maxWidth = window.innerWidth * 0.8; // Prevent container from taking more than 80%

  if (newWidth > 200 && newWidth < maxWidth) {
    container.style.flex = `0 0 ${newWidth}px`;
  }

  window.dispatchEvent(new Event("resize"));
}

function stopResizeMain() {
  document.removeEventListener("mousemove", resizeMain);
  document.removeEventListener("mouseup", stopResizeMain);
}

function renderSidebar() {
  const sidebarFiles = document.getElementById("sidebarFiles");
  if (!sidebarFiles) return;
  sidebarFiles.innerHTML = "";

  Object.keys(files).forEach((filename) => {
    const fileDiv = document.createElement("div");
    fileDiv.className = "sidebar-file";
    if (filename === currentFile) {
      fileDiv.classList.add("active");
    }
    fileDiv.textContent = filename;
    fileDiv.onclick = () => openFile(filename);
    sidebarFiles.appendChild(fileDiv);
  });
}

function renderFileBar() {
  const fileTabScroll = document.getElementById("fileTabScroll");
  fileTabScroll.innerHTML = "";

  Object.keys(files).forEach((filename) => {
    const tab = document.createElement("div");
    tab.className = "file-tab";
    if (filename === currentFile) {
      tab.classList.add("active");
    }

    const icon = getFileIcon(filename);
    tab.innerHTML = `<span class="file-icon">${icon}</span><span>${filename}</span>`;
    tab.onclick = () => openFile(filename);

    fileTabScroll.appendChild(tab);
  });
}

function openFile(filename: string) {
  if (currentFile && files[currentFile]) {
    files[currentFile].content = editor.getValue();
    // Save scroll position before switching
    files[currentFile].scrollTop = editor.getScrollTop();
    files[currentFile].scrollLeft = editor.getScrollLeft();
  }

  currentFile = filename;
  const file = files[filename];
  monaco.editor.setModelLanguage(editor.getModel(), file.language);
  editor.setValue(file.content);

  // Restore scroll position for the new file
  editor.setScrollPosition({
    scrollTop: file.scrollTop || 0,
    scrollLeft: file.scrollLeft || 0,
  });

  renderFileBar();
  renderSidebar();
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
// const url = "http://localhost:8081";
const url =
  "https://esp32-compile-api-v2-0-12-452188812531.us-central1.run.app";

document.getElementById("runButton").addEventListener("click", async () => {
  await runCode();
});

async function buildZipBuffer(): Promise<ArrayBuffer> {
  const zip = new JSZip();

  // Do not zip the following files inside src directory
  let zip_file_names = Object.keys(files).filter(
    (key) => key != "demo.ino" && key != "README.md",
  );

  zip_file_names.forEach((zip_file_name) => {
    zip.file("src/" + zip_file_name, files[zip_file_name].content);
  });

  // Zip the main ino file
  zip.file("demo.ino", files["demo.ino"].content);

  // Generate the zip as an ArrayBuffer
  return await zip.generateAsync({ type: "arraybuffer" });
}

async function runCode() {
  const runButton = document.getElementById("runButton") as HTMLButtonElement;
  // const stopButton = document.getElementById("stopButton") as HTMLButtonElement;
  const outputDiv = document.getElementById("buildOutput");
  outputDiv.scrollTop = 0; // scroll to top
  const outputContent = document.getElementById("outputContent");

  // Save current file content
  if (currentFile && files[currentFile]) {
    files[currentFile].content = editor.getValue();
  }

  const ino_source = files["demo.ino"].content; // get demo.ino file content

  // Disable button and show loading
  runButton.disabled = true;
  runButton.innerHTML = "<span>⏳</span><span>Compiling...</span>";
  // stopButton.disabled = true;

  outputDiv.classList.add("show");
  outputContent.innerHTML =
    '<div class="success">Compiling esp32 project...</div>';

  try {
    const zipBuffer = await buildZipBuffer();

    const result = await compileArduinoFromStrings(url, ino_source, zipBuffer);

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

    // if (!result.error) {
    //   let simulator = getSimulator();
    //   simulator.hybrid.reset();
    //   simulator.hybrid.reboot_code_controller(0, result.hex);
    //   // simulator.pendulum_raised = false;
    // }
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
    runButton.innerHTML = "<span>🔨</span><span>Compile</span>";
    // stopButton.disabled = false;
  }
}

document.getElementById("closeOutput").addEventListener("click", async () => {
  document.getElementById("buildOutput").classList.remove("show");
});

// document.getElementById("stopButton").addEventListener("click", async () => {
//   reset_simulator(default_ino_bin, default_symbols);
// });

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
    "demo.ino",
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

/// Reset the simulator and controller
function reset_simulator(ino_bin: Uint8Array, symbols: string) {
  let simulator = getSimulator();
  simulator.hybrid.reset();
  let targets = [135, 190, 190, 80, 80, 190, 80, 80, 190];

  for (let i = 0; i < targets.length; i++) {
    simulator.hybrid.set_joint_q(i + 1, targets[i] * (Math.PI / 180)); // skip first floating joint
  }
  simulator.hybrid.reboot_esp32_controller(0, ino_bin, symbols);
}

setInterval(() => {
  let simulator = getSimulator();
  if (simulator) {
    const realtimeRatio = document.getElementById("realtimeRatio");
    realtimeRatio.innerHTML =
      "realtime rate: " + simulator.realtimeRatio.toFixed(2);
  }
}, 500);

// ===== Backflip Button Functionality ================= //
const backflipButton = document.getElementById("backflipButton");
async function sendBackflip() {
  const payload = "kbf\n";
  console.log("Sending backflip command: ", JSON.stringify(payload));

  let simulator = getSimulator();
  if (simulator && simulator.hybrid) {
    simulator.hybrid.send_uart(payload);
  }
}
backflipButton.addEventListener("click", sendBackflip);

// Trot Button
const trotButton = document.getElementById("trotButton");
async function sendTrot() {
  const payload = "ktrF\n";
  console.log("Sending trot command: ", JSON.stringify(payload));
  let simulator = getSimulator();
  if (simulator && simulator.hybrid) {
    simulator.hybrid.send_uart(payload);
  }
}
trotButton.addEventListener("click", sendTrot);

// Backward Button
const backwardButton = document.getElementById("backwardButton");
async function sendBackward() {
  const payload = "kbk\n";
  console.log("Sending backward command: ", JSON.stringify(payload));
  let simulator = getSimulator();
  if (simulator && simulator.hybrid) {
    simulator.hybrid.send_uart(payload);
  }
}
backwardButton.addEventListener("click", sendBackward);

// Be Table Button
const beTableButton = document.getElementById("beTableButton");
async function sendBeTable() {
  const payload = "ktbl\n";
  console.log("Sending be table command: ", JSON.stringify(payload));
  let simulator = getSimulator();
  if (simulator && simulator.hybrid) {
    simulator.hybrid.send_uart(payload);
  }
}
beTableButton.addEventListener("click", sendBeTable);
