import { getSimulator } from "./sim";

// ===== Backflip Button Functionality ================= //
const backflipButton = document.getElementById("backflipButton");
async function sendBackflip() {
  const payload = "kbf\n";
  console.log("Sending backflip command: ", JSON.stringify(payload));

  let simulator = getSimulator();
  if (simulator && simulator.hybrid) {
    simulator.hybrid.send_uart(payload);
  }
}
backflipButton.addEventListener("click", sendBackflip);

// Trot Button
const trotButton = document.getElementById("trotButton");
async function sendTrot() {
  const payload = "ktrF\n";
  console.log("Sending trot command: ", JSON.stringify(payload));
  let simulator = getSimulator();
  if (simulator && simulator.hybrid) {
    simulator.hybrid.send_uart(payload);
  }
}
trotButton.addEventListener("click", sendTrot);

// Backward Button
const backwardButton = document.getElementById("backwardButton");
async function sendBackward() {
  const payload = "kbk\n";
  console.log("Sending backward command: ", JSON.stringify(payload));
  let simulator = getSimulator();
  if (simulator && simulator.hybrid) {
    simulator.hybrid.send_uart(payload);
  }
}
backwardButton.addEventListener("click", sendBackward);

// Be Table Button
const beTableButton = document.getElementById("beTableButton");
async function sendBeTable() {
  const payload = "ktbl\n";
  console.log("Sending be table command: ", JSON.stringify(payload));
  let simulator = getSimulator();
  if (simulator && simulator.hybrid) {
    simulator.hybrid.send_uart(payload);
  }
}
beTableButton.addEventListener("click", sendBeTable);

// Kick button
const kickButton = document.getElementById("kickButton");
async function kick() {
  const payload = "kkc\n";
  console.log("Sending be table command: ", JSON.stringify(payload));
  let simulator = getSimulator();
  if (simulator && simulator.hybrid) {
    simulator.hybrid.send_uart(payload);
  }
}
kickButton.addEventListener("click", kick);
