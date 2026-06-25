// Import CSS
import "chimpanzee-ui/css";

import { initSimulator } from "./sim";
import "./editor";
import "./motion_button";
import "./wasd";
import { files, inoFileName, resetFiles } from "./files";
import { openFile } from "./editor";
import "./compile";
import "./reset";
import { renderExplorer } from "./explorer";
import { renderFileBar } from "./filebar";
import { setupDownload } from "./download";
import { getSimulator } from "./sim";
import { createSerialMonitorPanel, setupResize } from "chimpanzee-ui";
import { updateUIforMode, isPhoneUA } from "chimpanzee-ui";

if (isPhoneUA()) {
  document.getElementById("explorer").classList.add("hidden");
  document.getElementById("editorContainer").style.flex = `0 0 10px`;
}

updateUIforMode();
setupResize();

initSimulator();
const panel = createSerialMonitorPanel({ getSimulator });
openFile(inoFileName());
setupDownload();

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
