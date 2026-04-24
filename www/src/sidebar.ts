import { openFile } from "./editor";
import { currentFile, files } from "./files";

export function renderSidebar() {
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

renderSidebar();

document.getElementById("toggleSidebar")!.addEventListener("click", () => {
  document.getElementById("sidebar")!.classList.toggle("hidden");
  window.dispatchEvent(new Event("resize"));
});
