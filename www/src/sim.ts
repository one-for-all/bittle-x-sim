import { createBittleX } from "bittle-x";
import { Simulator } from "gorilla-physics-ui";
import { GLTFLoader } from "three/examples/jsm/loaders/GLTFLoader.js";

let _simulator: Simulator | null = null;

export function initSimulator() {
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

    setInterval(() => {
      const realtimeRatio = document.getElementById("realtimeRatio");
      realtimeRatio.innerHTML =
        "realtime rate: " + simulator.realtimeRatio.toFixed(2);
    }, 500);
  });
}

function setSimulator(sim: Simulator) {
  _simulator = sim;
}

export function getSimulator(): Simulator | null {
  return _simulator;
}

/// Reset the simulator and controller
export function reset_simulator(ino_bin: Uint8Array, symbols: string) {
  let simulator = getSimulator();
  simulator.hybrid.reset();
  let targets = [135, 190, 190, 80, 80, 190, 80, 80, 190];

  for (let i = 0; i < targets.length; i++) {
    simulator.hybrid.set_joint_q(i + 1, targets[i] * (Math.PI / 180)); // skip first floating joint
  }
  simulator.hybrid.reboot_esp32_controller(0, ino_bin, symbols);
}
