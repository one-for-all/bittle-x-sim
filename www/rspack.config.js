const { rspack } = require("@rspack/core");
const WasmPackPlugin = require("@wasm-tool/wasm-pack-plugin");
const MonacoWebpackPlugin = require("monaco-editor-webpack-plugin");

const path = require("path");
const isDev = process.env.NODE_ENV === "development";
const dist = path.resolve(__dirname, "../docs");

const featureGPU = process.env.FEATURE_GPU === "1";

const rspackConfig = {
  mode: isDev ? "development" : "production",
  entry: {
    index: "./src/index.ts",
    editor: "./src/editor.ts",
  },
  devtool: isDev ? "inline-source-map" : false,
  output: {
    path: dist,
    filename: "[name].js",
  },
  resolve: {
    extensions: [".ts", ".js"],
  },
  experiments: {
    asyncWebAssembly: true,
    syncWebAssembly: true,
  },
  module: {
    rules: [
      {
        test: /\.ts$/,
        use: "ts-loader",
      },
      {
        test: /\.css$/,
        use: ["style-loader", "css-loader"],
      },
      {
        test: /\.(ino|h|cpp|md|hex|txt)$/,
        type: "asset/source",
      },
      {
        test: /\.bin$/,
        loader: "arraybuffer-loader",
      },
    ],
  },
  plugins: [
    new rspack.CopyRspackPlugin({
      patterns: [{ from: "static", to: dist }],
    }),

    new WasmPackPlugin({
      crateDirectory: path.resolve(__dirname, "../"),
      extraArgs: featureGPU ? "--features gpu" : "",
      watchDirectories: [
        path.resolve(__dirname, "../../gorilla-physics/src"),
        path.resolve(__dirname, "../../esp32rs/src"),
        path.resolve(__dirname, "static"),
      ],
    }),

    new MonacoWebpackPlugin({
      languages: ["cpp", "markdown"],
      themes: ["vs-dark"],
    }),
  ],
  // To disable warning on screen
  stats: {
    warnings: false,
  },
  performance: {
    hints: false,
  },
  cache: {
    // for speeding up the rebuild
    type: "filesystem",
    buildDependencies: {
      config: [__filename],
    },
  },
};

module.exports = rspackConfig;
