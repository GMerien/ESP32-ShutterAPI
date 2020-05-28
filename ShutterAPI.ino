#include <ESP32Servo.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <string.h>

const char* ssid = "wifi";
const char* pass = "pass";

const int maxTry = 10;
int nbTry = 0;

AsyncWebServer server(80);

const int PUSHBUTTON = 2; 
const int SERVOPIN = 12;

const int CLOSEDANGLE = 8;
const int OPENANGLE = 76;

Servo servo;

int shutterOpenBool = 1;

int pressedButton = 0;
int lastPressedButton = 0;

bool takingPicture = false;

long currentTime;
long pictureTimer;
long pictureDuration;

int isShutterOpen() {
  return shutterOpenBool;
}

void shutterClose() {
  if(isShutterOpen()){
    shutterOpenBool = 0;
    servo.write(CLOSEDANGLE);
    Serial.println("closing");
  }
}

void shutterOpen() {
  if(!isShutterOpen()){
    shutterOpenBool = 1;
    servo.write(OPENANGLE);
    Serial.println("opening");
  }
}

void shutterTakePicture(long duration) {
  if(duration > 0 && takingPicture == false) {
    shutterOpen();
    takingPicture = true;
    pictureTimer = currentTime;
    pictureDuration = currentTime + duration;
  }
}

void addHeader(AsyncWebServerResponse *response) {
  response->addHeader("Access-Control-Allow-Origin", "*");
  response->addHeader("Access-Control-Expose-Headers", "*");
  response->addHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
}

void setup() {
  // initialize the serial port:
  Serial.begin(115200);

  servo.attach(SERVOPIN);
  servo.setPeriodHertz(50);
  pinMode(PUSHBUTTON, INPUT);

  servo.write(OPENANGLE);

  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED && nbTry < maxTry) {
    delay(500);
    Serial.println("Connecting to wifi ...");
    nbTry ++;
  }

  Serial.println("Connected to wifi :\t");
  Serial.println(WiFi.localIP());

  server.on("/shutter/connected", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "connected");
    addHeader(response);
    request -> send(response);
  });

  server.on("/shutter/open", HTTP_POST, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "openned");
    addHeader(response);
    request -> send(response);
    shutterOpen();
  });

  server.on("/shutter/close", HTTP_POST, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "closed");
    addHeader(response);
    request -> send(response);
    shutterClose();
  });

  server.on("/shutter/take", HTTP_POST, [](AsyncWebServerRequest *request){
    long duration;

    int paramsNbr = request->params();

    for (int i=0; i<paramsNbr; i++) {
      AsyncWebParameter* p = request->getParam(i);

      if (p->name().equals("duration")) {
        duration = ((p->value().toInt()) * 1000) - 200;
      }
    }

    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "takingPicture");
    addHeader(response);
    request -> send(response);
    shutterTakePicture(duration);
  });

  server.begin();
}

void loop() {
  currentTime = millis();

  if (takingPicture == true) {
    pictureTimer = currentTime;

    if (pictureTimer > pictureDuration) {
      pictureTimer = 0;
      takingPicture = false;
      shutterClose();
    }
  }


}
