const mainResizeHandle = document.getElementById("mainResizeHandle")!;
const editorContainer = document.querySelector(
  ".editor-container",
) as HTMLElement;

mainResizeHandle.addEventListener("pointerdown", (e) => {
  e.preventDefault();
  document.addEventListener("pointermove", resizeMain);
  document.addEventListener("pointerup", stopResizeMain);
});

function resizeMain(e: PointerEvent) {
  const containerRect = editorContainer.getBoundingClientRect();
  const newWidth = e.clientX - containerRect.left;
  const maxWidth = window.innerWidth * 0.8; // Prevent container from taking more than 80%

  if (newWidth > 10 && newWidth < maxWidth) {
    editorContainer.style.flex = `0 0 ${newWidth}px`;
  }

  window.dispatchEvent(new Event("resize"));
}

function stopResizeMain() {
  document.removeEventListener("pointermove", resizeMain);
  document.removeEventListener("pointerup", stopResizeMain);
}
