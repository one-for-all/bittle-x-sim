import { initSimulator } from "./sim";
import "./editor";
import "./motion_button";
import "./serial_monitor";
import "./resize";
import "./wasd";
import { files, inoFileName, resetFiles } from "./files";
import { openFile } from "./editor";
import "./compile";
import { renderExplorer } from "./explorer";
import { renderFileBar } from "./filebar";

initSimulator();
openFile(inoFileName());

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
