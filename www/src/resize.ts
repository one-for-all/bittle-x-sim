const mainResizeHandle = document.getElementById("mainResizeHandle")!;
const editorContainer = document.querySelector(
  ".editor-container",
) as HTMLElement;

mainResizeHandle.addEventListener("mousedown", (e) => {
  e.preventDefault();
  document.addEventListener("mousemove", resizeMain);
  document.addEventListener("mouseup", stopResizeMain);
});

function resizeMain(e: MouseEvent) {
  const containerRect = editorContainer.getBoundingClientRect();
  const newWidth = e.clientX - containerRect.left;
  const maxWidth = window.innerWidth * 0.8; // Prevent container from taking more than 80%

  if (newWidth > 200 && newWidth < maxWidth) {
    editorContainer.style.flex = `0 0 ${newWidth}px`;
  }

  window.dispatchEvent(new Event("resize"));
}

function stopResizeMain() {
  document.removeEventListener("mousemove", resizeMain);
  document.removeEventListener("mouseup", stopResizeMain);
}
