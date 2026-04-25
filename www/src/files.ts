import main_ino from "./assets/OpenCatEsp32.ino";
import readme from "./assets/README.md";

type FileEntry = {
  content: string;
  language: string;
  scrollTop?: number; // Added for scroll position
  scrollLeft?: number; // Added for scroll position
};

export function resetFiles() {
  files = {
    "main.ino": {
      content: `void setup() {\n  // put your setup code here, to run once:\n}\n\nvoid loop() {\n  // put your main code here, to run repeatedly:\n}\n`,
      language: "cpp",
    },
  };
  currentFile = ""; // set to empty current file so editor will reset on open
}

export let files: Record<string, FileEntry> = {
  "main.ino": {
    content: main_ino,
    language: "cpp",
  },
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
