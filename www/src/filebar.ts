import { openFile } from "./editor";
import { currentFile, files } from "./files";

const icons: Record<string, string> = {
  ino: "📝",
  h: "📝",
  md: "📋",
};

function getFileIcon(filename: string) {
  const ext = filename.split(".").pop();
  return icons[ext] || "📄";
}

export function renderFileBar() {
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

renderFileBar();
