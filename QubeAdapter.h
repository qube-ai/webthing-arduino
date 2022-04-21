#pragma once


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

    void sendMessage(String & msg) {
        webSocket.sendTXT(msg.c_str(), msg.length() + 1);
    }


    void messageHandler(String payload){
        
        DynamicJsonDocument doc(LARGE_JSON_DOCUMENT_SIZE);
        DeserializationError error = deserializeJson(doc, payload);
        if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.c_str());
            String msg = "{\"messageType\":\"error\", \"errorMessage\":\"deserializeJson() failed \"}";
            sendMessage(msg);
        }

        JsonObject root = doc.as<JsonObject>();

        if (root["messageType"] == "getProperty") {
            String thingId = root["thingId"];
            handleThingPropertiesGet(thingId);
        }

        if (root["messageType"] == "setProperty") {
            String thingId = root["thingId"];
            String propertyId = root["data"]["propertyId"];
            Serial.println("messageType setProperty was called");
            Serial.println("Going to change the property");
            String data  = root["data"];
            handleThingPropertyPut(thingId, propertyId, data);
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
            handleThingActionPost(thingId, (const char*)root["data"]);
        }

        else {
            Serial.println("Unknown message type");
            Serial.println(root);
            Serial.println(payload);
            // String msg = "{\"messageType\":\"error\", \"errorMessage\":\"unknown messageType \"}";
            // sendMessage(msg);
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
        String msg = msgch;
        messageHandler(msg);
    }

    void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
    {
        switch (type)
        {
        case WStype_DISCONNECTED:
            // Serial.printf("[WSc] Disconnected!\n");
            break;

        case WStype_CONNECTED:
            Serial.printf("[WSc] Connected to url: %s\n", payload);
            webSocket.sendTXT("{\"messageType\":\"StartWs\"}");
            break;

        case WStype_TEXT:
            payloadHandler(payload, length);
            break;

        case WS_EVT_DATA:
            Serial.printf("[WSc] get binary length: %u\n", length);
            break;
        
        case WStype_ERROR:
            Serial.printf("[WSc] Error!\n");
            break;
        
        case WStype_FRAGMENT_TEXT_START:
            Serial.printf("[WSc] Fragment Text Start!\n");
            break;
        
        case WStype_FRAGMENT_BIN_START:
            Serial.printf("[WSc] Fragment Bin Start!\n");
            break;

        case WStype_FRAGMENT:
            Serial.printf("[WSc] Fragment!\n");
            break;
        
        case WStype_FRAGMENT_FIN:
            Serial.printf("[WSc] Fragment Fin!\n");
            break;

        case WStype_PING:
            Serial.printf("[WSc] Ping!\n");
            break;
        
        case WStype_PONG:
            Serial.printf("[WSc] Pong!\n");
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
    void begin(String websocketUrl, int websocketPort, String websocketPath) {

        #ifndef WSS_ENABLED
        webSocket.beginSSL(websocketUrl, websocketPort, websocketPath);
        #else
        // server address, port and URL
        webSocket.begin(websocketUrl, websocketPort, websocketPath);
        #endif
        // event handler
        webSocket.onEvent(std::bind(
        &QubeAdapter::webSocketEvent, this,std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3));
    }

    // Update method
    void update(){
        
        webSocket.loop();
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
            message["thingId"] = device->id;
            serializeJson(message, jsonStr);
            sendMessage(jsonStr);
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
        StaticJsonDocument<LARGE_JSON_DOCUMENT_SIZE> doc;
        JsonObject doc2 = doc.to<JsonObject>();
        doc2["messageType"] = "descriptionOfThings";
        doc2["things"] = things;
        String jsonStr;
        serializeJson(doc2, jsonStr);
        sendMessage(jsonStr);
    }

    // This is function is callback for `/things/{thingId}`
    void handleThing(String thingId) {

        ThingDevice *device = findDeviceById(thingId);
        if (device == nullptr) {
            String msg = "{\"messageType\":\"error\",\"errorCode\":\"404\",\"errorMessage\":\"Thing not found\", \"thingId\": \"" + thingId + "\"}";
            sendMessage(msg);
        }
        DynamicJsonDocument buf(LARGE_JSON_DOCUMENT_SIZE);
        JsonObject descr = buf.to<JsonObject>();
        device->serialize(descr, ip, port);
        String jsonStr;
        serializeJson(descr, jsonStr);
        sendMessage(jsonStr);
    }   

    // This is function is callback for GET `/things/{thingId}/properties`
    void handleThingPropertyGet(String thingId, String propertyId) {

        ThingDevice *device = findDeviceById(thingId);
        if (device == nullptr) {
            String msg = "{\"messageType\":\"error\",\"errorCode\":\"404\",\"errorMessage\":\"Thing not found\", \"thingId\": \"" + thingId + "\"}";
           sendMessage(msg);
        }
        ThingItem *item = findPropertyById(device, propertyId);
        if (item == nullptr) {
            String msg = "{\"messageType\":\"error\",\"errorCode\":\"404\",\"errorMessage\":\"Property not found\", \"thingId\": \"" + thingId + "\", \"propertyId\": \"" + propertyId + "\"}";
           sendMessage(msg);
        }
        DynamicJsonDocument doc(SMALL_JSON_DOCUMENT_SIZE);
        JsonObject prop = doc.to<JsonObject>();
        item->serializeValue(prop);
        DynamicJsonDocument finalDoc(SMALL_JSON_DOCUMENT_SIZE);
        JsonObject finalProp = finalDoc.to<JsonObject>();
        finalDoc["messageType"] = "getProperty";
        finalDoc["thingId"] = thingId;
        finalDoc["properties"] = prop;
        String jsonStr;
        serializeJson(finalProp, jsonStr);
        sendMessage(jsonStr);
    }

    // This is function is callback for GET `/things/{thingId}/actions/{actionId}`
    void handleThingActionGet(String thingId, String actionId) {
        ThingDevice *device = findDeviceById(thingId);
        if (device == nullptr) {
            String msg = "{\"messageType\":\"error\",\"errorCode\":\"404\",\"errorMessage\":\"Thing not found\", \"thingId\": \"" + thingId + "\"}";
           sendMessage(msg);
        }
        ThingAction *action = findActionById(device, actionId);
        if (action == nullptr) {
            String msg = "{\"messageType\":\"error\",\"errorCode\":\"404\",\"errorMessage\":\"Action not found\", \"thingId\": \"" + thingId + "\", \"actionId\": \"" + actionId + "\"}";
           sendMessage(msg);
        }
        DynamicJsonDocument doc(LARGE_JSON_DOCUMENT_SIZE);
        JsonArray queue = doc.to<JsonArray>();
        device->serializeActionQueue(queue, action->id);
        String jsonStr;
        serializeJson(queue, jsonStr);
        sendMessage(jsonStr);
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
    void handleThingActionPost(String thingId, const char *newActionData){
        ThingDevice *device = findDeviceById(thingId);
        if (device == nullptr) {
            return;
        }
        DynamicJsonDocument *newBuffer =
        new DynamicJsonDocument(SMALL_JSON_DOCUMENT_SIZE);
        auto error = deserializeJson(*newBuffer, newActionData);
        if (error) {
            // send error as response
            Serial.print(F("[handleThingActionsPost()] deserializeJson() failed with code "));
            Serial.println(error.c_str());
            return;
        }
        // JsonObject newAction = newBuffer->as<JsonObject>();

        ThingActionObject *obj = device->requestAction(newBuffer);
        if (obj == nullptr) {
            // Send error as response
            Serial.println(F("[handleThingActionsPost()] requestAction() failed. Obj was nullptr."));
            delete newBuffer;
            return;
        }

        // TODO add notify func
        // obj->setNotifyFunction([](ThingActionObject *action){
        //             DynamicJsonDocument message(LARGE_JSON_DOCUMENT_SIZE);
        //             message["messageType"] = "actionStatus";
        //             message["thingId"] = action->device->id;
        //             JsonObject prop = message.createNestedObject("data");
        //             action->serialize(prop, device->id);
        //             String jsonStr;
        //             serializeJson(message, jsonStr);
        //             sendMessage(jsonStr);
        //         });

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
            String msg = "{\"messageType\":\"error\",\"errorCode\":\"404\",\"errorMessage\":\"Thing not found\", \"thingId\": \"" + thingId + "\"}";
           sendMessage(msg);
        }
        ThingItem *item = findEventById(device, eventId);
        if (item == nullptr) {
            String msg = "{\"messageType\":\"error\",\"errorCode\":\"404\",\"errorMessage\":\"Event not found\", \"thingId\": \"" + thingId + "\", \"eventId\": \"" + eventId + "\"}";
           sendMessage(msg);
        }
        DynamicJsonDocument doc(LARGE_JSON_DOCUMENT_SIZE);
        JsonArray queue = doc.to<JsonArray>();
        device->serializeEventQueue(queue, item->id);
        String jsonStr;
        serializeJson(queue, jsonStr);
        sendMessage(jsonStr);
    }

    // This function is callback for GET `/things/{thingId}/properties`
    void handleThingPropertiesGet(String thingId){
        ThingItem *rootItem = findDeviceById(thingId)->firstProperty;
        if (rootItem == nullptr) {
            String msg = "{\"messageType\":\"error\",\"errorCode\":\"404\",\"errorMessage\":\"Thing not found\", \"thingId\": \"" + thingId + "\"}";
           sendMessage(msg);
        }
        DynamicJsonDocument doc(LARGE_JSON_DOCUMENT_SIZE);
        JsonObject prop = doc.to<JsonObject>();
        ThingItem *item = rootItem;
        while (item != nullptr) {
            item->serializeValue(prop);
            item = item->next;
        }
        DynamicJsonDocument finalDoc(SMALL_JSON_DOCUMENT_SIZE);
        JsonObject finalProp = finalDoc.to<JsonObject>();
        finalDoc["messageType"] = "getProperty";
        finalDoc["thingId"] = thingId;
        finalDoc["properties"] = prop;
        String jsonStr;
        serializeJson(finalProp, jsonStr);
        sendMessage(jsonStr);
    }

    // This function is callback for POST `/things/{thingId}/actions`
    void handleThingActionsGet(String thingId){
        ThingDevice *device = findDeviceById(thingId);
        if (device == nullptr) {
            String msg = "{\"messageType\":\"error\",\"errorCode\":\"404\",\"errorMessage\":\"Thing not found\", \"thingId\": \"" + thingId + "\"}";
           sendMessage(msg);
        }
        DynamicJsonDocument doc(LARGE_JSON_DOCUMENT_SIZE);
        JsonArray queue = doc.to<JsonArray>();
        device->serializeActionQueue(queue);
        String jsonStr;
        serializeJson(queue, jsonStr);
        sendMessage(jsonStr);
    }

    // This function is callback for POST `/things/{thingId}/actions`
    void handleThingActionsPost(String thingId, const char *newActionData){
       ThingDevice *device = findDeviceById(thingId);
       if (device == nullptr) {
            String msg = "{\"messageType\":\"error\",\"errorCode\":\"404\",\"errorMessage\":\"Thing not found\", \"thingId\": \"" + thingId + "\"}";
           sendMessage(msg);
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
        // JsonObject newAction = newBuffer->as<JsonObject>();

        ThingActionObject *obj = device->requestAction(newBuffer);
        if (obj == nullptr) {
            // Send error as response
            Serial.println(F("[handleThingActionPost()] requestAction() failed. Obj was nullptr."));
            delete newBuffer;
            String msg = "{\"messageType\":\"error\",\"errorCode\":\"404\",\"errorMessage\":\"Request action was null ptr.\", \"thingId\": \"" + thingId + "\", \"actionId\": \"" + obj->id + "\"}";
           sendMessage(msg);
        }

        // TODO add notify_fn_
        // obj->setNotifyFunction([](ThingActionObject *action){
        //             DynamicJsonDocument message(LARGE_JSON_DOCUMENT_SIZE);
        //             message["messageType"] = "actionStatus";
        //             message["thingId"] = device->id;
        //             JsonObject prop = message.createNestedObject("data");
        //             action->serialize(prop, device->id);
        //             String jsonStr;
        //             serializeJson(message, jsonStr);
        //             this->sendMessage(jsonStr);
        //         });

        DynamicJsonDocument respBuffer(SMALL_JSON_DOCUMENT_SIZE);
        JsonObject item = respBuffer.to<JsonObject>();
        obj->serialize(item, device->id);
        String jsonStr;
        serializeJson(item, jsonStr);
        obj->start();
        sendMessage(jsonStr);
    }

    // This function is callback for GET `/things/{thingId}/events`
    void handleThingEventsGet(String thingId) {
        ThingDevice *device = findDeviceById(thingId);
        if (device == nullptr) {
            String msg = "{\"messageType\":\"error\",\"errorCode\":\"404\",\"errorMessage\":\"Thing not found\", \"thingId\": \"" + thingId + "\"}";
           sendMessage(msg);
        }
        DynamicJsonDocument doc(LARGE_JSON_DOCUMENT_SIZE);
        JsonArray queue = doc.to<JsonArray>();
        device->serializeEventQueue(queue);
        String jsonStr;
        queue[0]["data"]["thingId"] = thingId;
        serializeJson(queue, jsonStr);
        sendMessage(jsonStr);
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

    // This function is callback for PUT `/things/{thingId}/properties/{propertyId}`
    void handleThingPropertyPut(String thingId, String propertyId, String newPropertyData){


        ThingDevice *device = findDeviceById(thingId);
        if (device == nullptr) {
            return;
        }
        ThingProperty *property = findPropertyById(device, propertyId);
        if (property == nullptr) {
            return;
        }
        DynamicJsonDocument newBuffer(LARGE_JSON_DOCUMENT_SIZE);
        auto error = deserializeJson(newBuffer, newPropertyData);

        if (error) { 
            // unable to parse json
            Serial.println("Unable to parse json for property PUT request V2");
            Serial.println(error.c_str());
            // serializeJsonPretty(newPropertyData, Serial);
            return;
        }
        newBuffer["thingId"] = thingId;
        newBuffer["messageType"] = "updatedProperty";
        JsonObject newProp = newBuffer.as<JsonObject>();
        device->setProperty(property->id.c_str(), newProp["value"]);
        String jsonStr;
        serializeJson(newProp, jsonStr);
        sendMessage(jsonStr);
    }

};

#endif // ESP