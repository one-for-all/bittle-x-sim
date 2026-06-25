import { getSimulator } from "./sim";
import { toggleMode } from "chimpanzee-ui";

document.getElementById("resetButton").addEventListener("click", () => {
  const simulator = getSimulator();
  if (simulator?.hybrid) {
    simulator.hybrid.reset();

    // Initial pose
    let targets = [135, 190, 190, 80, 80, 190, 80, 80, 190];
    for (let i = 0; i < targets.length; i++) {
      simulator.hybrid.set_joint_q(i + 1, targets[i] * (Math.PI / 180)); // skip first floating joint
    }

    simulator.hybrid.reboot_code_controller(0, "");
  }
});

document.getElementById("toggleModeButton").addEventListener("click", () => {
  toggleMode();
  document.getElementById("projectDialog")?.classList.add("hidden");
});
