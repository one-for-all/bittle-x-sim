# Bittle X UI

This is the UI subcomponent for the Bittle X Robot Simulation project, responsible for providing the web-based interactive simulation environment.

## Overview

The Bittle X UI is a TypeScript-based web application that integrates:
*   **3D Visualization:** Uses `Three.js` to render the Bittle X robot based on its URDF model.
*   **Interactive Code Editor:** Built with `Monaco Editor` for editing and compiling Arduino-based control code.
*   **Simulation Engine:** Communicates with the Rust-compiled Wasm backend (`bittle-x`) to drive the robot physics simulation.
*   **Firmware Management:** Allows users to compile code and interact with a simulated ESP32 controller.

## Development

The project uses [Rspack](https://rspack.dev/) for high-performance bundling.

### Build and Run

1.  **Dependencies:** Ensure you have installed the project dependencies:
    ```bash
    npm install
    ```
2.  **Start Development Server:**
    ```bash
    npm run start
    ```
    This bundles the application using Rspack and starts the development server.

### Scripts

*   `npm run build`: Bundles the project to the `docs/` directory using Rspack.
*   `npm run start`: Starts the development server.
*   `npm run prod`: Starts the development server with production settings.

GPU-accelerated builds are supported by setting the `FEATURE_GPU` environment variable (e.g., `npm run start:gpu`).

## Key Dependencies

*   **Bundler:** Rspack
*   **Language:** TypeScript
*   **Editor:** Monaco Editor
*   **3D Rendering:** Three.js
*   **Physics:** Gorilla Physics (UI integration)
