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
openFile(Object.keys(files)[0]);

document.getElementById("newProject")!.addEventListener("click", () => {
  resetFiles();
  renderExplorer();
  renderFileBar();
  openFile("main.ino");
});
