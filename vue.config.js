const { defineConfig } = require('@vue/cli-service');

module.exports = defineConfig({
  transpileDependencies: true,
  pluginOptions: {
    dotenv: {
      path: '.env',
    },
  },
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
