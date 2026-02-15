#include <TFT_eSPI.h>
#include <ESP32Servo.h>

TFT_eSPI tft;
Servo gasServo;

// ---------- Pins ----------
#define BTN_UP     32
#define BTN_DOWN   33
#define BTN_LEFT   25
#define BTN_RIGHT  26
#define BTN_OK     27
#define SERVO_PIN  19
#define BUZZER     12

#define CX 120

// ---------- State ----------
enum State { SET_TIME, SET_FLAME, RUNNING, PAUSED, FINISHED };
State state = SET_TIME, lastState = FINISHED;

// ---------- Time ----------
int hh = 0, mm = 1, ss = 0;
int cursor = 0;
unsigned long remainSec = 0, lastTick = 0;

// ---------- Flame ----------
int flame = 1;
unsigned long lastOKTime = 0;
bool resetTriggered = false;

// ---------- Blink ----------
bool blink = true;
unsigned long blinkTimer = 0;

// ---------- Button debounce ----------
unsigned long lastBtnTime[5] = {0};
unsigned long okPressStart = 0;
bool okHolding = false;

const int debounceMs = 150;
const int resetHoldMs = 3000;



// ---------- UI cache ----------
int p_h=-1,p_m=-1,p_s=-1,p_flame=-1;

// ---------- Setup ----------
void setup() {
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
  handleButtons();
  updateTimer();
  updateBlink();
  drawUI();
}

// ---------- Static UI ----------
void drawStaticUI() {
  tft.fillScreen(TFT_BLACK);
  tft.drawRoundRect(5,5,230,230,15,TFT_DARKGREY);
  tft.setTextColor(TFT_ORANGE);
  tft.drawCentreString("SMART STOVE", CX, 12, 2);
}

// ---------- UI ----------
void drawUI() {
  drawTime();
  drawFlame();
  drawState();
}

// ---------- Time ----------
void drawTime() {
  int dh = hh, dm = mm, ds = ss;

  if (state == RUNNING || state == PAUSED) {
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
  if (state==SET_TIME && blink) {
    int x[]={60,120,180};
    tft.drawFastHLine(x[cursor]-18,80,36,TFT_ORANGE);
  }
}

// ---------- Flame ----------
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
  if(state==SET_FLAME && blink){
    int x[]={60,120,180};
    tft.drawFastHLine(x[flame]-15,125,30,TFT_ORANGE);
  }
}

// ---------- State ----------
void drawState() {
  if (state==lastState) return;

  tft.fillRoundRect(40,170,160,35,10,TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE,TFT_DARKGREY);

  const char* s[]={"SET TIME","SET FLAME","COOKING","PAUSED","DONE"};
  tft.drawCentreString(s[state],CX,180,2);

  lastState=state;
}

// ---------- Timer ----------
void updateTimer() {
  if(state==RUNNING && millis()-lastTick>=1000){
    lastTick=millis();
    if(remainSec>0) remainSec--;
    else finishCooking();
  }
}

// ---------- Buttons ----------
void handleButtons() {
  readBtn(BTN_UP,0);
  readBtn(BTN_DOWN,1);
  readBtn(BTN_LEFT,2);
  readBtn(BTN_RIGHT,3);
  handleOK();
}

void readBtn(int pin,int id){
  if(digitalRead(pin)==LOW && millis()-lastBtnTime[id]>debounceMs){
    lastBtnTime[id]=millis();
    beep(60);                // 🔊 louder click
    onPress(id);
  }
}

// ---------- OK with long press ----------
void handleOK() {
  bool pressed = digitalRead(BTN_OK) == LOW;

  // OK pressed (debounced)
  if (pressed && !okHolding && millis() - lastOKTime > debounceMs) {
    okHolding = true;
    okPressStart = millis();
    resetTriggered = false;
    lastOKTime = millis();
    tone(BUZZER, 3000);   // start long beep
  }

  // OK released
  if (!pressed && okHolding) {
    okHolding = false;
    noTone(BUZZER);

    if (!resetTriggered) {
      beep(80);          // short click
      onPress(4);        // SINGLE press guaranteed
    }
  }

  // Long press reset
  if (pressed && okHolding && !resetTriggered) {
    if (millis() - okPressStart >= resetHoldMs) {
      resetTriggered = true;
      noTone(BUZZER);
      resetAll();
    }
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
    if(id==0) state=SET_TIME;    // UP → back
    if(id==2) flame=(flame+2)%3; // LEFT
    if(id==3) flame=(flame+1)%3; // RIGHT
    if(id==4) startCooking();
  }

  else if(state==RUNNING && id==4){
    state=PAUSED; updateServo();
  }

  else if(state==PAUSED && id==4){
    state=RUNNING; updateServo();
  }
}

// ---------- Cooking ----------
void startCooking(){
  remainSec=hh*3600+mm*60+ss;
  if(remainSec==0) return;
  state=RUNNING;
  updateServo();
}

void finishCooking(){
  state=FINISHED;
  gasServo.write(0);
  for(int i=0;i<5;i++){ beep(400); delay(600); }
  state=SET_TIME;
}

// ---------- RESET ----------
void resetAll(){
  gasServo.write(0);
  for(int i=0;i<3;i++){ beep(300); delay(300); }
  state=SET_TIME;
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
  tone(BUZZER,3500,d);   // 
}
