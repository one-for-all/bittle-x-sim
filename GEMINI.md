# Bittle X Robot Simulation

This is an end-to-end simulation environment for Petoi's Bittle X Robot Dog, enabling users to experiment with code, control, and visualize robot behavior.

## Project Overview

The Bittle X Robot Simulation project is a hybrid system designed to provide a comprehensive development and testing platform for the Bittle X Robot Dog. It integrates a Rust backend, capable of targeting native, WebAssembly (for the web UI), and ESP32 firmware, with a rich web frontend built using TypeScript, rspack, and Three.js for 3D visualization. The robot's kinematic and visual properties are defined using the URDF format.

## Technologies Used

*   **Backend/Simulation Engine:** Rust (with `wasm-bindgen` for Wasm compilation)
*   **Web Frontend:** TypeScript, rspack, Three.js, Monaco Editor
*   **Robot Description:** URDF (Unified Robot Description Format)
*   **Physics Engine:** `gorilla-physics`
*   **Embedded Target:** ESP32 microcontroller (via Arduino framework and `esp32rs` crate)
*   **Build Tools:** `cargo` (Rust), `npm`/`rspack` (Frontend), `arduino-cli` (ESP32 Firmware)

## Building and Running

### Web Frontend Development

1.  **Install Dependencies:**
    ```bash
    npm install
    ```
2.  **Start Development Server:**
    ```bash
    npm run start
    ```
    This command will bundle the Rust WebAssembly code and serve the web application using Rspack. Open the provided URL in your browser to access the simulation.
3.  **GPU-Accelerated Features:**
    To enable optional GPU acceleration:
    ```bash
    npm run start:gpu
    ```

### Rust Backend / WebAssembly Compilation

The Rust library (`bittle-x`) is compiled to WebAssembly for use in the web frontend. This is typically handled by the frontend build process when `npm install` is run and `bittle-x` is included as a dependency.

*   **Native Compilation:**
    ```bash
    cargo build
    ```
*   **Wasm Compilation (typically handled by frontend build):**
    ```bash
    wasm-pack build --target web
    ```

### ESP32 Firmware Compilation

The `build-open-cat.sh` script automates the compilation and packaging of Arduino sketches for the ESP32 microcontroller.

1.  **Execute the build script:**
    ```bash
    ./build-open-cat.sh
    ```
    This script uses `arduino-cli` and `xtensa-esp32-elf-nm` to compile firmware, extract symbols, and generate binary files. It then copies these artifacts, along with ROM files and source code, into the `www/static/` and `www/src/assets/` directories, making them available to the web UI.

### Running the Simulation

*   **Via Web UI:** Start the development server (`npm run start`) and access the simulation through your web browser. The UI provides controls to load code, compile, run the simulation, and view serial output.
*   **Via Native Binary:** Execute the Rust application directly:
    ```bash
    cargo run
    ```
    This will run the simulation logic defined in `src/main.rs`, likely without graphical output.

## Key Features

*   **3D Robot Visualization:** A real-time 3D rendering of the Bittle X robot is provided using Three.js, driven by its URDF model and physics simulation data.
*   **Interactive Code Editor:** Integrated Monaco Editor allows users to edit robot control code (Arduino `.ino` files), documentation (`.md`), and other source files.
*   **Simulation Control:** Users can compile Arduino code, upload it to a simulated ESP32 controller, and run the simulation directly from the web interface.
*   **Serial Communication:** The UI displays real-time serial output from the simulated ESP32 and allows users to send commands.
*   **Physics Simulation:** The `gorilla-physics` library simulates the robot's dynamics, including kinematics and joint movements.
*   **URDF Model Definition:** The robot's structure, kinematics, and visual properties are defined in `www/static/robot.urdf` and associated `.stl` mesh files located in `www/static/assets/`.

## Development Conventions

*   **Rust:** Follows standard Rust practices. Asynchronous operations are managed using `tokio`. The `bittle-x` library is structured into `builder` and `control` modules for organizing robot construction and control logic.
*   **TypeScript/Frontend:** Employs standard ECMAScript and TypeScript module patterns. `rspack` is configured for bundling. `monaco-editor` is integrated for an enhanced code editing experience. `three.js` is utilized for 3D graphics rendering. Assets are managed and imported via the `rspack` build process.
*   **ESP32 Interaction:** The `esp32rs` crate and custom controller implementations (e.g., `BittleXEsp32Controller`) facilitate interaction with simulated ESP32 hardware and communication protocols.
*   **File Structure:** Source code is organized into `src/` for Rust and `www/src/` for the web frontend. Assets, including the URDF definition, STL meshes, compiled firmware, and source files, are placed within `www/static/` and `www/src/assets/`.

## Project Status

The project provides an end-to-end simulation environment for Petoi's Bittle X Robot Dog. It includes features for code editing, compilation for both Wasm and ESP32 targets, 3D visualization, and serial communication.
