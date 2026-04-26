import { openFile } from "./editor";
import { currentFile, files, renameFile } from "./files";
import { renderFileBar } from "./filebar";

export function renderExplorer() {
  const explorerFiles = document.getElementById("explorerFiles");
  if (!explorerFiles) return;
  explorerFiles.innerHTML = "";

  Object.keys(files).forEach((filename) => {
    const fileDiv = document.createElement("div");
    fileDiv.className = "explorer-file";
    if (filename === currentFile) {
      fileDiv.classList.add("active");
    }
    fileDiv.textContent = filename;
    fileDiv.onclick = () => openFile(filename);

    fileDiv.oncontextmenu = (e) => {
      e.preventDefault();
      const newName = prompt("Rename file:", filename);
      if (newName && newName !== filename) {
        if (renameFile(filename, newName)) {
          renderExplorer();
          renderFileBar();
          if (currentFile === newName) {
            // Re-open/refresh editor if needed
            openFile(newName);
          }
        }
      }
    };

    explorerFiles.appendChild(fileDiv);
  });
}

renderExplorer();

document
  .getElementById("toggleExplorerButton")!
  .addEventListener("click", () => {
    document.getElementById("explorer")!.classList.toggle("hidden");
    window.dispatchEvent(new Event("resize"));
  });
