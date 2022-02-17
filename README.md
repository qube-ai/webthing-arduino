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

- Set Property. `The setProperty message type is sent from a web client to a Web Thing in order to set the value of one or more of its properties. This is equivalent to a PUT request on a Property resource URL using the REST API, but with the WebSocket API a property value can be changed multiple times in quick succession over an open socket and multiple properties can be set at the same time.`
```json
{
  "messageType": "setProperty",
  "thingId": "thingId",
  "id": "1",
  "data": {
    "propertyId": "propertyId",
    "newValue": "newValue",
  }
}
```

- Get Action Request.
`The requestAction message type is sent from a web client to a Web Thing to request an action be carried out on a Web Thing. This is equivalent to a POST request on an Actions resource URL using the REST API, but multiple actions can be requested at the same time or in quick succession over an open socket.`
```json
{
  "thingId": "thingId",
  "messageType": "getActionRequest",
  "id": "2",
  "data": {
    "actionId": "actionId",
    "params": {
      "param1": "param1",
      "param2": "param2",
    }
  }
}
```

- Get thing description. `The getThingDescription message type is sent from a web client to a Web Thing to request the description of a Web Thing. This is equivalent to a GET request on a Thing resource URL using the REST API. This will give you the thingDescription of the provided thingId.`
```json
{
  "thingId": "thingId",
  "messageType": "getThingDescription",
  "id": "3",
}
```

- Get All Things. `The getThingDescription message type is sent from a web client to a Web Thing to request the description of a Web Thing. This is equivalent to a GET request on a Thing resource URL using the REST API. This will give you the thingDescription of the all things.`
```json
{
  "messageType": "getAllThings",
  "id": "4",
}
```

- Get Property. `This will give the value of the propertyId of the provided thingId.`
```json
{
  "messageType": "getProperty",
  "thingId": "thingId",
  "id": "5", 
}
```