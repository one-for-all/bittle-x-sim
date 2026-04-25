import * as monaco from "monaco-editor";
import { currentFile, files, setCurrentFile } from "./files";
import { renderFileBar } from "./filebar";
import { renderExplorer } from "./explorer";

export const editor = monaco.editor.create(document.getElementById("editor")!, {
  language: "cpp",
  theme: "vs-dark",
  automaticLayout: true,
  fontSize: 14,
  minimap: { enabled: false },
});

editor.onDidChangeModelContent(() => {
  if (currentFile && files[currentFile]) {
    files[currentFile].content = editor.getValue();
  }
});

export function openFile(filename: string) {
  if (currentFile && files[currentFile]) {
    files[currentFile].content = editor.getValue();
    // Save scroll position before switching
    files[currentFile].scrollTop = editor.getScrollTop();
    files[currentFile].scrollLeft = editor.getScrollLeft();
  }

  setCurrentFile(filename);

  const file = files[filename];
  monaco.editor.setModelLanguage(editor.getModel(), file.language);
  editor.setValue(file.content);

  // Restore scroll position for the new file
  editor.setScrollPosition({
    scrollTop: file.scrollTop || 0,
    scrollLeft: file.scrollLeft || 0,
  });

  renderFileBar();
  renderExplorer();
}
