import { createRequire } from "module";
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";
const require = createRequire(import.meta.url);
// Usage: node run.mjs <pkg dir> [sim seconds] [time:command ...]   (QUIET=1 for totals only)
const here = path.dirname(fileURLToPath(import.meta.url));
const pkgDir = path.resolve(process.argv[2] ?? `${here}/pkg`);
const tFinal = parseFloat(process.argv[3] ?? "8");
const cmds = process.argv.slice(4).map((a) => { const i = a.indexOf(":"); return [parseFloat(a.slice(0, i)), a.slice(i + 1)]; });
const { Bench } = require(`${pkgDir}/bittle_realtime_wasm.js`);
const W = path.resolve(here, "../../../www/static");
const B = `${W}/OpenCatEsp32/build`;
const rd = (p) => fs.readFileSync(p);
const tx = (p) => fs.readFileSync(p, "utf8");
const bench = new Bench(
  tx(`${W}/robot.urdf`), rd(`${W}/rom/wokwi/rom1.bin`), rd(`${W}/rom/wokwi/rom0.bin`), tx(`${W}/rom/symbols.txt`),
  rd(`${B}/OpenCatEsp32.ino.bootloader.bin`), rd(`${B}/OpenCatEsp32.ino.partitions.bin`),
  rd(`${B}/OpenCatEsp32.ino.bin`), tx(`${B}/symbols.txt`), tx(`${W}/OpenCatEsp32/bootloader_symbols.txt`));
const dt = 1 / 3600, window = 1800, steps = Math.round(tFinal / dt);
const quiet = !!process.env.QUIET;
let next = 0;
const t0 = performance.now();
for (let s = 0; s < steps; s += window) {
  const w0 = performance.now();
  for (let k = 0; k < window; k += 50) {
    const t = (s + k) * dt;
    while (next < cmds.length && t >= cmds[next][0]) { bench.send_uart(cmds[next][1]); next++; }
    bench.step(50);
  }
  const wall = (performance.now() - w0) / 1000;
  if (!quiet) console.log(`t=${((s + window) * dt).toFixed(2)} rt=${(window * dt / wall).toFixed(2)} step=${(1e6 * wall / window).toFixed(0)}us`);
}
const wall = (performance.now() - t0) / 1000;
console.log(`TOTAL sim ${tFinal.toFixed(2)}s wall ${wall.toFixed(2)}s realtime ${(tFinal / wall).toFixed(3)}`);
console.log(bench.fingerprint());
