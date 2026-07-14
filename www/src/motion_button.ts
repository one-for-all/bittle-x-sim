import { getSimulator } from "./sim";

function createMotionButton(payload: string, label: string) {
  const container = document.getElementById("movementButtonContainer");
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
  { payload: "kvtF", label: "Step" },
  { payload: "kvtL", label: "SpinLeft" },
  { payload: "kvtR", label: "SpinRight" },
  { payload: "kwkF", label: "WalkForward" },
  { payload: "kwkL", label: "WalkLeft" },
  { payload: "kwkR", label: "WalkRight" },
  { payload: "kbkF", label: "Back" },
  { payload: "kbkL", label: "BackLeft" },
  { payload: "kbkR", label: "BackRight" },
  { payload: "ktrF", label: "TrotForward" },
  { payload: "ktrL", label: "TrotLeft" },
  { payload: "ktrR", label: "TrotRight" },
  { payload: "kcrF", label: "CrawlForward" },
  { payload: "kcrL", label: "CrawlLeft" },
  { payload: "kcrR", label: "CrawlRight" },
  { payload: "kgpF", label: "StrideForward" },
  { payload: "kgpL", label: "StrideLeft" },
  { payload: "kgpR", label: "StrideRight" },
  { payload: "kphF", label: "PushForward" },
  { payload: "kphL", label: "PushLeft" },
  { payload: "kphR", label: "PushRight" },
  { payload: "kmw", label: "Moonwalk" },
  { payload: "kup", label: "Stand" },
  { payload: "kbalance", label: "Balance" },
  { payload: "ksit", label: "Sit" },
  { payload: "kstr", label: "Stretch" },
  { payload: "kbuttUp", label: "ButtUp" },
  { payload: "kcalib", label: "Calibration" },
  { payload: "khds", label: "Handstand" },
  { payload: "kbx", label: "Boxing" },
  { payload: "kflipD", label: "Backflip" },
  { payload: "kflipF", label: "Frontflip" },
  { payload: "kjmp", label: "Jump" },
  { payload: "khi", label: "Hi" },
  { payload: "khsk", label: "Handshake" },
  { payload: "kfiv", label: "HighFive" },
  { payload: "khg", label: "Hug" },
  { payload: "khu", label: "HandsUp" },
  { payload: "knd", label: "Nod" },
  { payload: "kcmh", label: "ComeHere" },
  { payload: "kgdb", label: "GoodBoy" },
  { payload: "kchr", label: "Cheers" },
  { payload: "kpee", label: "Pee" },
  { payload: "ksnf", label: "Sniff" },
  { payload: "kck", label: "Check" },
  { payload: "kdg", label: "Dig" },
  { payload: "kang", label: "Angry" },
  { payload: "kscrh", label: "Scratch" },
  { payload: "kwh", label: "WaveHead" },
  { payload: "ktbl", label: "BeTable" },
  { payload: "kpd", label: "PlayDead" },
  { payload: "krl", label: "Roll" },
  { payload: "krc", label: "Recover" },
  { payload: "kpu", label: "PushUp" },
  { payload: "kpu1", label: "PushUp1Hand" },
  { payload: "kkc", label: "Kick" },
  { payload: "klpov", label: "LeapOver" },
  { payload: "kts", label: "TestServo" },
];

for (const { payload, label } of motions) {
  createMotionButton(payload, label);
}
