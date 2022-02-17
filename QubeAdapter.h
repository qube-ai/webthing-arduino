#pragma once

#define ESP32 1

#if defined(ESP32) || defined(ESP8266)

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include "Thing.h"
#include <WebSocketsClient.h>

#define ESP_MAX_PUT_BODY_SIZE 512

#ifndef LARGE_JSON_DOCUMENT_SIZE
#ifdef LARGE_JSON_BUFFERS
#define LARGE_JSON_DOCUMENT_SIZE 4096
#else
#define LARGE_JSON_DOCUMENT_SIZE 1024
#endif
#endif

#ifndef SMALL_JSON_DOCUMENT_SIZE
#ifdef LARGE_JSON_BUFFERS
#define SMALL_JSON_DOCUMENT_SIZE 1024
#else
#define SMALL_JSON_DOCUMENT_SIZE 256
#endif
#endif

class QubeAdapter {

    public:
    QubeAdapter(String _name, IPAddress _ip, uint16_t _port = 80,
                  bool _disableHostValidation = false)
      : name(_name), ip(_ip.toString()), port(_port),
        disableHostValidation(_disableHostValidation) {}
    
    String name;
    String ip;
    uint16_t port;
    bool disableHostValidation;
    ThingDevice *firstDevice = nullptr;
    ThingDevice *lastDevice = nullptr;
    char body_data[ESP_MAX_PUT_BODY_SIZE];
    bool b_has_body_data = false;
    WebSocketsClient webSocket;

    WebSocketsClient getWebSocketClient() {
        return webSocket;
    }    

    void messageHandler(const String payload){
        
        DynamicJsonDocument doc(LARGE_JSON_DOCUMENT_SIZE);
        DeserializationError error = deserializeJson(doc, payload);
        if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.c_str());
            String errorMessage = "{\"error\":\"" + error.c_str() + "\"}";
            websocket.sendTXT(errorMessage);
        }

        JsonObject root = doc.as<JsonObject>();

        if (root["messageType"] == "getProperty") {
            String thingId = root["thingId"];
            handleThingPropertiesGet(thingId);
        }

        if (root["messageType"] == "setProperty") {
            String thingId = root["thingId"];
            String propertyId = root["data"]["propertyId"];
            handleThingPropertyPutV2(thingId, propertyId, root["data"]);
        }

        if (root["messageType"] == "getThingDescription") {
            String thingId = root["thingId"];
            handleThing(thingId);
        }

        if (root["messageType"] == "getAllThings") {
            handleThings();
        }

        if (root["messageType"] == "performAction") {
            String thingId = root["thingId"];
            String actionId = root["data"]["actionId"];
            handleThingActionPost(thingId, root["data"]);
        }

    }

    void payloadHandler(uint8_t *payload, size_t length)
    {
        Serial.printf("Got payload -> %s\n", payload);
        char msgch[length];
        for (unsigned int i = 0; i < length; i++)
        {
            msgch[i] = ((char)payload[i]);
        }
        msgch[length] = '\0';
        // Parse Json
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, msgch);
        if (error)
        {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.c_str());
            return;
        }
        else
        {
            Serial.println("Parsed JSON");
            Serial.println(doc.as<String>());
        }
    }

    void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
    {
        switch (type)
        {
        case WStype_DISCONNECTED:
            Serial.printf("[WSc] Disconnected!\n");
            break;

        case WStype_CONNECTED:
            Serial.printf("[WSc] Connected to url: %s\n", payload);
            break;

        case WStype_TEXT:
            payloadHandler(payload, length);
            break;

        case WS_EVT_DATA:
            Serial.printf("[WSc] get binary length: %u\n", length);
            break;
        }
}

    ThingDevice* findDeviceById(String id){
        ThingDevice *device = this->firstDevice;
        while(device != nullptr){
            if(device->id == id){
                return device;
            }
            device = device->next;
        }
        return nullptr;
    }

    ThingProperty *findPropertyById(ThingDevice *device, String id){
        ThingProperty *property = device->firstProperty;
        while(property != nullptr){
            if(property->id == id){
                return property;
            }
            property = (ThingProperty *)property->next;
        }
        return nullptr;
    }

    ThingAction *findActionById(ThingDevice *device, String id){
        ThingAction *action = device->firstAction;
        while(action != nullptr){
            if(action->id == id){
                return action;
            }
            action = (ThingAction *)action->next;
        }
        return nullptr;
    }

    ThingEvent *findEventById(ThingDevice *device, String id){
        ThingEvent *event = device->firstEvent;
        while(event != nullptr){
            if(event->id == id){
                return event;
            }
            event = (ThingEvent *)event->next;
        }
        return nullptr;
    }

    // Begin method
    void begin(String websocketUrl, unint8_t websocketPort){

       // TODO - I don't fully understand this logic.
       ThingDevice *device  = this->firstDevice;
        // server address, port and URL
        webSocket.begin(websocketUrl, websocketPort);
        // event handler
        webSocket.onEvent(webSocketEvent);
    }

    // Update method
    void update(){
    
        #ifndef WITHOUT_WS
        // * Send changed properties as defined in "4.5 propertyStatus message"
        // Do this by looping over all devices and properties
        ThingDevice *device = this->firstDevice;
        while (device != nullptr) {
            sendChangedProperties(device);
            device = device->next;
        }
        #endif
    }

    // Add device method
    void addDevice(ThingDevice *device){

        // TODO - I don't fully understand this logic.
        if (this->lastDevice == nullptr) {
            this->firstDevice = device;
            this->lastDevice = device;
        } else {
            this->lastDevice->next = device;
            this->lastDevice = device;
        }

        #ifndef WITHOUT_WS
        // TODO - I don't fully understand this logic.
        // Initiate the websocket instance
        /* 
            - Create  a websocket instance.
            - Set the callback for the websocket.
            call back to be called :
                handleWS()
            - Add this websocket instance to the server.
        */
        #endif
    }

    void sendChangedProperties(ThingDevice *device) {
    // Prepare one buffer per device
        DynamicJsonDocument message(LARGE_JSON_DOCUMENT_SIZE);
        message["messageType"] = "propertyStatus";
        JsonObject prop = message.createNestedObject("data");
        bool dataToSend = false;
        ThingItem *item = device->firstProperty;
        while (item != nullptr) {
            ThingDataValue *value = item->changedValueOrNull();
            if (value) {
                dataToSend = true;
                item->serializeValue(prop);
            }
            item = item->next;
        }
        if (dataToSend) {
            String jsonStr;
            serializeJson(message, jsonStr);
            webSocket.sendTXT(jsonStr);
        }
    }

    // This is function is callback for `/things`
    void handleThings(){
        DynamicJsonDocument buf(LARGE_JSON_DOCUMENT_SIZE);
        JsonArray things = buf.to<JsonArray>();
        ThingDevice *device = this->firstDevice;
        while (device != nullptr) {
            JsonObject descr = things.createNestedObject();
            device->serialize(descr, ip, port);
            descr["href"] = "/things/" + device->id;
            device = device->next;   
        }
        String jsonStr;
        serializeJson(things, jsonStr);
        webSocket.sendTXT(jsonStr);
    }

    // This is function is callback for `/things/{thingId}`
    void handleThing(String thingId) {

        ThingDevice *device = findDeviceById(thingId);
        if (device == nullptr) {
            return "error";
        }
        DynamicJsonDocument buf(LARGE_JSON_DOCUMENT_SIZE);
        JsonObject descr = buf.to<JsonObject>();
        device->serialize(descr, ip, port);
        String jsonStr;
        serializeJson(descr, jsonStr);
        webSocket.sendTXT(jsonStr);
    }   

    // This is function is callback for GET `/things/{thingId}/properties`
    void handleThingPropertyGet(String thingId, String propertyId) {

        ThingDevice *device = findDeviceById(thingId);
        if (device == nullptr) {
            return "error";
        }
        ThingItem *item = findPropertyById(device, propertyId);
        if (item == nullptr) {
            return "item-error";
        }
        DynamicJsonDocument doc(SMALL_JSON_DOCUMENT_SIZE);
        JsonObject prop = doc.to<JsonObject>();
        item->serializeValue(prop);
        String jsonStr;
        serializeJson(prop, jsonStr);
        websocket.sendTXT(jsonStr);
    }

    // This is function is callback for GET `/things/{thingId}/actions/{actionId}`
    void handleThingActionGet(String thingId, String actionId) {
        ThingDevice *device = findDeviceById(thingId);
        if (device == nullptr) {
            return "error";
        }
        ThingAction *action = findActionById(device, actionId);
        if (action == nullptr) {
            return "action-error";
        }
        DynamicJsonDocument doc(LARGE_JSON_DOCUMENT_SIZE);
        JsonArray queue = doc.to<JsonArray>();
        device->serializeActionQueue(queue, action->id);
        String jsonStr;
        serializeJson(queue, jsonStr);
        rwebsocket.sendTXT(jsonStr);
    }

    // Delete action from queue
    void handleThingActionDelete(String thingId, String actionId){
        
        ThingDevice *device = findDeviceById(thingId);
        if (device == nullptr) {
            return;
        }
        device->removeAction(actionId);
    }

    // This function is callback for POST `/things/{thingId}/actions/{actionId}`
    void handleThingActionPost(String thingId, const char *newActionData, std::function<void(ThingActionObject *)> notify_fn_){

        /*
            This function takes a callback function as parameter.
            The function should be like this :
                void notify_fn(ThingActionObject *actionObject){
                    DynamicJsonDocument message(LARGE_JSON_DOCUMENT_SIZE);
                    message["messageType"] = "actionStatus";
                    JsonObject prop = message.createNestedObject("data");
                    action->serialize(prop, id);
                    String jsonStr;
                    serializeJson(message, jsonStr);
                } 
        */
        ThingDevice *device = findDeviceById(thingId);
        if (device == nullptr) {
            return;
        }
        DynamicJsonDocument *newBuffer =
        new DynamicJsonDocument(SMALL_JSON_DOCUMENT_SIZE);
        auto error = deserializeJson(*newBuffer, newActionData);
        if (error) {
            // send error as response
            Serial.print(F("[handleThingActionPost()] deserializeJson() failed with code "));
            Serial.println(error.c_str());
            return;
        }
        JsonObject newAction = newBuffer->as<JsonObject>();

        ThingActionObject *obj = device->requestAction(newBuffer);
        if (obj == nullptr) {
            // Send error as response
            Serial.println(F("[handleThingActionPost()] requestAction() failed. Obj was nullptr."));
            delete newBuffer;
            return;
        }

        obj->setNotifyFunction([](ThingActionObject *actionObject){
                    DynamicJsonDocument message(LARGE_JSON_DOCUMENT_SIZE);
                    message["messageType"] = "actionStatus";
                    JsonObject prop = message.createNestedObject("data");
                    action->serialize(prop, id);
                    String jsonStr;
                    serializeJson(message, jsonStr);
                    webSocket.sendTXT(jsonStr);
                });

        DynamicJsonDocument respBuffer(SMALL_JSON_DOCUMENT_SIZE);
        JsonObject item = respBuffer.to<JsonObject>();
        obj->serialize(item, device->id);
        String jsonStr;
        serializeJson(item, jsonStr);
        obj->start();
    }

    // This function is callback for GET `/things/{thingId}/events/{eventId}`
    void handleThingEventGet(String thingId, String eventId){
        ThingDevice *device = findDeviceById(thingId);
        if (device == nullptr) {
            return "error";
        }
        ThingItem *item = findEventById(device, eventId);
        if (item == nullptr) {
            return "item-error";
        }
        DynamicJsonDocument doc(LARGE_JSON_DOCUMENT_SIZE);
        JsonArray queue = doc.to<JsonArray>();
        device->serializeEventQueue(queue, item->id);
        String jsonStr;
        serializeJson(queue, jsonStr);
        websocket.sendTXT(jsonStr);
    }

    // This function is callback for GET `/things/{thingId}/properties`
    void handleThingPropertiesGet(String thingId){
        ThingItem *rootItem = findDeviceById(thingId)->firstProperty;
        if (rootItem == nullptr) {
            websocket.sendTXT("400");
        }
        DynamicJsonDocument doc(LARGE_JSON_DOCUMENT_SIZE);
        JsonObject prop = doc.to<JsonObject>();
        ThingItem *item = rootItem;
        while (item != nullptr) {
            item->serializeValue(prop);
            item = item->next;
        }
        String jsonStr;
        serializeJson(prop, jsonStr);
        websocket.sendTXT(jsonStr);
    }

    // This function is callback for POST `/things/{thingId}/actions`
    void handleThingActionsGet(String thingId){
        ThingDevice *device = findDeviceById(thingId);
        if (device == nullptr) {
            websocket.sendTXT("400");
        }
        DynamicJsonDocument doc(LARGE_JSON_DOCUMENT_SIZE);
        JsonArray queue = doc.to<JsonArray>();
        device->serializeActionQueue(queue);
        String jsonStr;
        serializeJson(queue, jsonStr);
        websocket.sendTXT(jsonStr);
    }

    // This function is callback for POST `/things/{thingId}/actions`
    void handleThingActionsPost(String thingId, const char *newActionData, std::function<void(ThingActionObject *)> notify_fn_){
        /*
            This function takes a callback function as parameter.
            The function should be like this :
                void notify_fn(ThingActionObject *actionObject){
                    DynamicJsonDocument message(LARGE_JSON_DOCUMENT_SIZE);
                    message["messageType"] = "actionStatus";
                    JsonObject prop = message.createNestedObject("data");
                    action->serialize(prop, id);
                    String jsonStr;
                    serializeJson(message, jsonStr);
                } 
        */

       ThingDevice *device = findDeviceById(thingId);
       if (device == nullptr) {
            websocket.sendTXT("400");
        }
        DynamicJsonDocument *newBuffer =
        new DynamicJsonDocument(SMALL_JSON_DOCUMENT_SIZE);
        auto error = deserializeJson(*newBuffer, newActionData);
        if (error) {
            // send error as response
            Serial.print(F("[handleThingActionPost()] deserializeJson() failed with code "));
            Serial.println(error.c_str());
            return;
        }
        JsonObject newAction = newBuffer->as<JsonObject>();

        ThingActionObject *obj = device->requestAction(newBuffer);
        if (obj == nullptr) {
            // Send error as response
            Serial.println(F("[handleThingActionPost()] requestAction() failed. Obj was nullptr."));
            delete newBuffer;
            websocket.sendTXT("400");
        }

        obj->setNotifyFunction([](ThingActionObject *actionObject){
                    DynamicJsonDocument message(LARGE_JSON_DOCUMENT_SIZE);
                    message["messageType"] = "actionStatus";
                    JsonObject prop = message.createNestedObject("data");
                    action->serialize(prop, id);
                    String jsonStr;
                    serializeJson(message, jsonStr);
                    webSocket.sendTXT(jsonStr);
                });

        DynamicJsonDocument respBuffer(SMALL_JSON_DOCUMENT_SIZE);
        JsonObject item = respBuffer.to<JsonObject>();
        obj->serialize(item, device->id);
        String jsonStr;
        serializeJson(item, jsonStr);
        obj->start();
        websocket.sendTXT(jsonStr);
    }

    // This function is callback for GET `/things/{thingId}/events`
    String handleThingEventsGet(String thingId) {
        ThingDevice *device = findDeviceById(thingId);
        if (device == nullptr) {
            return "error";
        }
        DynamicJsonDocument doc(LARGE_JSON_DOCUMENT_SIZE);
        JsonArray queue = doc.to<JsonArray>();
        device->serializeEventQueue(queue);
        String jsonStr;
        serializeJson(queue, jsonStr);
        websocket.sendTXT(jsonStr);
    }

    // This function was used to call handleBody for all requests
    void handleBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        if (total >= ESP_MAX_PUT_BODY_SIZE || index + len >= ESP_MAX_PUT_BODY_SIZE) {
            return; // cannot store this size..
        }
        // copy to internal buffer
        memcpy(&body_data[index], data, len);
        b_has_body_data = true;
    }

    // Most important, Most used function.
    // This function is callback for PUT `/things/{thingId}/properties/{propertyId}`
    void handleThingPropertyPut(ThingDevice *device, ThingProperty *property){
        DynamicJsonDocument newBuffer(SMALL_JSON_DOCUMENT_SIZE);
        auto error = deserializeJson(newBuffer, body_data);
        if (error) { 
            // unable to parse json
            b_has_body_data = false;
            memset(body_data, 0, sizeof(body_data));
            // TODO - Return error to the client [httpCode = 400].
            return;
        }

        // create a json objectfrom the newBuffer (Which holds the body data)
        JsonObject newProp = newBuffer.as<JsonObject>();

        // check if the json object already contains the property id
        // if it does not contain the property id, then return error
        // Beacause then body is not valid for the passed property.
        // This logic is not very usefull to us, as we will directly pass
        // the body data which is already parsed.
        if (!newProp.containsKey(property->id)) {
            b_has_body_data = false;
            memset(body_data, 0, sizeof(body_data));
            // request->send(400);
            return;
        }

        // Set the property of the device to the new value
        // Let's say body data is {"on": true}
        // We will set the property to true as
        //  device->setProperty("some-device-id", newProp["on"]);
        device->setProperty(property->id.c_str(), newProp[property->id]);

        // TODO - Return success and updated property value `JsonObject newProp` [httpCode = 200].

        // clear body data and set body data flag
        
        b_has_body_data = false;
        memset(body_data, 0, sizeof(body_data));
    }

    void handleThingPropertyPutV2(String thingId, String propertyId, const char *newPropertyData){


        ThingDevice *device = findDeviceById(thingId);
        if (device == nullptr) {
            return;
        }
        ThingProperty *property = findPropertyById(propertyId);
        if (property == nullptr) {
            return;
        }
        DynamicJsonDocument newBuffer(SMALL_JSON_DOCUMENT_SIZE);
        auto error = deserializeJson(newBuffer, newPropertyData);

        if (error) { 
            // unable to parse json
            Serial.println("Unable to parse json for property PUT request V2");
            return;
        }

        JsonObject newProp = newBuffer.as<JsonObject>();
        device->setProperty(property->id.c_str(), newProp[property->id]);
        String jsonStr;
        serializeJson(newProp, jsonStr);
        websocket.sendTXT(jsonStr);
    }

};

#endif // ESP