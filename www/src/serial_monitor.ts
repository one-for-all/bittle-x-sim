import { getSimulator } from "./sim";

// Show the output div at the beginning
const outputDiv = document.getElementById("buildOutput");
outputDiv.classList.add("show");

document.getElementById("closeOutput").addEventListener("click", async () => {
  document.getElementById("buildOutput").classList.remove("show");
});

// Update serial monitor message periodically
const serialMonitor = document.getElementById("serialMonitor");
setInterval(() => {
  let simulator = getSimulator();
  if (simulator && simulator.hybrid) {
    serialMonitor.textContent = simulator.hybrid.get_uart();
  }
}, 100); // update every 100 ms

// ===== Serial Monitor Input ================= //
const serialInput = document.getElementById("serialInput") as HTMLInputElement;
async function sendSerialData() {
  const text = serialInput.value;
  if (!text) return;

  const payload = text + "\n";
  console.log("serial tx: ", JSON.stringify(payload));
  serialInput.value = "";

  let simulator = getSimulator();
  if (simulator && simulator.hybrid) {
    simulator.hybrid.send_uart(payload);
  }
}
serialInput.addEventListener("keydown", (e) => {
  if (e.key === "Enter") sendSerialData();
});

document.getElementById("serialSend").addEventListener("click", sendSerialData);
