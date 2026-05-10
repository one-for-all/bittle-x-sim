export const isPhoneUA = (): boolean => {
  const ua = navigator.userAgent;
  return /Android|webOS|iPhone|iPod|BlackBerry|IEMobile|Opera Mini/i.test(ua);
};

if (isPhoneUA()) {
  document.getElementById("explorer").classList.add("hidden");
  document.getElementById("editorContainer").style.flex = `0 0 10px`;
}
