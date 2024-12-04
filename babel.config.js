module.exports = {
  presets: [
    [
      "@babel/preset-env",
      {
        useBuiltIns: "usage",
        corejs: 3, // 최신 CoreJS 사용
      },
    ],
  ],
  plugins: ["@babel/plugin-transform-runtime"],
};
