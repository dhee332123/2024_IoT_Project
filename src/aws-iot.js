import { IoTDataPlaneClient, GetThingShadowCommand } from "@aws-sdk/client-iot-data-plane";

const client = new IoTDataPlaneClient({
  region: "ap-northeast-2", // AWS 리전
  credentials: {
    accessKeyId: "AKIA2MNVMG4PH4Y6NWMS", // AWS 액세스 키
    secretAccessKey: "QbiNNEQaV1kbrDQg/YltXXjFiolKj5CO4C6II2Q0", // AWS 시크릿 키
  },
  endpoint: "https://a2bx7ei8ft2iig-ats.iot.ap-northeast-2.amazonaws.com", // AWS IoT 엔드포인트
});

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
