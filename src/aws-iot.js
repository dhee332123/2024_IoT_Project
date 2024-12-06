import {
  IoTDataPlaneClient,
  GetThingShadowCommand,
  UpdateThingShadowCommand,
} from "@aws-sdk/client-iot-data-plane";

// AWS IoT 클라이언트 초기화
const client = new IoTDataPlaneClient({
  region: process.env.VUE_APP_AWS_REGION,
  credentials: {
    accessKeyId: process.env.VUE_APP_AWS_ACCESS_KEY_ID,
    secretAccessKey: process.env.VUE_APP_AWS_SECRET_ACCESS_KEY,
  },
  endpoint: process.env.VUE_APP_AWS_IOT_ENDPOINT,
});

// Shadow 데이터를 가져오는 함수
export const getShadowData = async (thingName) => {
  const command = new GetThingShadowCommand({ thingName });

  try {
    const response = await client.send(command);
    const shadowData = JSON.parse(new TextDecoder("utf-8").decode(response.payload));
    console.log("Shadow Data:", shadowData);
    return shadowData;
  } catch (error) {
    console.error("Error fetching shadow data:", error);
    throw error;
  }
};

// Shadow 데이터를 업데이트하는 함수
export const updateShadowData = async (thingName, payload) => {
  const command = new UpdateThingShadowCommand({
    thingName,
    payload: new TextEncoder().encode(JSON.stringify(payload)),
  });

  try {
    const response = await client.send(command);
    const updatedShadow = JSON.parse(new TextDecoder("utf-8").decode(response.payload));
    console.log("Updated Shadow:", updatedShadow);
    return updatedShadow;
  } catch (error) {
    console.error("Error updating shadow data:", error);
    throw error;
  }
};

// 창문 상태를 desired에 업데이트하는 함수
export const updateWindowState = async (thingName, state) => {
  const payload = {
    state: {
      desired: {
        window: state, // desired 상태
      },
    },
  };

  try {
    const response = await updateShadowData(thingName, payload);
    console.log(`Window state updated to ${state} in desired`, response);
    alert(`창문이 ${state === "open" ? "열림" : "닫힘"} 상태로 업데이트되었습니다.`);
  } catch (error) {
    console.error("Failed to update window state:", error);
    alert("창문 상태를 업데이트하는 데 실패했습니다.");
  }
};
