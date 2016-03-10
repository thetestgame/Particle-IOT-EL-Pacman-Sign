/*
The MIT License (MIT)

Copyright (c) 2016 Jordan Maxwell

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "MQTT/MQTT.h"

/***********************************
 * general settings definitions
 * *********************************/

//Light pin
#define POWER_PIN 0

//Audio pins
#define START_PIN 1
#define LOSE_PIN 2
#define JINGLE_PIN 3

int powered = 0;

/***********************************
 * particle settings definitions
 * *********************************/ 

// PARTICLE RESTFUL API

//Enable/Disable Particle cloud breakout api
bool particleApi = true; 

// PARTICLE FEEDS
 //Enable/Disable Particle cloud feed functionality
bool particleFeeds = false;

//Particle feeds require particleFeeds to be set to true
String particlePowerFeed = "";
String particleAudioFeed = "";
 
/***********************************
 * MQTT settings definitions
 * *********************************/ 
 
//Enable/Disable MQTT functionality 
bool mqttEnabled = true;
char* mqttHost = "io.adafruit.com";
int mqttPort = 1883;
String mqttUsername = "thetestgame";
String mqttPassword = "9b95a6a0d4849520af14a250dd2b8f451f246eae";
//MQTT feeds require mqttEnable to be set to true
String mqttPowerFeed = "";
String mqttAudioFeed = "";

//Do not change. MQTT client declaration
void MQTTcallback(char* topic, byte* payload, unsigned int length);
MQTT client(mqttHost, mqttPort, MQTTcallback);

/**************************************
 * startup and loop functions *
 * ************************************/

void setup() {
    Serial.begin(9600);
    pinMode(POWER_PIN, OUTPUT);
    initMQTT();
    initParticleVariables();
}

void loop() {
    if (client.isConnected()) {
        client.loop(); 
    } else {
        initMQTT();
    }

    if (particleFeeds || particleApi) {
        Particle.process();
    }   
    digitalWrite(POWER_PIN, powered);
}

/***********************************
 * debug functions and definitions *
 * *********************************/
 
void debug(String message) {
#ifdef SERIAL_DEBUG
    Serial.print(message);
#endif
}

void debugln(String message) {
#ifdef SERIAL_DEBUG
    Serial.println(message);
#endif
}

/***********************************
 * audio functions and definitions *
 * *********************************/

void playAudio(int pin) {
    digitalWrite(pin, HIGH);
    delay(1000);
    digitalWrite(pin, LOW);
}

/**************************************
 * MQTT API functions and definitions *
 * ************************************/

void initMQTT() {
    if (mqttEnabled) {
        client.connect(mqttHost, mqttUsername, mqttPassword);   
        if (client.isConnected()) {
            MQTTSubscribe(mqttPowerFeed);
            MQTTSubscribe(mqttAudioFeed);
        } else {
            debugln("Failed to initalize MQTT services!");
        }
    }
}

void MQTTSubscribe(String feed) {
    if (feed != "") {
        client.subscribe(feed);
    }
}

void MQTTcallback(char* topic, byte* payload, unsigned int length) {
    char p[length + 1];
    memcpy(p, payload, length);
    p[length] = NULL;
    String message(p);
    debugln("Received MQTT data (" + message + ") from " + topic);
    String feed(topic);
    if (feed.equals(mqttPowerFeed)) {
        powered = message.toInt();
        if (powered > 1)
            powered = 1;
        if (powered < 0 )
            powered = 0;
    }
    
    if (feed.equals(mqttAudioFeed)) {
        int pin = message.toInt();
        playAudio(pin);
    }
}

bool publishMQTT(String feed, String payload) {
    if (feed == "")
        return false;
    if (!mqttEnabled) 
        return false;
    if (!client.isConnected()) {
        debugln("Failed to publish to mqtt feed (" + feed + "). No MQTT connection.");
        return false;
    }
    client.publish(feed, payload);
    return true;
}

/******************************************
 * Particle API functions and definitions *
 * ***************************************/
 
//initializes the shared variables and functions that are accessible through the particle API
void initParticleVariables() {
    if (particleApi) {
        debugln("Initializing Particle cloud callbacks...");
        
        //Particle Cloud API variable defintions
        Particle.variable("powered", &powered, INT);
        
        //Particle Cloud API function callback defintions
        Particle.function("setPower", (int (*)(String)) setPower);
    }
    
    if (particleFeeds) {
        debugln("Initializing Particle cloud feed subscriptions...");
        if (particlePowerFeed != "")
            Particle.subscribe(particlePowerFeed, ParticleHandler);
            Particle.subscribe(particleAudioFeed, ParticleHandler);
    }
    
    if (particleFeeds || particleApi)
        debugln("Particle cloud ready!");
    Particle.connect();
}

int setPower(String incoming) {
    powered = incoming.toInt();
    return powered;
}

void ParticleHandler(const char *event, const char *data) {
    String e(event);
    String d(event);
    if (e == particlePowerFeed) {
        powered = d.toInt();
        if (powered > 1)
            powered = 1;
        if (powered < 0 )
            powered = 0;
    }
 
    if (e == particleAudioFeed) {
        playAudio(d.toInt());
    }   
}

bool publishParticle(String feed, String payload) {
    if (feed == "")
        return false;
    if (!particleFeeds) 
        return false;
    if (!Particle.connected()) {
        debugln("Failed to publish to Particle feed (" + feed + "). No Particle Cloud connection.");
        return false;
    }
    Particle.publish(feed, payload);
    return true;
}
