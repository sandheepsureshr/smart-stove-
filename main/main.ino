#include <TFT_eSPI.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "base64.h"


TFT_eSPI tft;
Servo gasServo;
Servo cutServo;

// ---------- Pins ----------
#define BTN_UP     32
#define BTN_DOWN   33
#define BTN_LEFT   25
#define BTN_RIGHT  26
#define BTN_OK     27
#define SERVO_PIN  19
#define BUZZER     12
// ---------- GAS SENSOR + EMERGENCY ----------
#define MQ4_PIN        34      // Analog pin for MQ4
#define CUT_SERVO_PIN  21     // Second servo for gas cutoff
#define GAS_THRESHOLD  2000    // Adjust after calibration
bool emergencyMode = false;
unsigned long gasLeakStart = 0;
unsigned long emergencyHoldStart = 0;



#define CX 120

// ---------- State ----------
enum State { SET_TIME, SET_FLAME, RUNNING, PAUSED_TIME, PAUSED_FLAME, FINISHED };
State state = SET_TIME, lastState = FINISHED;

// ---------- Time ----------
int hh = 0, mm = 1, ss = 0;
int cursor = 0;
unsigned long remainSec = 0, lastTick = 0;

// ---------- Hold / Long Press ----------
unsigned long okPressStart = 0;
bool okHeld = false;
bool resetTriggered = false;


// ---------- Flame ----------
int flame = 1;

// ---------- Blink ----------
bool blink = true;
unsigned long blinkTimer = 0;

// ---------- Button debounce ----------
unsigned long lastBtnTime[5] = {0};
unsigned long lastOKTime = 0;
const int debounceMs = 150;

// ---------- UI cache ----------
int p_h=-1,p_m=-1,p_s=-1,p_flame=-1;

// ---------- Flame animation ----------
unsigned long flameAnimTimer = 0;
int flameFrame = 0;

// ---------- Tick Animation ----------
unsigned long tickAnimTimer = 0;
int tickFrame = 0;

// ---------- Finish handling ----------
unsigned long finishStartTime = 0;
bool finishBeepDone = false;

// ---------- Finished beep (intermittent) ----------
unsigned long finishBeepStart = 0;
unsigned long lastBeepToggle = 0;
bool beepOn = false;


// ---------- WiFi & Twilio ----------


const char* ssid = "";
const char* password = "";
const char* accountSid = "#";
const char* authToken = "#";
const char* twilioPhoneNumber = "+99999999999999";
const char* recipientPhoneNumber = "+9199999999999";



// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  Serial.println("SMART STOVE STARTING...");
  pinMode(MQ4_PIN, INPUT);

  cutServo.attach(CUT_SERVO_PIN);
  cutServo.write(0);   // normal open

  Serial.print("Connecting to WiFi");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());




  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_OK, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);

  gasServo.attach(SERVO_PIN);
  gasServo.write(0);

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  drawStaticUI();
}

// ---------- Loop ----------
void loop() {
  checkGasLeak();        // 🔴 must be FIRST to detect danger immediately
  handleEmergencyUI();   // 🔴 handles alarm screen & reset hold

  if (emergencyMode) return;   // ⛔ stop all normal stove logic in emergency

  handleButtons();
  updateTimer();
  handleFinishedState(); 
  updateBlink();
  drawUI();
}


// ---------- Static UI ----------
void drawStaticUI() {
  tft.fillScreen(TFT_BLACK);
  // tft.drawRoundRect(5,5,230,230,15,TFT_DARKGREY);
  tft.setTextColor(TFT_ORANGE);
  tft.drawCentreString("SMART STOVE", CX, 12, 2);
}

// ---------- UI ----------
void drawUI() {
  drawTime();
  drawFlame();
  drawState();
  if(state == RUNNING)
    drawFlameAnimation();
  else if(state == FINISHED)
    drawTickAnimation();

}

// ---------- Time ----------
void drawTime() {
  int dh = hh, dm = mm, ds = ss;

  if (state == RUNNING) {
    unsigned long t = remainSec;
    dh = t / 3600;
    dm = (t % 3600) / 60;
    ds = t % 60;
  }

  if (dh!=p_h || dm!=p_m || ds!=p_s) {
    tft.fillRect(20,40,200,50,TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    drawNum(dh,60,60);
    drawNum(dm,120,60);
    drawNum(ds,180,60);
    tft.drawCentreString(":",90,60,4);
    tft.drawCentreString(":",150,60,4);
    p_h=dh; p_m=dm; p_s=ds;
  }

  tft.drawFastHLine(42,80,156,TFT_BLACK);
  if ((state==SET_TIME || state==PAUSED_TIME) && blink) {
    int x[]={60,120,180};
    tft.drawFastHLine(x[cursor]-18,80,36,TFT_ORANGE);
  }
}

// ---------- Flame Level ----------
void drawFlame() {
  if (flame!=p_flame || state!=lastState) {
    tft.fillRect(20,95,200,45,TFT_BLACK);
    const char* f[]={"LOW","MED","HIGH"};
    for(int i=0;i<3;i++){
      int x=60+i*60;
      tft.setTextColor(i==flame?TFT_ORANGE:TFT_DARKGREY);
      tft.drawCentreString(f[i],x,105,2);
    }
    p_flame=flame;
  }

  tft.drawFastHLine(45,125,150,TFT_BLACK);
  if ((state==SET_FLAME || state==PAUSED_FLAME) && blink) {
    int x[]={60,120,180};
    tft.drawFastHLine(x[flame]-15,125,30,TFT_ORANGE);
  }
}

// ---------- State ----------
void drawState() {
  if (state==lastState) return;

  tft.fillRoundRect(40,200,160,35,10,TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE,TFT_DARKGREY);

  const char* s[]={
    "SET TIME","SET FLAME","COOKING",
    "PAUSE : TIME","PAUSE: FLAME","DONE"
  };
  tft.drawCentreString(s[state],CX,210,2);

  lastState=state;
}

// ---------- 🔥 FLAME ANIMATION (CENTER GAP) ----------
void drawFlameAnimation() {
  if (state != RUNNING) return;

  // -------- FIXED FLAME AREA --------
  const int flameX = 120;   // screen center
  const int flameTop = 140;
  const int flameBottom = 195;

  const int flameHeight = flameBottom - flameTop;

  // -------- Animation timing --------
  static uint32_t lastAnim = 0;
  static uint8_t frame = 0;

  if (millis() - lastAnim > 120) {
    lastAnim = millis();
    frame = (frame + 1) % 4;
  }

  int sway = (frame % 2 == 0) ? -2 : 2;
  int flicker = frame * 2;

  // -------- Clear ONLY flame area --------
  tft.fillRect(40, flameTop, 160, flameHeight, TFT_BLACK);

  int baseY = flameBottom;

  // ================= OUTER FLAME (RED) =================
  tft.fillTriangle(
    flameX + sway, flameTop + flicker,
    flameX - 28, baseY,
    flameX + 28, baseY,
    TFT_RED
  );

  tft.fillTriangle(
    flameX - 18 + sway, flameTop + 18,
    flameX - 36, baseY - 10,
    flameX - 6, baseY,
    TFT_RED
  );

  tft.fillTriangle(
    flameX + 18 + sway, flameTop + 18,
    flameX + 6, baseY,
    flameX + 36, baseY - 10,
    TFT_RED
  );

  // ================= MID FLAME (ORANGE) =================
  tft.fillTriangle(
    flameX + sway, flameTop + 10 + flicker,
    flameX - 18, baseY,
    flameX + 18, baseY,
    TFT_ORANGE
  );

  // ================= INNER FLAME (YELLOW) =================
  tft.fillTriangle(
    flameX + sway, flameTop + 25 + flicker,
    flameX - 10, baseY - 5,
    flameX + 10, baseY - 5,
    TFT_YELLOW
  );
}

// ----------  TICK ANIMATION ----------
void drawTickAnimation() {

  const int cx = 120;
  const int top = 140;
  const int bottom = 200;

  // Clear animation area
  tft.fillRect(40, top, 160, bottom - top, TFT_BLACK);

  // Animate tick drawing
  if(millis() - tickAnimTimer > 180){
    tickAnimTimer = millis();
    tickFrame++;
    if(tickFrame > 3) tickFrame = 3;
  }

  tft.drawCircle(cx, (top + bottom) / 2, 28, TFT_GREEN);

  if(tickFrame >= 1)
    tft.drawLine(cx - 14, (top + bottom)/2,
                 cx - 4, (top + bottom)/2 + 10,
                 TFT_GREEN);

  if(tickFrame >= 2)
    tft.drawLine(cx - 4, (top + bottom)/2 + 10,
                 cx + 16, (top + bottom)/2 - 10,
                 TFT_GREEN);
}


// ---------- Timer ----------
void updateTimer() {
  if(state==RUNNING && millis()-lastTick>=1000){
    lastTick=millis();
    if(remainSec>0) remainSec--;
    else finishCooking();
  }
}
void handleFinishedState(){
  if(state != FINISHED) return;

  unsigned long now = millis();

  // 🔊 Beep–beep (200ms ON / 200ms OFF) for 5 seconds
  if(now - finishBeepStart < 5000){

    if(now - lastBeepToggle >= 200){
      lastBeepToggle = now;
      beepOn = !beepOn;

      if(beepOn)
        tone(BUZZER, 3500);
      else
        noTone(BUZZER);
    }

  } else {
    noTone(BUZZER);   // stop after 5 seconds
  }
}



// ---------- Buttons ----------
void handleButtons() {

  // 🔥 If cooking finished → any button resets immediately
  if (state == FINISHED) {
    if (digitalRead(BTN_UP) == LOW ||
        digitalRead(BTN_DOWN) == LOW ||
        digitalRead(BTN_LEFT) == LOW ||
        digitalRead(BTN_RIGHT) == LOW ||
        digitalRead(BTN_OK) == LOW) {
        reset();
      
      return;
    }
  }

  readBtn(BTN_UP,0);
  readBtn(BTN_DOWN,1);
  readBtn(BTN_LEFT,2);
  readBtn(BTN_RIGHT,3);
  handleOK();
}


void readBtn(int pin,int id){
  if(digitalRead(pin)==LOW && millis()-lastBtnTime[id]>debounceMs){
    lastBtnTime[id]=millis();
    beep(60);
    onPress(id);
  }
}

void handleOK() {
  // Check if OK is pressed
  if (digitalRead(BTN_OK) == LOW) {
    if (!okHeld) {            // First moment of press
      okHeld = true;
      okPressStart = millis();
      resetTriggered = false;
      beep(50);               // short beep for press feedback
    } else {
      // Check long press (3 seconds)
      if (!resetTriggered && millis() - okPressStart >= 3000) {
        resetStove();        // call reset function
        resetTriggered = true;
      }
    }
  } else { 
    // Button released
    if (okHeld && !resetTriggered) {
      okHeld = false;
      onPress(4);            // normal OK behavior
    }
    okHeld = false;           // reset hold flag
  }
}


// ---------- Logic ----------
void onPress(int id){

  if(state==SET_TIME){
    if(id==0) changeTime(1);
    if(id==1) changeTime(-1);
    if(id==2) cursor=(cursor+2)%3;
    if(id==3) cursor=(cursor+1)%3;
    if(id==4) state=SET_FLAME;
  }

  else if(state==SET_FLAME){
    if(id==0) state=SET_TIME;   // 🔹 FIX: UP button goes back to time
    if(id==2) flame=(flame+2)%3;
    if(id==3) flame=(flame+1)%3;
    if(id==4) startCooking();
  }

  else if(state==RUNNING && id==4){
    state = PAUSED_TIME;
    gasServo.write(0);
    hh = remainSec / 3600;
    mm = (remainSec % 3600) / 60;
    ss = remainSec % 60;
  }

  else if(state==PAUSED_TIME){
    if(id==0) changeTime(1);
    if(id==1) changeTime(-1);
    if(id==2) cursor=(cursor+2)%3;
    if(id==3) cursor=(cursor+1)%3;
    if(id==4) state=PAUSED_FLAME;
  }

  else if(state==PAUSED_FLAME){
    if(id==0){                     // UP → back to time
      state = PAUSED_TIME;
    }
    if(id==2) flame=(flame+2)%3;
    if(id==3) flame=(flame+1)%3;
    if(id==4){
      remainSec = hh*3600 + mm*60 + ss;
      if(remainSec>0){
        state = RUNNING;
        updateServo();
      }
    }
  }
}

// ---------- Cooking ----------
void startCooking(){
  remainSec = hh*3600 + mm*60 + ss;
  if(remainSec==0) return;
  state = RUNNING;
  updateServo();
}

void finishCooking(){
  state = FINISHED;
  gasServo.write(0);

  tickFrame = 0;
  tickAnimTimer = millis();

  finishStartTime = millis();
  finishBeepStart = millis();
  lastBeepToggle = millis();
  beepOn = false;
}


// ---------- Servo ----------
void updateServo(){
  if(state==RUNNING){
    int a[]={40,90,140};
    gasServo.write(a[flame]);
  } else gasServo.write(0);
}

// ---------- Utils ----------
void changeTime(int v){
  if(cursor==0) hh=constrain(hh+v,0,23);
  if(cursor==1) mm=constrain(mm+v,0,59);
  if(cursor==2) ss=constrain(ss+v,0,59);
}

void updateBlink(){
  if(millis()-blinkTimer>400){
    blinkTimer=millis();
    blink=!blink;
  }
}

void drawNum(int v,int x,int y){
  tft.drawCentreString(v<10?"0"+String(v):String(v),x,y,4);
}

// ---------- Buzzer ----------
void beep(int d){
  tone(BUZZER,3500,d);
}
void resetStove() {
  // Reset all values
  state = SET_TIME;
  hh = 0;
  mm = 1;
  ss = 0;
  flame = 1;
  remainSec = 0;
  cursor = 0;
  gasServo.write(0);

  // Reset UI cache to force redraw
  p_h = p_m = p_s = p_flame = -1;
  lastState = FINISHED;

  // Clear screen and redraw static UI
  tft.fillScreen(TFT_BLACK);
  drawStaticUI();

  beep(3000);  // long beep to indicate reset
}
void reset() {
  // Reset all values
  state = SET_TIME;
  hh = 0;
  mm = 1;
  ss = 0;
  flame = 1;
  remainSec = 0;
  cursor = 0;
  gasServo.write(0);

  // Reset UI cache to force redraw
  p_h = p_m = p_s = p_flame = -1;
  lastState = FINISHED;

  // Clear screen and redraw static UI
  tft.fillScreen(TFT_BLACK);
  drawStaticUI();

  
}

void checkGasLeak() {

  if (emergencyMode) return;

  int gasValue = analogRead(MQ4_PIN);
  Serial.println(gasValue);

  if (gasValue > GAS_THRESHOLD) {

    if (gasLeakStart == 0)
      gasLeakStart = millis();

    // Leak for more than 5 seconds
    if (millis() - gasLeakStart > 5000) {
      triggerEmergency();
    }

  } else {
    gasLeakStart = 0;
  }
}
void triggerEmergency() {

  emergencyMode = true;

  // Cut gas immediately
  gasServo.write(0);
  cutServo.write(90);

  // Send SMS
  String emergencyMessage ="gas leaked";
  sendTwilioSMS(emergencyMessage);
}

void handleEmergencyUI() {

  if (!emergencyMode) return;

  // Screen alert
  tft.fillScreen(TFT_RED);
  tft.setTextColor(TFT_WHITE, TFT_RED);
  tft.drawCentreString("!!! GAS LEAKAGE !!!", 120, 80, 2);
  tft.drawCentreString("HOLD OK 10s", 120, 120, 2);

  // Continuous buzzer
  tone(BUZZER, 2000);

  // Hold OK for 10 sec to reset
  if (digitalRead(BTN_OK) == LOW) {

    if (emergencyHoldStart == 0)
      emergencyHoldStart = millis();

    if (millis() - emergencyHoldStart > 10000) {
      noTone(BUZZER);
      resetStove();
      cutServo.write(0);
      emergencyMode = false;
      emergencyHoldStart = 0;
    }

  } else {
    emergencyHoldStart = 0;
  }
}




void sendTwilioSMS(String message) {
    WiFiClientSecure client;
    client.setInsecure();

    if (client.connect("api.twilio.com", 443)) {
        String auth = base64::encode(accountSid + String(":") + authToken);
        String body = "Body=" + message + "&From=" + twilioPhoneNumber + "&To=" + recipientPhoneNumber;

        client.println("POST /2010-04-01/Accounts/" + String(accountSid) + "/Messages.json HTTP/1.1");
        client.println("Host: api.twilio.com");
        client.println("Authorization: Basic " + auth);
        client.println("Content-Type: application/x-www-form-urlencoded");
        client.println("Content-Length: " + String(body.length()));
        client.println();
        client.println(body);

        Serial.println(client.readString());  // Debugging
    }
    //call();
}

// void call() {
//     WiFiClientSecure client;
//     client.setInsecure();

//     if (client.connect("api.twilio.com", 443)) {
//         String data = "To=" + String(recipientPhoneNumber) + "&From=" + String(twilioPhoneNumber) + "&Url=" + "http://demo.twilio.com/docs/voice.xml";
//         String url = "/2010-04-01/Accounts/" + String(accountSid) + "/Calls.json";
//         String auth = "Basic " + base64::encode(accountSid + String(":") + authToken);

//         client.print("POST " + url + " HTTP/1.1\r\n");
//         client.print("Host: api.twilio.com\r\n");
//         client.print("Authorization: " + auth + "\r\n");
//         client.print("Content-Type: application/x-www-form-urlencoded\r\n");
//         client.print("Content-Length: " + String(data.length()) + "\r\n\r\n");
//         client.print(data);

//         Serial.println(client.readString());  // Debugging
//     }
// }