import { getSimulator } from "./sim";

function setupMotionButton(buttonId: string, payload: string, label: string) {
  const button = document.getElementById(buttonId);
  if (!button) return;

  button.addEventListener("click", () => {
    console.log(`Sending ${label} command:`, JSON.stringify(payload));
    const simulator = getSimulator();
    if (simulator?.hybrid) {
      simulator.hybrid.send_uart(payload);
    }
  });
}

setupMotionButton("backflipButton", "kbf\n", "backflip");
setupMotionButton("trotButton", "ktrF\n", "trot");
setupMotionButton("backwardButton", "kbk\n", "backward");
setupMotionButton("beTableButton", "ktbl\n", "be table");
setupMotionButton("kickButton", "kkc\n", "kick");
setupMotionButton("recoverButton", "krc\n", "recover");
