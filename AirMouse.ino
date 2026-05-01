#include <BleConnectionStatus.h>
#include <BleMouse.h>

// Brownout overrides (just in case)
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

BleMouse bleMouse("Frankenstein", "Espressif", 100);

//using hiletgo to connect to the esp32
#include <Arduino.h>
#include <HardwareSerial.h>

//pin definitions

//joystick pins
const int PIN_X = 34; //horizaontal axis
const int PIN_Y = 35; //vertical movement

// //the buttons orientation of joystick being at the top
// const int PIN_BTN_C = 27; //bottom yellow button (MB4/back)
// const int PIN_BTN_A = 13; //top yel btn (MB5/forward)
const int PIN_BTN_B = 14; //left green btn (left click)
const int PIN_BTN_D = 26; //right green button (right click)

// //small buttons I will use as home and map ig
// const int PIN_BTN_E = 25; //bottom button ()
// const int PIN_BTN_F = 33; //top button (scroll mode toggle)

// //joystick btn
const int PIN_BTN_K = 32; //joystick button (middle click)

//NEW: Status LED Pin (Usually GPIO 2)
const int STATUS_LED = 2;

//mouse code

//config for the "cursor"
const int DEADZONE = 400; //Ignores minor drift
const int CENTERVAL = 1930; //joystick center
const int SENSITIVITY = 120; //Higher sens = slower cursor

//state tracker for the btns
int lastBtnKState = HIGH; //High meaning that it is unpressed due to INPUT_PULLUP
int lastBtnBState = HIGH; //High meaning that it is unpressed due to INPUT_PULLUP
int lastBtnDState = HIGH; //High meaning that it is unpressed due to INPUT_PULLUP

//void setup cause this is C and void means no data type assignment is needed
void setup(){
  // Disable the brownout detector so the board doesn't crash on BLE startup
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);

  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, LOW); //Starts off

  //Joystick config
  //Since it is purely analog for now, we can leave it as INPUT
  pinMode(PIN_X, INPUT);
  pinMode(PIN_Y, INPUT);

  //Button config
  pinMode(PIN_BTN_K, INPUT_PULLUP); //since it wires straight to gnd it needs to be INPUT_PULLUP
  pinMode(PIN_BTN_B, INPUT_PULLUP); //since it wires straight to gnd it needs to be INPUT_PULLUP
  pinMode(PIN_BTN_D, INPUT_PULLUP); //since it wires straight to gnd it needs to be INPUT_PULLUP

  Serial.println("Starting BLE Testing...");
  bleMouse.begin();
}

//void loop time for the ESP32 to continually check the inputs
void loop(){

  //LED Logic
  if(bleMouse.isConnected()){
    digitalWrite(STATUS_LED, HIGH); //solid blue when connected
  }
  //search mode
  else{
    //blinking in search mode
    if((millis()/500)%2 == 0){
      digitalWrite(STATUS_LED, HIGH); //solid blue when connected
    }
    else{
      digitalWrite(STATUS_LED, LOW);
    }
  }

  //Joystick logic
  if(bleMouse.isConnected()){

    //movement logic

    //reading the analog signals of the joystick
    int xReadings = analogRead(PIN_X);
    int yReadings = analogRead(PIN_Y);
    int xVal = 0;
    int yVal = 0;

    //Using the deadzone to make the adjustments for drift
    if(abs(xReadings - CENTERVAL) > DEADZONE) {
      xVal = xReadings - CENTERVAL;
    }
    if(abs(yReadings - CENTERVAL) > DEADZONE) {
      yVal = yReadings - CENTERVAL;
    }

    // calculating how far to move the mouse based on sens
    int xMove = xVal/SENSITIVITY;
    int yMove = yVal/SENSITIVITY;

    //making sure signal is sent only if there is actual movement
    if(xMove != 0 || yMove != 0){
      bleMouse.move(xMove, yMove);
    }

    //middle click logic
    
    //updating state based on the input
    int currBtnKState = digitalRead(PIN_BTN_K);

    //checking if the button state changed after the last loop
    if(currBtnKState != lastBtnKState){

      if(currBtnKState == LOW){
        //LOW shows that the button was pressed
        bleMouse.press(MOUSE_MIDDLE);
        //commented out to prevent serial monitor clutter
        // Serial.println("Middle Click has been Pressed");
      }
      else{
        //meaning it is on high and button has been released
        bleMouse.release(MOUSE_MIDDLE);
        //commented out to prevent serial monitor clutter
        // Serial.println("Middle click has been Released");
      }

      //updating the state to prevent spamming
      lastBtnKState = currBtnKState;

      //adding a delay for consistency
      delay(20);
    }

    //Right click logic
    
    //updating state based on the input
    int currBtnBState = digitalRead(PIN_BTN_B);

    //checking if the button state changed after the last loop
    if(currBtnBState != lastBtnBState){

      if(currBtnBState == LOW){
        //LOW shows that the button was pressed
        bleMouse.press(MOUSE_RIGHT);
        Serial.println("Right Click has been Pressed");
      }
      else{
        //meaning it is on high and button has been released
        bleMouse.release(MOUSE_RIGHT);
        Serial.println("Right click has been Released");
      }

      //updating the state to prevent spamming
      lastBtnBState = currBtnBState;

      //adding a delay for consistency
      delay(20);
    }

    //Left click logic
    
    //updating state based on the input
    int currBtnDState = digitalRead(PIN_BTN_D);

    //checking if the button state changed after the last loop
    if(currBtnDState != lastBtnDState){

      if(currBtnDState == LOW){
        //LOW shows that the button was pressed
        bleMouse.press(MOUSE_LEFT);
        Serial.println("Left Click has been Pressed");
      }
      else{
        //meaning it is on high and button has been released
        bleMouse.release(MOUSE_LEFT);
        Serial.println("Left click has been Released");
      }

      //updating the state to prevent spamming
      lastBtnDState = currBtnDState;

      //adding a delay for consistency
      delay(20);
    }

    //delay for stability
    delay(10);
  }
}
