import { initSimulator } from "./sim";
import "./editor";
import "./motion_button";
import "./serial_monitor";
import "./resize";
import { files, resetFiles } from "./files";
import { openFile } from "./editor";
import "./compile";
import { renderExplorer } from "./explorer";
import { renderFileBar } from "./filebar";

initSimulator();
openFile("OpenCatEsp32.ino");

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
  openFile("OpenCatEsp32.ino");
  document.getElementById("projectDialog")!.classList.add("hidden");
});
