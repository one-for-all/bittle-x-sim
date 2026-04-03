import { createBittleX } from "bittle-x";
import { Simulator } from "gorilla-physics-ui";

import("bittle-x").then((furuta) => {
  createBittleX().then((state) => {
    let interfaceSimulator = null;
    let simulator = new Simulator(interfaceSimulator);

    simulator.addHybrid(state);
    simulator.updateHybrid();

    let cameraPosition = {
      eye: { x: 0.5, y: 0, z: 0.1 },
      target: { x: 0.0, y: 0, z: 0 },
    };
    simulator.graphics.lookAt(cameraPosition);

    simulator.run(100, 0); // 10

    setSimulator(simulator);
  });
});

let _simulator: Simulator | null = null;

function setSimulator(sim: Simulator) {
  _simulator = sim;
}

export function getSimulator(): Simulator | null {
  return _simulator;
}
