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

  <!-- Schema 
  1. Get Property
  2. Set Property
  3. Get Action Queue
  4. Set Action Queue
  5. Get Event Queue
  6. Get All Properties
  7. Get All Actions
  8. Get All Events
  9. Get Full thing description
  -->


## Message Schema
![Schema](https://img.shields.io/badge/Schema-Qube%20Things-blue.svg)

- Get Property
```json
{
  "messageType": "getProperty",
  "thingId": "thingId",
  "id": "1", 
}
```

- Set Property
```json
{
  "messageType": "setProperty",
  "thingId": "thingId",
  "id": "2",
  "data": {
    "propertyId": "propertyId",
    "newValue": "newValue",
  }
}
```

- Get Action Queue
```json
{
  "thingId": "thingId",
  "messageType": "getActionQueue",
  "id": "3",
}
```

- Set Action Queue
```json
{
  "messageType": "setActionQueue",
  "thingId": "thingId",
  "id": "4",
  "data": {
    "actionId": "actionId",
    "newValue": "newValue",
  }
}
```

- Get Event Queue
```json
{
  "thingId": "thingId",
  "messageType": "getEventQueue",
  "id": "5",
}
```

- Get All Properties
```json
{
  "thingId": "thingId",
  "messageType": "getAllProperties",
  "id": "6",
}
```

- Get All Actions
```json
{
  "thingId": "thingId",
  "messageType": "getAllActions",
  "id": "7",
}
```

- Get All Events
```json
{
  "thingId": "thingId",
  "messageType": "getAllEvents",
  "id": "8",
}
```

- Get Full thing description
```json
{
  "thingId": "thingId",
  "messageType": "getFullThingDescription",
  "id": "9",
}
```