<template>
  <div class="device-shadow">
    <!-- 탭 버튼 -->
    <div class="tabs">
      <button
        :class="{ active: activeTab === 'care' }"
        @click="activeTab = 'care'"
      >
        에어 케어
      </button>
      <button
        :class="{ active: activeTab === 'data' }"
        @click="activeTab = 'data'"
      >
        에어 데이터
      </button>
    </div>

    <!-- 에어 케어 탭 -->
    <div v-if="activeTab === 'care'" class="tab-content">
      <h2>집 안</h2>
      <div class="card-group">
        <div
          class="card"
          :style="{ backgroundColor: getColorByStatus(co2Status) }"
        >
          <h3>이산화탄소 농도</h3>
          <p>{{ co2Status }} ({{ shadowData?.state?.reported?.co2 || 0 }} ppm)</p>
        </div>
        <div
          class="card"
          :style="{ backgroundColor: getColorByStatus(humidityStatus) }"
        >
          <h3>습도</h3>
          <p>{{ humidityStatus }} ({{ shadowData?.state?.reported?.humid || 0 }}%)</p>
        </div>
        <div
          class="card"
          :style="{ backgroundColor: getColorByStatus(harmfulGasStatus) }"
        >
          <h3>유해가스</h3>
          <p>{{ harmfulGasStatus }} ({{ shadowData?.state?.reported?.mq135 || 0 }})</p>
        </div>
      </div>

      <h2>현재 지역: {{ shadowData?.state?.reported?.region || "알 수 없음" }}</h2>
      <div class="card-group">
        <div
          class="card"
          :style="{ backgroundColor: getColorByStatus(pm10Status) }"
        >
          <h3>미세먼지</h3>
          <p>{{ pm10Status }} ({{ shadowData?.state?.reported?.airQuality?.pm10 || 0 }} μg/m³)</p>
        </div>
        <div
          class="card"
          :style="{ backgroundColor: getColorByStatus(pm25Status) }"
        >
          <h3>초미세먼지</h3>
          <p>{{ pm25Status }} ({{ shadowData?.state?.reported?.airQuality?.pm25 || 0 }} μg/m³)</p>
        </div>
      </div>

      <div class="controls">
        <button>창문 열기</button>
        <button>창문 닫기</button>
      </div>
    </div>

    <!-- 에어 데이터 탭 -->
    <div v-if="activeTab === 'data'" class="tab-content">
      <div class="graph-container">
        <div>
          <h3>이산화탄소 농도</h3>
          <div ref="co2Graph" class="graph"></div>
          <p>가장 높은 값: {{ maxCo2Time }}</p>
        </div>
        <div>
          <h3>유해가스 농도</h3>
          <div ref="harmfulGasGraph" class="graph"></div>
          <p>가장 높은 값: {{ maxGasTime }}</p>
        </div>
      </div>
    </div>
  </div>
</template>

<script>
import * as echarts from "echarts";
import { getShadowData } from "@/aws-iot";

export default {
  data() {
    return {
      shadowData: null, // 섀도우 데이터 저장
      pollingInterval: null, // 주기적 호출을 위한 인터벌 저장
      thingName: "Sensor_Device", // Thing 이름
      activeTab: "care", // 현재 활성화된 탭 (초기값은 'care')
      co2Data: [], // 이산화탄소 데이터 저장 (최대 24개)
      harmfulGasData: [], // 유해가스 데이터 저장 (최대 24개)
    };
  },
  computed: {
    harmfulGasStatus() { // 복구된 함수
    const gas = this.shadowData?.state?.reported?.mq135 || 0;
    if (gas <= 2300) return "안전";
    if (gas < 3000) return "주의";
    return "위험";
  },
    co2Status() {
      const co2 = this.shadowData?.state?.reported?.co2 || 0;
      if (co2 <= 1000) return "좋음";
      if (co2 <= 2000) return "보통";
      if (co2 < 5000) return "나쁨";
      return "매우 나쁨";
    },
    humidityStatus() {
      const humidity = this.shadowData?.state?.reported?.humid || 0;
      if (humidity < 40) return "건조함";
      if (humidity <= 60) return "적당함";
      return "습함";
    },
    pm10Status() {
      const pm10 = this.shadowData?.state?.reported?.airQuality?.pm10 || 0;
      if (pm10 <= 30) return "좋음";
      if (pm10 <= 80) return "보통";
      if (pm10 <= 150) return "나쁨";
      return "매우 나쁨";
    },
    pm25Status() {
      const pm25 = this.shadowData?.state?.reported?.airQuality?.pm25 || 0;
      if (pm25 <= 15) return "좋음";
      if (pm25 <= 50) return "보통";
      if (pm25 <= 100) return "나쁨";
      return "매우 나쁨";
    },
    maxCo2Time() {
      const maxValue = Math.max(...this.co2Data);
      const maxIndex = this.co2Data.indexOf(maxValue);
      return maxIndex >= 0 ? `${maxIndex * 5}분` : "알 수 없음";
    },
    maxGasTime() {
      const maxValue = Math.max(...this.harmfulGasData);
      const maxIndex = this.harmfulGasData.indexOf(maxValue);
      return maxIndex >= 0 ? `${maxIndex * 5}분` : "알 수 없음";
    },
  },

  methods: {
    getColorByStatus(status) {
      switch (status) {
        case "좋음":
          return "#d4edda"; // 연한 녹색
        case "보통":
          return "#fff3cd"; // 연한 노랑
        case "주의":
        case "나쁨":
          return "#f8d7da"; // 연한 빨강
        case "매우 나쁨":
        case "위험":
          return "#f5c6cb"; // 짙은 빨강
        case "건조함":
          return "#e8f4fa"; // 연한 파랑
        case "습함":
          return "#cce5ff"; // 밝은 파랑
        default:
          return "#f4f4f4"; // 기본값 회색
      }
    },

    async fetchShadowData() {
      try {
        const data = await getShadowData(this.thingName);
        this.shadowData = data;

        // 데이터 배열에 새 값 추가
        this.addDataPoint(
          this.co2Data,
          this.shadowData?.state?.reported?.co2 || 0
        );
        this.addDataPoint(
          this.harmfulGasData,
          this.shadowData?.state?.reported?.mq135 || 0
        );

        // 그래프 갱신
        this.updateGraph("co2Graph", this.co2Data);
        this.updateGraph(
          "harmfulGasGraph",
          this.harmfulGasData
        );
      } catch (error) {
        console.error("Failed to fetch shadow data:", error);
      }
    },
    addDataPoint(array, value) {
      array.push(value);
      if (array.length > 24) {
        array.shift();
      }
    },
    updateGraph(refName, data, title) {
      const chart = echarts.init(this.$refs[refName]);
      const options = {
        title: {
          text: title,
          left: "center",
        },
        xAxis: {
          type: "category",
          data: Array.from({ length: data.length }, (_, i) => `${i * 5}분`),
        },
        yAxis: {
          type: "value",
        },
        series: [
          {
            data,
            type: "line",
            smooth: true,
          },
        ],
      };
      chart.setOption(options);
    },
    startPolling() {
      this.pollingInterval = setInterval(this.fetchShadowData, 5000);
    },
    stopPolling() {
      if (this.pollingInterval) {
        clearInterval(this.pollingInterval);
        this.pollingInterval = null;
      }
    },
  },
  mounted() {
    this.fetchShadowData();
    this.startPolling();
  },
  beforeUnmount() {
    this.stopPolling();
  },
};
</script>

<style scoped>
.device-shadow {
  font-family: Arial, sans-serif;
  padding: 20px;
  text-align: center;
}
.tabs {
  display: flex;
  justify-content: center;
  margin-bottom: 20px;
}
.tabs button {
  padding: 10px 20px;
  margin: 0 10px;
  cursor: pointer;
  border: none;
  background-color: #f0f0f0;
  font-size: 16px;
}
.tabs button.active {
  background-color: #007bff;
  color: white;
}
.card-group {
  display: flex;
  justify-content: center;
  gap: 20px;
  margin-bottom: 20px;
}
.card {
  flex: 1;
  max-width: 200px;
  padding: 20px;
  background-color: #f4f4f4;
  border: 1px solid #ddd;
  text-align: center;
  font-size: 18px;
  border-radius: 8px;
}
.controls {
  display: flex;
  justify-content: center;
  gap: 10px;
  margin-top: 20px;
}
.controls button {
  padding: 10px 20px;
  background-color: #007bff;
  color: white;
  border: none;
  cursor: pointer;
  border-radius: 5px;
}
.graph-container {
  display: flex;
  justify-content: center;
  gap: 20px;
  flex-wrap: wrap;
}
.graph {
  width: 400px;
  height: 300px;
  border: 1px solid #ddd;
  border-radius: 8px;
  margin-top: 10px;
}

.device-shadow {
  font-family: Arial, sans-serif;
  padding: 20px;
  text-align: center;
}
.tabs {
  display: flex;
  justify-content: center;
  margin-bottom: 20px;
}
.tabs button {
  padding: 10px 20px;
  margin: 0 10px;
  cursor: pointer;
  border: none;
  background-color: #f0f0f0;
  font-size: 16px;
}
.tabs button.active {
  background-color: #007bff;
  color: white;
}
.graph-container {
  display: flex;
  justify-content: center;
  gap: 20px;
  flex-wrap: wrap;
}
.graph {
  width: 400px;
  height: 300px;
  border: 1px solid #ddd;
  border-radius: 8px;
  margin-top: 10px;
}
</style>
