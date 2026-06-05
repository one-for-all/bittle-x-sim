const queryString = window.location.search;
const urlParams = new URLSearchParams(queryString);
const mode = urlParams.get("mode");

export const isControlMode = (): boolean => {
  return mode === "control";
};

const controlModeHideElements: string[] = [
  "projectSidebar",
  "explorer",
  "editorContainer",
  "buildOutput",
  "serialInputBar",
];
if (isControlMode()) {
  for (const element of controlModeHideElements) {
    document.getElementById(element).style.display = "none";
  }
}
