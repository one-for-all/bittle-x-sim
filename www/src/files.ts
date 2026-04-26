import readme from "./assets/README.md";
import default_ino from "./assets/default.ino";

type FileEntry = {
  content: string;
  language: string;
  scrollTop?: number; // Added for scroll position
  scrollLeft?: number; // Added for scroll position
};

export function resetFiles() {
  files = {
    "main.ino": {
      content: default_ino,
      language: "cpp",
    },
  };
  currentFile = ""; // set to empty current file so editor will reset on open
}

export let files: Record<string, FileEntry> = {
  "README.md": {
    content: readme,
    language: "markdown",
  },
};
export let currentFile: string = null;
export function setCurrentFile(filename: string) {
  currentFile = filename;
}

initFiles();

function initFiles() {
  // 1. Create the Webpack context
  // Arguments: (Directory, Search Subdirectories?, Regex to match files)
  const filesContext = require.context(
    "./assets/OpenCatEsp32",
    true,
    /\.(cpp|c|h|hpp|ino)$/i,
  );

  // 2. Iterate through all the matched files
  filesContext.keys().forEach((key) => {
    // 'key' looks like: "./main.cpp" or "./utils/config.h"
    // filesContext(key) returns the raw string content (thanks to your asset/source rule)
    const fileContent = filesContext(key);

    // add to files record
    files[key.replace(/^\.\//, "")] = {
      content: fileContent,
      language: "cpp",
    };
  });
}

export function inoFileName() {
  let ino_file_names = Object.keys(files).filter((key) => key.endsWith(".ino"));
  if (ino_file_names.length != 1) {
    alert("Need to have one and only one .ino file");
  }
  return ino_file_names[0];
}
