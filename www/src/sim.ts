import { createBittleX } from "bittle-x";
import { Simulator } from "gorilla-physics-ui";
import { GLTFLoader } from "three/examples/jsm/loaders/GLTFLoader.js";
import { Euler, Vector3 } from "three";

let _simulator: Simulator | null = null;

export function initSimulator() {
  createBittleX().then((state) => {
    let interfaceSimulator = null;
    let showGrid = false;
    let simulator = new Simulator(interfaceSimulator, showGrid);
    simulator.showHalfspaces = false;

    let scenes = [
      {
        scenePath: "gamer_setup_pack.glb",
        rotation: new Euler(Math.PI / 2),
        position: new Vector3(0.6, 0, -0.82),
      },
      // {
      //   scenePath: "cozy_living_room_baked.glb",
      //   rotation: new Euler(Math.PI / 2, -Math.PI / 2, 0),
      //   position: new Vector3(0, 0, -0.88),
      // },
    ];
    let gltfLoader = new GLTFLoader();
    for (const scene of scenes) {
      gltfLoader.load(
        scene.scenePath,
        (gltf) => {
          const gltfScene = gltf.scene;
          gltfScene.setRotationFromEuler(scene.rotation);
          (gltfScene.position.set(
            scene.position.x,
            scene.position.y,
            scene.position.z,
          ),
            simulator.graphics.scene.add(gltfScene));
        },
        undefined,
        (error) => {
          console.error(error);
        },
      );
    }

    simulator.addHybrid(state);
    simulator.updateHybrid();

    let cameraPosition = {
      eye: { x: 0.0, y: -1.2, z: 0.4 },
      target: { x: 0.0, y: 0, z: 0 },
    };
    simulator.graphics.lookAt(cameraPosition);

    simulator.run(60, 0); // 10

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
