# Realtime benchmarks

Headless tools for measuring how close to realtime the simulator runs. Both
build the scene `createBittleX` builds -- robot, classic ESP32 board running the
firmware in `www/static/OpenCatEsp32`, beverage can, ground -- from this repo's
own sources, and step it at the page's substep (`simulator.run(60, 0)`,
1/3600 s). Realtime rate is reported with all of the wall clock spent
stepping; the page gives the physics roughly 70-80% of each frame, so it shows
about that fraction of these numbers.

```sh
# native: fast to build, for profiling
cd tools/realtime/native
cargo run --release -- 8 4.0:kwkF        # 8 simulated s, "kwkF" typed at t = 4 s

# wasm under Node: what the page runs
cd tools/realtime/wasm
wasm-pack build --release --target nodejs --out-dir pkg
node run.mjs pkg 8 4.0:kwkF
```

The native bench also checks the firmware's IMU auto-recovery: a command
`@tip<degrees>` tips the robot over about its long axis at that time (the
same `builder::tip_over` the dev server's "Lay on ... Side" / "Flip on Back"
buttons call), and `TRACE=1` prints where its top and head point every half
second. Recovered, it ends with `up.z=+1.00` and the console shows
`EXCEPTION: Fall over`, `rc`, `up`:

```sh
TRACE=1 UART=1 cargo run --release -- 14 4.0:@tip180   # onto its back
TRACE=1 UART=1 cargo run --release -- 16 4.0:@tip90    # onto its left side
STATIC_DIR=../../../docs cargo run --release -- 6 2.0:kwkF   # the deployed images
```

The native bench reads the images from `www/static`, as the page does, through
the native branch of `BittleXEsp32Controller::new` (its relative paths match
the layout there). Both print a fingerprint at the end -- final poses and a
hash of the console. The console hash differs run to run (the firmware names
the robot from the emulated RNG); the pose does not.

Profiling helpers and notes on telling a real behaviour change from solver
noise are with the same tools in quaddle-sim: `../quaddle-sim/tools/realtime`.
