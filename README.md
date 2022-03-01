[![Gitpod ready-to-code](https://img.shields.io/badge/Gitpod-ready--to--code-blue?logo=gitpod)](https://gitpod.io/#https://github.com/qube-ai/webthing-arduino)

webthing-arduino
================

A library with simple websocket client for the ESP8266 and the ESP32 boards that implements Mozilla's proposed Web of Things API.


## Installation (Platformio)
 - From local lib
    ```
    lib_deps = file:///path/to/lib/webthing-arduino
    ```
  - From Git
    ```
    lib_deps = https://github.com/qube-ai/webthing-arduino.git
    ```

## Message Schema
![Schema](https://img.shields.io/badge/Schema-Qube%20Things-blue.svg)

- `Set Property` The setProperty message type is sent from a web client to a Web Thing in order to set the value of one or more of its properties. This is equivalent to a PUT request on a Property resource URL using the REST API, but with the WebSocket API a property value can be changed multiple times in quick succession over an open socket and multiple properties can be set at the same time.
```json
{
  "messageType": "setProperty",
  "thingId": "thingId",
  "data": {
    "propertyId": "propertyId",
    "newValue": "newValue",
  }
}
```

- `Perform action` The requestAction message type is sent from a web client to a Web Thing to request an action be carried out on a Web Thing. This is equivalent to a POST request on an Actions resource URL using the REST API, but multiple actions can be requested at the same time or in quick succession over an open socket.
```json
{
  "thingId": "thingId",
  "messageType": "performAction",
  "data": {
    "actionId": { 
      "param1": "param1",
      "param2": "param2",
    }
  }
}


// here actionId is the not the key but the value. Eg : 
"fade": {
    "input": {
      "level": 50, // param1
      "duration": 2000 // param2
    },
   }

```

- `Get thing description` The getThingDescription message type is sent from a web client to a Web Thing to request the description of a Web Thing. This is equivalent to a GET request on a Thing resource URL using the REST API. This will give you the thingDescription of the provided thingId.
```json
{
  "thingId": "thingId",
  "messageType": "getThingDescription",
}
```

- `Get All Things` The getThingDescription message type is sent from a web client to a Web Thing to request the description of a Web Thing. This is equivalent to a GET request on a Thing resource URL using the REST API. This will give you the thingDescription of the all things.
```json
{
  "messageType": "getAllThings",
}
```

- `Get Property` This will give the value of the propertyId of the provided thingId.
```json
{
  "messageType": "getProperty",
  "thingId": "thingId",
}
```

### Message sent by the Web Thing to the Web Client

- `Property Status` The propertyStatus message type is sent from a Web Thing to a web client whenever a property of a Web Thing changes. The payload data of this message is in the same format as a response to a GET request on Property resource using the REST API, but the message is pushed to the client whenever a property changes and can include multiple properties at the same time.
```json
{
  "messageType": "propertyStatus",
  "thingId" : "some_thing_id",
  "data": {
    "led": true
  }
}
```

- `Action Status` The actionStatus message type is sent from a Web Thing to a web client when the status of a requested action changes. The payload data is consistent with the format of an Action resource in the REST API, but messages are pushed to the client as soon as the status of an action changes.
```json
{
  "messageType": "actionStatus",
  "thingId" : "some_thing_id",
  "data": {
    "grab": {
      "href": "/actions/grab/123e4567-e89b-12d3-a456-426655",
      "status": "completed",
      "timeRequested": "2017-01-24T11:02:45+00:00",
      "timeCompleted": "2017-01-24T11:02:46+00:00"
    }
  }
}
```

- `Event` The event message type is sent from a Web Thing to a web client when an event occurs on the Web Thing. The payload data is consistent with the format of an Event resource in the REST API but messages are pushed to the client as soon as an event occurs.
```json
{
  "messageType": "event",
  "thingId" : "some_thing_id",
  "data": {
    "motion": {
      "timestamp": "2017-01-24T13:02:45+00:00"
    }
  }
}
```
## Example Sketch
```cpp
#include <Arduino.h>
#include "Thing.h"
#include "QubeAdapter.h"

// TODO: Hardcode your wifi credentials here (and keep it private)
const char *ssid = "WillowCove";
const char *password = "Deepwaves007";

const int ledPin = LED_BUILTIN;

QubeAdapter *adapter;

void onOffChanged(ThingPropertyValue newValue); 
ThingActionObject *action_generator(DynamicJsonDocument *);

const char *ledTypes[] = {"OnOffSwitch", "Light", nullptr};
ThingDevice led("led", "Built-in LED", ledTypes);
ThingProperty ledOn("on", "", BOOLEAN, "OnOffProperty", onOffChanged);
StaticJsonDocument<256> fadeInput;
JsonObject fadeInputObj = fadeInput.to<JsonObject>();
ThingAction fade("fade", "Fade", "Fade the lamp to a given level",
                 "FadeAction", &fadeInputObj, action_generator);
ThingEvent overheated("overheated",
                      "The lamp has exceeded its safe operating temperature",
                      NUMBER, "OverheatedEvent");
                      
bool lastOn = false;

void onOffChanged(ThingPropertyValue newValue) {
  Serial.print("On/Off changed to : ");
  Serial.println(newValue.boolean);
  digitalWrite(ledPin, newValue.boolean ? LOW : HIGH);
}

void setup(void)
{
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  Serial.begin(115200);
  Serial.println("");
  Serial.print("Connecting to \"");
  Serial.print(ssid);
  Serial.println("\"");
#if defined(ESP8266) || defined(ESP32)
  WiFi.mode(WIFI_STA);
#endif
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  bool blink = true;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    digitalWrite(ledPin, blink ? LOW : HIGH); // active low led
    blink = !blink;
  }
  digitalWrite(ledPin, HIGH); // active low led
  adapter = new QubeAdapter("led", WiFi.localIP());

  led.addProperty(&ledOn);
  fadeInputObj["type"] = "object";
  JsonObject fadeInputProperties =
      fadeInputObj.createNestedObject("properties");
  JsonObject brightnessInput =
      fadeInputProperties.createNestedObject("brightness");
  brightnessInput["type"] = "integer";
  brightnessInput["minimum"] = 0;
  brightnessInput["maximum"] = 100;
  brightnessInput["unit"] = "percent";
  JsonObject durationInput =
      fadeInputProperties.createNestedObject("duration");
  durationInput["type"] = "integer";
  durationInput["minimum"] = 1;
  durationInput["unit"] = "milliseconds";
  led.addAction(&fade);

  overheated.unit = "degree celsius";
  led.addEvent(&overheated);

  adapter->addDevice(&led);
  adapter->begin("192.168.29.154", 8765);
  Serial.println("HTTP server started");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.print("/things/");
  Serial.println(led.id);
}

void loop()
{
  adapter->update();
}

void do_fade(const JsonVariant &input) {
  Serial.println("Fade called");
  Serial.println(input.as<String>());
  fadeInputObj = input.as<JsonObject>();
  Serial.println(fadeInputObj["level"].as<String>());
  Serial.println(fadeInputObj["duration"].as<String>());
  Serial.println(fadeInputObj["unit"].as<String>());
}

ThingActionObject *action_generator(DynamicJsonDocument *input) {
  return new ThingActionObject("fade", input, do_fade, nullptr);
}
```
