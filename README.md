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

### Message sent by the Web Thing to the Web Client

- Property Status. `The propertyStatus message type is sent from a Web Thing to a web client whenever a property of a Web Thing changes. The payload data of this message is in the same format as a response to a GET request on Property resource using the REST API, but the message is pushed to the client whenever a property changes and can include multiple properties at the same time.`
```json
{
  "messageType": "propertyStatus",
  "data": {
    "led": true
  }
}
```

- Action Status. `The actionStatus message type is sent from a Web Thing to a web client when the status of a requested action changes. The payload data is consistent with the format of an Action resource in the REST API, but messages are pushed to the client as soon as the status of an action changes.`
```json
{
  "messageType": "actionStatus",
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

- Event. `The event message type is sent from a Web Thing to a web client when an event occurs on the Web Thing. The payload data is consistent with the format of an Event resource in the REST API but messages are pushed to the client as soon as an event occurs.`
```json
{
  "messageType": "event",
  "data": {
    "motion": {
      "timestamp": "2017-01-24T13:02:45+00:00"
    }
  }
}
```