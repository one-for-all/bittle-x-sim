# UI Organization Strategy

The Bittle X Robot Simulation utilizes a modular, component-based layout strategy designed for a split-screen workspace environment.

## Layout Architecture

### 1. High-Level Structure (Flexbox)
- **`main-wrapper` (`main.css`)**: The root container uses `display: flex` with `flex-direction: row` to create a side-by-side workspace.
- **Resizing**: A `.main-resize-handle` separates the editor area from the simulation area, allowing for manual width adjustment.

### 2. Functional Areas
- **Sidebar (`sidebar.css`)**: A left-aligned flex container that toggles visibility. It occupies a fixed width and manages file navigation.
- **Editor Area (`editor.css`)**: Uses a vertical flex column (`flex-direction: column`) to stack the `file-bar` and the `editor` (Monaco) area. The editor expands (`flex: 1`) to fill available space.
- **Simulation/Serial Area (`sim.css`, `serial-monitor.css`)**: 
    - The container (`#threejs`) fills the remaining horizontal space (`flex: 1`).
    - **Serial Interface**: `serial-monitor.css` uses `position: absolute` with `bottom: 0` to anchor the serial input bar and build output logs at the bottom of the simulation view, overlaying the 3D canvas.

### 3. Absolute Positioning & Overlays (`overlay.css`)
- **Layering (`z-index`)**: An `.overlay` container is used for elements that must persist above the simulation canvas (e.g., logos, movement controls, simulation metrics).
- **Positioning**: Absolute positioning is used to anchor controls to specific corners of the screen, ensuring they remain accessible regardless of the canvas state.

## Best Practices for Future Development
- **New Components**: Follow the existing pattern: define structure in `index.html`, basic container layout in a dedicated `.css` file in `www/static/css/`, and apply via appropriate HTML classes.
- **Layout Integrity**: Maintain the `main-wrapper` as the source of truth for high-level partitioning. Avoid modifying the `body`/`html` styles in `index.html` unless changing global viewport constraints.
- **Layering**: Use `z-index` values defined in `overlay.css` (>10) for UI components meant to float above the simulation, and lower values for background/layout components.
