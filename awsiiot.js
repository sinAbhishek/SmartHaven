const express = require("express");
const app = express();
const dotenv = require("dotenv");
const awsIot = require("aws-iot-device-sdk");

dotenv.config();

const device = awsIot.device({
  keyPath: "./private.key",
  certPath: "./certificate.crt",
  caPath: "./AmazonRootCA1.pem",
  clientId: "NodeClient",
  host: "a223t3ecpuf9xd-ats.iot.us-east-1.amazonaws.com",
});

let tempHumidityData = null;

device.on("connect", function () {
  console.log("Connected to AWS IoT");

  // Subscribe to topics
  device.subscribe("esp8266/control/on");
  device.subscribe("esp8266/control/off");
  device.subscribe("esp8266/temperature_humidity");
});

device.on("error", function (error) {
  console.log("Error: ", error);
});

// Handle incoming messages
device.on("message", function (topic, payload) {
  console.log("Message received on topic", topic);
  console.log("Payload:", payload.toString());

  if (topic === "esp8266/temperature_humidity") {
    tempHumidityData = JSON.parse(payload.toString());
    console.log("Temperature:", tempHumidityData.temperature);
    console.log("Humidity:", tempHumidityData.humidity);
  }
});

app.get("/on", async (req, res) => {
  try {
    device.publish("esp8266/control/on", JSON.stringify({}));
    res.send("Bulb turned ON");
  } catch (error) {
    res.send("Error turning on the bulb");
  }
});

app.get("/off", async (req, res) => {
  try {
    device.publish("esp8266/control/off", JSON.stringify({}));
    res.send("Bulb turned OFF");
  } catch (error) {
    res.send("Error turning off the bulb");
  }
});

app.get("/temperature_humidity", async (req, res) => {
  try {
    // Publish a request to the ESP8266 to send the temperature and humidity data
    device.publish("esp8266/request/temperature_humidity", JSON.stringify({}));

    // Wait for the ESP8266 to respond
    setTimeout(() => {
      if (tempHumidityData) {
        res.send(tempHumidityData);
      } else {
        res.send("No temperature and humidity data received");
      }
    }, 5000); // Wait for 5 seconds
  } catch (error) {
    res.send("Error requesting temperature and humidity data");
  }
});

const port = process.env.PORT || 4000;
app.listen(port, () => {
  console.log(`Server is running on port ${port}`);
});
