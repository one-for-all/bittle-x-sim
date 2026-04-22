import { createBittleX } from "bittle-x";
import { Simulator } from "gorilla-physics-ui";
import { GLTFLoader } from "three/examples/jsm/loaders/GLTFLoader.js";

import("bittle-x").then((furuta) => {
  createBittleX().then((state) => {
    let interfaceSimulator = null;
    let showGrid = false;
    let simulator = new Simulator(interfaceSimulator, showGrid);
    simulator.showCollisionVisual = false;
    simulator.showHalfspaces = false;

    let scenePath = "gamer_setup_pack.glb";
    let gltfLoader = new GLTFLoader();
    gltfLoader.load(
      scenePath,
      (gltf) => {
        const scene = gltf.scene;
        scene.rotation.x = Math.PI / 2;
        scene.position.z -= 0.82;
        scene.position.x += 0.6;
        simulator.graphics.scene.add(scene);
      },
      undefined,
      (error) => {
        console.error(error);
      },
    );

    simulator.addHybrid(state);
    simulator.updateHybrid();

    let cameraPosition = {
      eye: { x: 0.0, y: -1.2, z: 0.4 },
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
