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

const motions = [
  { id: "backflipButton", payload: "kbf", label: "backflip" },
  { id: "trotButton", payload: "ktrF", label: "trot" },
  { id: "backwardButton", payload: "kbk", label: "backward" },
  { id: "beTableButton", payload: "ktbl", label: "be table" },
  { id: "kickButton", payload: "kkc", label: "kick" },
  { id: "recoverButton", payload: "krc", label: "recover" },
];

for (const { id, payload, label } of motions) {
  setupMotionButton(id, payload, label);
}
