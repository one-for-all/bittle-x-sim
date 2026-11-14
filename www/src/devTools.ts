import { bittleXTipOver } from "bittle-x";
import { getSimulator } from "./sim";

// Buttons for testing the robot in situations that are slow to set up by
// driving it there. Only on the dev server, see DEV_TOOLS in rspack.config.js.
export function setupDevTools() {
  const container = document.getElementById("movementButtonContainer");
  if (!container) return;

  // Tips the robot over with the firmware still running, to test its IMU
  // auto-recovery (the "rc" skill, triggered by dealWithExceptions())
  for (const { roll, label } of [
    { roll: 90, label: "Lay on Left Side (dev)" },
    { roll: -90, label: "Lay on Right Side (dev)" },
    { roll: 180, label: "Flip on Back (dev)" },
  ]) {
    const button = document.createElement("button");
    button.className = "movement-button";
    button.textContent = label;
    button.addEventListener("click", () => {
      const hybrid = getSimulator()?.hybrid;
      if (hybrid) {
        bittleXTipOver(hybrid, roll);
      }
    });
    container.appendChild(button);
  }
}
