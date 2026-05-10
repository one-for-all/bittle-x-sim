import { getSimulator } from "./sim";

function setupMotionButton(buttonId: string, payload: string, label: string) {
  const button = document.getElementById(buttonId);
  if (!button) return;

  button.addEventListener("click", () => {
    console.log(`Sending ${label} command:`, JSON.stringify(payload));
    const simulator = getSimulator();
    if (simulator?.hybrid) {
      simulator.hybrid.send_uart(payload + "\n");
    }
  });
}

setupMotionButton("backflipButton", "kbf", "backflip");
setupMotionButton("trotButton", "ktrF", "trot");
setupMotionButton("backwardButton", "kbk", "backward");
setupMotionButton("beTableButton", "ktbl", "be table");
setupMotionButton("kickButton", "kkc", "kick");
setupMotionButton("recoverButton", "krc", "recover");
