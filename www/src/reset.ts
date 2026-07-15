import { getSimulator, cameraPosition } from "./sim";
import { setupResetButton, setupModeToggleButton } from "chimpanzee-ui";

setupResetButton(
  { getSimulator },
  (hybrid) => {
    // Initial pose
    let targets = [135, 190, 190, 80, 80, 190, 80, 80, 190];
    for (let i = 0; i < targets.length; i++) {
      hybrid.set_joint_q(i + 1, targets[i] * (Math.PI / 180)); // skip first floating joint
    }

    hybrid.reboot_code_controller(0, "");
  },
  // cameraPosition, // optionally reset camera
);

setupModeToggleButton();
