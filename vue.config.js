const { defineConfig } = require('@vue/cli-service');

module.exports = defineConfig({
  transpileDependencies: true,
  configureWebpack: {
    resolve: {
      fallback: {
        util: require.resolve("util/"),
        url: require.resolve("url/"),
        buffer: require.resolve("buffer/"),
        stream: require.resolve("stream-browserify"),
      },
    },
  },
});
