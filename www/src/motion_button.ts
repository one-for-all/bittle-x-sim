import { getSimulator } from "./sim";

const container = document.getElementById("movementButtonContainer");

function createMotionButton(payload: string, label: string) {
  if (!container) return;

  const button = document.createElement("button");
  button.className = "movement-button";
  button.textContent = label;

  button.addEventListener("click", () => {
    console.log(`Sending ${label} command:`, JSON.stringify(payload));
    const simulator = getSimulator();
    if (simulator?.hybrid) {
      simulator.hybrid.send_uart(payload + "\n");
    }
  });

  container.appendChild(button);
}

const motions = [
  { payload: "kbf", label: "backflip" },
  { payload: "ktrF", label: "trot" },
  { payload: "kbk", label: "backward" },
  { payload: "ktbl", label: "be table" },
  { payload: "kkc", label: "kick" },
  { payload: "krc", label: "recover" },
];

for (const { payload, label } of motions) {
  createMotionButton(payload, label);
}
