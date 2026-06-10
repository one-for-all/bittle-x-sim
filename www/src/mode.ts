const queryString = window.location.search;
const urlParams = new URLSearchParams(queryString);
let mode = urlParams.get("mode");

export const isControlMode = (): boolean => {
  return mode === "control";
};

function toggleMode() {
  if (isControlMode()) {
    mode = "code";
  } else {
    mode = "control";
  }
  updateUIforMode();
}

const controlModeHideElements: string[] = [
  "projectSidebar",
  "explorer",
  "editorContainer",
  "buildOutput",
  "serialInputBar",
];

function updateUIforMode() {
  for (const element of controlModeHideElements) {
    if (isControlMode()) {
      document.getElementById(element).classList.add("control-mode");
    } else {
      document.getElementById(element).classList.remove("control-mode");
    }
  }
}

updateUIforMode();
