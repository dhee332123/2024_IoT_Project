<template>
  <div id="app">
    <DeviceShadow />
  </div>
</template>

<script>
import DeviceShadow from "./components/DeviceShadow.vue";

export default {
  name: "App",
  components: {
    DeviceShadow,
  },
};
</script>

<style>
#app {
  text-align: center;
  margin-top: 50px;
}
</style>
