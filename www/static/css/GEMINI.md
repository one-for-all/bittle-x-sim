# CSS Component Structure

The CSS for the Bittle X Simulation project is modularly organized into distinct functional components, each targeted at a specific part of the user interface. This separation ensures maintainability and clear scoping of styles.

## Overview of CSS Files

- **`main.css`**: Defines the global layout wrapper and the resizing mechanism for the main interface panels.
- **`sidebar.css`**: Manages the styling of the file explorer sidebar, including layout, interactivity, and visual states.
- **`editor.css`**: Contains styles for the code editor area, including file tabs, the file management bar, and run/stop action buttons.
- **`sim.css`**: Sets the structural foundation for the 3D simulation canvas (`#threejs`).
- **`serial-monitor.css`**: Handles the appearance and layout of the build output logs, serial monitor text, and the serial input bar at the bottom of the simulation view.
- **`overlay.css`**: Manages components that float above the main interface, such as the Petoi/APE logos, simulation stats (`#realtimeRatio`), and pre-defined movement buttons.

## Organization Strategy

The project employs a **modular component-based approach**. By segmenting the CSS into functional files that map to the application's DOM structure (defined in `index.html`), the architecture keeps the codebase clean:

1.  **Layout-First Approach**: `main.css` acts as the root container, using Flexbox for high-level partitioning.
2.  **Contextual Scoping**: Styles are scoped to specific ID and class selectors (e.g., `#editor`, `.sidebar`, `.serial-input-bar`) to avoid global style collisions.
3.  **UI Component Separation**: Features like the serial monitor and the overlay are extracted into their own files, even though they share the same physical screen area, making it easy to toggle or adjust them independently.
