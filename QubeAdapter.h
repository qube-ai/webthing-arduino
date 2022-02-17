#pragma once

#define ESP32 1

#if defined(ESP32) || defined(ESP8266)

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include "Thing.h"

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

    // Constructor
    QubeAdapter(String _name, IPAddress _ip, uint16_t _port = 80,
                  bool _disableHostValidation = false)
      : name(_name), ip(_ip.toString()), port(_port),
        disableHostValidation(_disableHostValidation) {}
    
    String name;

    /******* Even though we don't need these variables, but to 
     be compatible with Thing.h we'll use this. *******/
    String ip;
    uint16_t port;
    /*********************/

    bool disableHostValidation;
    ThingDevice *firstDevice = nullptr;
    ThingDevice *lastDevice = nullptr;
    // Here all the body data in PUT and POST requests are stored.
    char body_data[ESP_MAX_PUT_BODY_SIZE];
    // If we fill above buffer this flag will be set to true.
    bool b_has_body_data = false;


    ThingDevice *findDeviceById(String id){
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
            property = property->next;
        }
        return nullptr;
    }

    ThingAction *findActionById(ThingDevice *device, String id){
        ThingAction *action = device->firstAction;
        while(action != nullptr){
            if(action->id == id){
                return action;
            }
            action = action->next;
        }
        return nullptr;
    }

    ThingEvent *findEventById(ThingDevice *device, String id){
        ThingEvent *event = device->firstEvent;
        while(event != nullptr){
            if(event->id == id){
                return event;
            }
            event = event->next;
        }
        return nullptr;
    }


    // Begin method
    void begin(){

       // TODO - I don't fully understand this logic.
       ThingDevice *device  = this->firstDevice;

         while(device != nullptr){
             
             // Create an endpoint with /things/{thingId}
            String deviceBase = "/things/" + device->id;
            ThingProperty *property = device->firstProperty;

            while (property != nullptr) {
                String propertyBase = deviceBase + "/properties/" + property->id;
                /* 
                    Here attach some callbacks to the endpoints. Since we are not using servers we will call these callbacks directly.
                    2 Callbacks are to be attached:
                        - GET : handleThingPropertyGet()
                        - PUT : handleThingPropertyPut()
                        To obtain body for put method use:
                            handleBody()
                */

               property = (ThingProperty *)property->next;
            }

            ThingAction *action = device->firstAction;
            while (action != nullptr) {
                String actionBase = deviceBase + "/actions/" + action->id;
                /* 
                    Here attach some callbacks to the endpoints. Since we are not using servers we will call these callbacks directly.
                    2 Callbacks are to be attached:
                        - POST : handleThingActionPost()
                        - GET : handleThingActionGet()
                        To obtain body for post method use:
                            handleBody()
                        - DELETE : handleThingActionDelete()
                */

                action = (ThingAction *)action->next;
            }

            ThingEvent *event = device->firstEvent;
            while (event != nullptr) {
                String eventBase = deviceBase + "/events/" + event->id;
                /* 
                    Here attach some callbacks to the endpoints. Since we are not using servers we will call these callbacks directly.
                    2 Callbacks are to be attached:
                        - GET : handleThingEventGet()
                */

                event = (ThingEvent *)event->next;
            }

            /* 
                Here attach callbacks for getting all thingDescription, actions, properties and events.
                Since we are not using servers we will call these callbacks directly.
                5 Callbacks to be attached to:
                    - GET : handleThingPropertiesGet() => /things/{thingId}/properties
                    - GET : handleThingActionsGet() => /things/{thingId}/actions
                    - POST : handleThingActionsPost() => /things/{thingId}/actions
                        To obtain body for post method use:
                            - handleBody()
                    - GET : handleThingEventsGet() => /things/{thingId}/events
                    - GET : handleThing() => /things/{thingId} : To get thingDescription
            **/

           device = device->next;

         }

         // Here start the server

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
            Serial.println("Soemthing changed");
            Serial.println(jsonStr);
            // Inform all connected ws clients of a Thing about changed properties
            // ((AsyncWebSocket *)device->ws)->textAll(jsonStr);
        }
    }


    // This is function is callback for `/things`
    String handleThings(){
        /* 
            This function has @param AsyncWebServerRequest *request
            but as we will call this function directly we dont need to
            pass this request object.
        */
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
        return jsonStr;
    }

    ThingDevice 

    // This is function is callback for `/things/{thingId}`
    String handleThing() {
        ThingDevice *device = this->firstDevice;
        DynamicJsonDocument buf(LARGE_JSON_DOCUMENT_SIZE);
        JsonObject descr = buf.to<JsonObject>();
        device->serialize(descr, ip, port);
        String jsonStr;
        serializeJson(descr, jsonStr);
        return jsonStr;
    }   

    // This is function is callback for GET `/things/{thingId}/properties`
    String handleThingPropertyGet(){
        ThingItem *item = this->firstDevice->firstProperty;
        DynamicJsonDocument doc(SMALL_JSON_DOCUMENT_SIZE);
        JsonObject prop = doc.to<JsonObject>();
        item->serializeValue(prop);
        String jsonStr;
        serializeJson(prop, jsonStr);
        return jsonStr;
    }

    // This is function is callback for GET `/things/{thingId}/actions/{actionId}`
    String handleThingActionGet(){
        ThingDevice *device = this->firstDevice;
        ThingAction *action = this->firstDevice->firstAction;
        DynamicJsonDocument doc(LARGE_JSON_DOCUMENT_SIZE);
        JsonArray queue = doc.to<JsonArray>();
        device->serializeActionQueue(queue, action->id);
        String jsonStr;
        serializeJson(queue, jsonStr);
        return jsonStr;
    }

    // Add action delete method here
    void handleThingActionDelete(ThingDevice *device, String actionId){
        device->removeAction(actionId);
    }

    // This is function is callback for POST `/things/{thingId}/actions/{actionId}`
    void handleThingActionPost(const char *newActionData, std::function<void(ThingActionObject *)> notify_fn_){

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
       ThingDevice *device = this->firstDevice;
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

        obj->setNotifyFunction(notify_fn_);

        DynamicJsonDocument respBuffer(SMALL_JSON_DOCUMENT_SIZE);
        JsonObject item = respBuffer.to<JsonObject>();
        obj->serialize(item, device->id);
        String jsonStr;
        serializeJson(item, jsonStr);
        obj->start();
    }

    // This is function is callback for GET `/things/{thingId}/events/{eventId}`
    String handleThingEventGet(){
        ThingDevice *device = this->firstDevice;
        ThingItem *item = this->firstDevice->firstEvent;
        DynamicJsonDocument doc(LARGE_JSON_DOCUMENT_SIZE);
        JsonArray queue = doc.to<JsonArray>();
        device->serializeEventQueue(queue, item->id);
        String jsonStr;
        serializeJson(queue, jsonStr);
        return jsonStr;
    }

    // This is function is callback for GET `/things/{thingId}/properties`
    String handleThingPropertiesGet(){

        /* 
            - This function has @param `ThingItem *rootItem` which i do not
            understand. In the callback above  `*property` is passed.

            In in ESPWebThingAdapter.h

            param property is defined on line 84 :
            ThingDevice *device = this->firstDevice;

            and on line 87  the callback is defined:
            """
            (propertyBase.c_str(), HTTP_GET,
                        std::bind(&WebThingAdapter::handleThingPropertyGet,
                                  this, std::placeholders::_1, property))
            """ 

            Thus i can use that to create a ThingItem inside the function
            body and should still point to the same object. Like this:
            ThingItem *item = device->firstProperty; 
        */

        ThingItem *rootItem = this->firstDevice->firstProperty;
        DynamicJsonDocument doc(LARGE_JSON_DOCUMENT_SIZE);
        JsonObject prop = doc.to<JsonObject>();
        ThingItem *item = rootItem;
        while (item != nullptr) {
            item->serializeValue(prop);
            item = item->next;
        }
        String jsonStr;
        serializeJson(prop, jsonStr);
        return jsonStr;
    }

    // This is function is callback for POST `/things/{thingId}/actions`
    String handleThingActionsGet(){
        ThingDevice *device = this->firstDevice;
        DynamicJsonDocument doc(LARGE_JSON_DOCUMENT_SIZE);
        JsonArray queue = doc.to<JsonArray>();
        device->serializeActionQueue(queue);
        String jsonStr;
        serializeJson(queue, jsonStr);
        return jsonStr;
    }

    // This is function is callback for POST `/things/{thingId}/actions`
    void handleThingActionsPost(const char *newActionData, std::function<void(ThingActionObject *)> notify_fn_){
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

       ThingDevice *device = this->firstDevice;
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

        obj->setNotifyFunction(notify_fn_);

        DynamicJsonDocument respBuffer(SMALL_JSON_DOCUMENT_SIZE);
        JsonObject item = respBuffer.to<JsonObject>();
        obj->serialize(item, device->id);
        String jsonStr;
        serializeJson(item, jsonStr);
        obj->start();
    }

    // This is function is callback for GET `/things/{thingId}/events`
    String handleThingEventsGet() {
        ThingDevice *device = this->firstDevice;
        DynamicJsonDocument doc(LARGE_JSON_DOCUMENT_SIZE);
        JsonArray queue = doc.to<JsonArray>();
        device->serializeEventQueue(queue);
        String jsonStr;
        serializeJson(queue, jsonStr);
        return jsonStr;
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
    // This is function is callback for PUT `/things/{thingId}/properties/{propertyId}`
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

    void handleThingPropertyPutV2(ThingDevice *device, ThingProperty *property, const char *newPropertyData){
        DynamicJsonDocument newBuffer(SMALL_JSON_DOCUMENT_SIZE);
        auto error = deserializeJson(newBuffer, newPropertyData);

        if (error) { 
            // unable to parse json
            Serial.println("Unable to parse json for property PUT request V2");
            return;
        }

        JsonObject newProp = newBuffer.as<JsonObject>();
        device->setProperty(property->id.c_str(), newProp[property->id]);
    }

};

#endif // ESP