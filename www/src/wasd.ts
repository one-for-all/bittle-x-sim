import { getSimulator } from "./sim";

const wasdContainer = document.getElementById("wasdContainer");
const buildOutput = document.getElementById("buildOutput");

// Function to update WASD position based on build output visibility
function updateWasdPosition() {
  if (buildOutput.classList.contains("show")) {
    wasdContainer.classList.add("expanded");
  } else {
    wasdContainer.classList.remove("expanded");
  }
}

// Watch for changes to buildOutput class
const observer = new MutationObserver(updateWasdPosition);
observer.observe(buildOutput, { attributes: true, attributeFilter: ["class"] });

// Initial check
updateWasdPosition();

// WASD Command Mapping
const commands: Record<string, string> = {
  btnW: "kphF",
  btnA: "kvtL",
  btnS: "kbk",
  btnD: "kvtR",
};

// Map keyboard keys to command IDs
const keyMap: Record<string, string> = {
  w: "btnW",
  a: "btnA",
  s: "btnS",
  d: "btnD",
};

function sendCommand(id: string) {
  const payload = commands[id] + "\n";
  console.log("Sending WASD command: ", JSON.stringify(payload));

  let simulator = getSimulator();
  if (simulator && simulator.hybrid) {
    simulator.hybrid.send_uart(payload);
  }
}

Object.keys(commands).forEach((id) => {
  document.getElementById(id).addEventListener("click", () => sendCommand(id));
});

// Keyboard event listener
document.addEventListener("keydown", (e) => {
  // Ignore keydown if user is typing in the serial monitor input
  if (document.activeElement?.id === "serialInput") return;

  const key = e.key.toLowerCase();
  if (keyMap[key]) {
    sendCommand(keyMap[key]);
  }
});
