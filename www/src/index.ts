import { initSimulator } from "./sim";
import "./editor";
import "./motion_button";
import "./resize";
import "./wasd";
import { files, inoFileName, resetFiles } from "./files";
import { openFile } from "./editor";
import "./compile";
import "./reset";
import { renderExplorer } from "./explorer";
import { renderFileBar } from "./filebar";
import { setupDownload } from "./download";
import { getSimulator } from "./sim";
import { createSerialMonitorPanel } from "chimpanzee-ui";
import { updateUIforMode, isPhoneUA } from "chimpanzee-ui";

// Import CSS
import "chimpanzee-ui/index.css";

initSimulator();
const panel = createSerialMonitorPanel({ getSimulator });
openFile(inoFileName());
setupDownload();
updateUIforMode();

if (isPhoneUA()) {
  document.getElementById("explorer").classList.add("hidden");
  document.getElementById("editorContainer").style.flex = `0 0 10px`;
}

document
  .getElementById("projectDialogButton")!
  .addEventListener("click", () => {
    const dialog = document.getElementById("projectDialog")!;
    dialog.classList.toggle("hidden");
  });

document.getElementById("createProjectBtn")!.addEventListener("click", () => {
  resetFiles();
  renderExplorer();
  renderFileBar();
  openFile("main.ino");
  document.getElementById("projectDialog")!.classList.add("hidden");
});

document.getElementById("newFileBtn")!.addEventListener("click", () => {
  const filename = "untitled";
  files[filename] = {
    content: "",
    language: "cpp",
  };
  renderExplorer();
  renderFileBar();
  openFile(filename);
  document.getElementById("projectDialog")!.classList.add("hidden");
});
