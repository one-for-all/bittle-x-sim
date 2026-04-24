import demo_ino from "./assets/OpenCatEsp32.ino";
import readme from "./assets/README.md";

type FileEntry = {
  content: string;
  language: string;
  scrollTop?: number; // Added for scroll position
  scrollLeft?: number; // Added for scroll position
};

export let files: Record<string, FileEntry> = {
  "demo.ino": {
    content: demo_ino,
    language: "cpp",
  },
  "README.md": {
    content: readme,
    language: "markdown",
  },
};

export let currentFile: string = null;

function initFiles() {
  if (initialized) return;
  initialized = true;

  // 1. Create the Webpack context
  // Arguments: (Directory, Search Subdirectories?, Regex to match files)
  const filesContext = require.context(
    "./assets/src",
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

export function setCurrentFile(file: string) {
  currentFile = file;
}

let initialized = false;
initFiles();
