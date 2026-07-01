// Pre-gzip the OBJ meshes so they can be fetched compressed at runtime.
//
// The simulator downloads every mesh referenced by robot.urdf on load, and
// GitHub Pages serves `.obj` as `application/x-tgif` (not on its gzip
// whitelist), so the raw meshes ship uncompressed (~20MB). We gzip them here
// to `*.obj.gz` (~6-8x smaller); the wasm inflates them via `maybe_read_web_file_gz`.
const fs = require("fs");
const path = require("path");
const zlib = require("zlib");

const meshDir = path.resolve(__dirname, "../static/mesh");

if (!fs.existsSync(meshDir)) {
  console.warn(`[gzip-meshes] mesh dir not found, skipping: ${meshDir}`);
  process.exit(0);
}

const files = fs.readdirSync(meshDir).filter((f) => f.endsWith(".obj"));
let rawTotal = 0;
let gzTotal = 0;

for (const file of files) {
  const src = path.join(meshDir, file);
  const dst = `${src}.gz`;
  const input = fs.readFileSync(src);
  const gz = zlib.gzipSync(input, { level: zlib.constants.Z_BEST_COMPRESSION });
  fs.writeFileSync(dst, gz);
  rawTotal += input.length;
  gzTotal += gz.length;
}

const mb = (n) => (n / 1048576).toFixed(1);
console.log(
  `[gzip-meshes] gzipped ${files.length} meshes: ` +
    `${mb(rawTotal)}MB -> ${mb(gzTotal)}MB`,
);
