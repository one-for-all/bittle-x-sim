const { rspack } = require("@rspack/core");
const WasmPackPlugin = require("@wasm-tool/wasm-pack-plugin");
const MonacoWebpackPlugin = require("monaco-editor-webpack-plugin");

const path = require("path");
const isDev = process.env.NODE_ENV === "development";
const dist = path.resolve(__dirname, "../docs");

const featureGPU = process.env.FEATURE_GPU === "1";

const rspackConfig = {
  mode: isDev ? "development" : "production",
  entry: "./src/index.ts",
  devtool: isDev ? "inline-source-map" : false,
  output: {
    path: dist,
    filename: "bundle.js",
    clean: true,
  },
  resolve: {
    extensions: [".ts", ".js"],
    alias: {
      three: path.resolve(__dirname, "node_modules/three"),
    },
  },
  experiments: {
    asyncWebAssembly: true,
    syncWebAssembly: true,
  },
  module: {
    rules: [
      {
        test: /\.ts$/,
        loader: "builtin:swc-loader",
      },
      {
        test: /\.css$/,
        use: [rspack.CssExtractRspackPlugin.loader, "css-loader"],
      },
      {
        test: /\.(ino|h|c|cpp|hpp|md|hex|txt)$/,
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
      patterns: [
        {
          from: "static",
          to: dist,
          globOptions: {
            // Raw `.obj` meshes are fetched pre-gzipped (`.obj.gz`) at runtime,
            // so don't ship the uncompressed originals.
            ignore: ["**/GEMINI.md", "**/index.html", "**/mesh/*.obj"],
          },
        },
      ],
    }),

    new rspack.HtmlRspackPlugin({
      template: "./static/index.html",
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

    new rspack.CssExtractRspackPlugin({ filename: "css/main.css" }),
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
  devServer: {
    client: {
      overlay: {
        runtimeErrors: false,
      },
    },
  },
};

// Testing tools (see src/devTools.ts) are for the dev server only: `npm run
// start` and `npm run prod` serve, while `npm run build` makes the production
// site in docs/, where DEV_TOOLS is false and the tools are dropped as dead code.
module.exports = (env) => {
  rspackConfig.plugins.push(
    new rspack.DefinePlugin({ DEV_TOOLS: JSON.stringify(!!env.RSPACK_SERVE) }),
  );
  return rspackConfig;
};
