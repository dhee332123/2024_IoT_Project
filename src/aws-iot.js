import {
  IoTDataPlaneClient,
  GetThingShadowCommand,
  UpdateThingShadowCommand,
} from "@aws-sdk/client-iot-data-plane";

const client = new IoTDataPlaneClient({
  region: "ap-northeast-2", // AWS 리전
  credentials: {
    accessKeyId: "AKIA2MNVMG4PH4Y6NWMS", // AWS 액세스 키
    secretAccessKey: "QbiNNEQaV1kbrDQg/YltXXjFiolKj5CO4C6II2Q0", // AWS 시크릿 키
  },
  endpoint: "https://a2bx7ei8ft2iig-ats.iot.ap-northeast-2.amazonaws.com", // AWS IoT 엔드포인트
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
    // Shadow 업데이트
    const response = await updateShadowData(thingName, payload);
    console.log(`Window state updated to ${state} in desired`, response);
    alert(`창문이 ${state === "open" ? "열림" : "닫힘"} 상태로 업데이트되었습니다.`);
  } catch (error) {
    console.error("Failed to update window state:", error);
    alert("창문 상태를 업데이트하는 데 실패했습니다.");
  }
};
