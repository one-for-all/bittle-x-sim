import { openFile } from "./editor";
import { currentFile, files, renameFile } from "./files";
import { renderFileBar } from "./filebar";

interface FileNode {
  __isFile?: boolean;
  fullPath?: string;
  [key: string]: any;
}

export const expandedFolders = new Set<string>();

export function renderExplorer() {
  const explorerFiles = document.getElementById("explorerFiles");
  if (!explorerFiles) return;
  explorerFiles.innerHTML = "";

  const fileTree: FileNode = {};
  Object.keys(files).forEach((filename) => {
    const parts = filename.split("/");
    let currentLevel: FileNode = fileTree;
    for (let i = 0; i < parts.length; i++) {
      const part = parts[i];
      if (!currentLevel[part]) {
        currentLevel[part] =
          i === parts.length - 1 ? { __isFile: true, fullPath: filename } : {};
      }
      currentLevel = currentLevel[part] as FileNode;
    }
  });

  function createTreeNodes(
    tree: FileNode,
    container: HTMLElement,
    depth: number = 0,
    currentPath: string = "",
  ) {
    Object.keys(tree)
      .sort((a, b) => {
        const isFileA = tree[a].__isFile;
        const isFileB = tree[b].__isFile;
        if (isFileA && !isFileB) return -1;
        if (!isFileA && isFileB) return 1;
        return a.localeCompare(b);
      })
      .forEach((key) => {
        const node = tree[key] as FileNode;
        const div = document.createElement("div");
        div.className = node.__isFile ? "explorer-file" : "explorer-folder";
        div.style.paddingLeft = `${depth * 15 + 10}px`;
        div.textContent = (node.__isFile ? "📄 " : "📁 ") + key;

        const path = currentPath ? `${currentPath}/${key}` : key;

        if (node.__isFile) {
          if (node.fullPath === currentFile) div.classList.add("active");
          div.onclick = (e) => {
            e.stopPropagation();
            if (node.fullPath) openFile(node.fullPath);
          };
        } else {
          const folderContent = document.createElement("div");
          folderContent.style.display = expandedFolders.has(path)
            ? "block"
            : "none";
          div.onclick = (e) => {
            e.stopPropagation();
            if (expandedFolders.has(path)) {
              expandedFolders.delete(path);
              folderContent.style.display = "none";
            } else {
              expandedFolders.add(path);
              folderContent.style.display = "block";
            }
          };
          createTreeNodes(node, folderContent, depth + 1, path);
          container.appendChild(div);
          container.appendChild(folderContent);
        }

        div.oncontextmenu = (e) => {
          e.preventDefault();
          e.stopPropagation();
          const newName = prompt(
            `Rename ${node.__isFile ? "file" : "folder"}:`,
            key,
          );
          if (newName && newName !== key) {
            if (node.__isFile) {
              if (
                renameFile(path, path.replace(new RegExp(key + "$"), newName))
              ) {
                renderExplorer();
                renderFileBar();
              }
            } else {
              const oldPathPrefix = path + "/";
              const newPath = path.replace(new RegExp(key + "$"), newName);
              const newPathPrefix = newPath + "/";
              Object.keys(files).forEach((f) => {
                if (f.startsWith(oldPathPrefix)) {
                  const newFilePath = f.replace(oldPathPrefix, newPathPrefix);
                  files[newFilePath] = files[f];
                  delete files[f];
                  if (expandedFolders.has(path)) {
                    expandedFolders.delete(path);
                    expandedFolders.add(newPath);
                  }
                }
              });
              renderExplorer();
              renderFileBar();
            }
          }
        };

        if (node.__isFile) container.appendChild(div);
      });
  }

  createTreeNodes(fileTree, explorerFiles);
}

renderExplorer();

document
  .getElementById("toggleExplorerButton")!
  .addEventListener("click", () => {
    document.getElementById("explorer")!.classList.toggle("hidden");
    window.dispatchEvent(new Event("resize"));
  });
