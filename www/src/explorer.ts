import { openFile } from "./editor";
import { currentFile, files } from "./files";

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
    explorerFiles.appendChild(fileDiv);
  });
}

renderExplorer();

document.getElementById("toggleExplorer")!.addEventListener("click", () => {
  document.getElementById("explorer")!.classList.toggle("hidden");
  window.dispatchEvent(new Event("resize"));
});
