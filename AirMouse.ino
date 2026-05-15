#include <BleConnectionStatus.h>
#include <BleMouse.h>
#include <BLEDevice.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Brownout overrides (just in case)
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

BleMouse bleMouse("Frankenstein", "Espressif", 100);

//using hiletgo to connect to the esp32
#include <Arduino.h>
#include <HardwareSerial.h>

//OLED defines
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET    -1 // This tells the library your screen doesn't have a dedicated reset pin

//Creating the display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//pin definitions

//joystick pins
const int PIN_X = 34; //horizaontal axis
const int PIN_Y = 35; //vertical movement

//the buttons orientation of joystick being at the top
const int PIN_BTN_C = 27; //Green button (MB4/back)
const int PIN_BTN_A = 13; //Blue btn (MB5/forward)
const int PIN_BTN_B = 14; //Red btn (right click)
const int PIN_BTN_D = 26; //Black button (left click)

//small buttons I will use as sensitivity adjusters
const int PIN_BTN_E = 25; //lower button (lower sens)[traditional mouse decrease]
const int PIN_BTN_F = 33; //upper button (increase sens)[traditional mouse increase]

//joystick btn
const int PIN_BTN_K = 32; //joystick button (middle click)

//OLED buttons
/* Green wire goes to Pin 22 and Blue wire goes to Pin 21 */

//Status LED Pin (Usually GPIO 2)
const int STATUS_LED = 2;

//state tracker for the BLE connection
bool wasConnected = false; //default

//mouse code

//config for the "cursor"
const int DEADZONE = 850; //Ignores minor drift
const int CENTERVAL = 1930; //joystick center
int sensitivity = 120; //Higher sens = slower cursor (changed so I can add functionality to change the sens using F and E)

//state tracker for the btns
int lastBtnKState = HIGH; //High meaning that it is unpressed due to INPUT_PULLUP
int lastBtnBState = HIGH; //High meaning that it is unpressed due to INPUT_PULLUP
int lastBtnDState = HIGH; //High meaning that it is unpressed due to INPUT_PULLUP
int lastBtnAState = HIGH; //High meaning that it is unpressed due to INPUT_PULLUP
int lastBtnCState = HIGH; //High meaning that it is unpressed due to INPUT_PULLUP
int lastBtnEState = HIGH; //High meaning that it is unpressed due to INPUT_PULLUP
int lastBtnFState = HIGH; //High meaning that it is unpressed due to INPUT_PULLUP

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
  pinMode(PIN_BTN_A, INPUT_PULLUP); //since it wires straight to gnd it needs to be INPUT_PULLUP
  pinMode(PIN_BTN_C, INPUT_PULLUP); //since it wires straight to gnd it needs to be INPUT_PULLUP
  pinMode(PIN_BTN_E, INPUT_PULLUP); //since it wires straight to gnd it needs to be INPUT_PULLUP
  pinMode(PIN_BTN_F, INPUT_PULLUP); //since it wires straight to gnd it needs to be INPUT_PULLUP

  // Initialize the OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("OLED failed to initialize"));
    // We don't stop the code here so the mouse still works even if screen fails
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.display(); // Start with a blank screen

  Serial.println("Initiating lightning strike.");
  //just cuz
  Serial.print(".");
  Serial.print(".");
  Serial.print(".");
  Serial.print(".");
  updateDisplay();
  bleMouse.begin();
}

//void loop time for the ESP32 to continually check the inputs
void loop(){

  //connection checker
  bool isConnected = bleMouse.isConnected();//local variable so it is easier to use multiple times

  //When Connection has just been established
  if(isConnected && !wasConnected){
    Serial.println("Frankenstein has risen!");
    digitalWrite(STATUS_LED, HIGH); //updating blue LED
    wasConnected = true; //updating the flag
  }

  //Just diconnected
  else if(!isConnected && wasConnected){
    Serial.println("Frankenstein needs some juice!");
    digitalWrite(STATUS_LED, LOW);

    //Delay to ensure the board has enough time to restart
    delay(500);

    //Restarting the advertising
    BLEDevice::startAdvertising();

    wasConnected = false; //updating the flag
  }

  //LED logic
  if(!isConnected){
    if((millis()/500)%2 == 0){
      digitalWrite(STATUS_LED, HIGH);
    }
    else{
      digitalWrite(STATUS_LED, LOW);
    }
  }

  //mouse code start

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
    int xMove = xVal/sensitivity;
    int yMove = (yVal/sensitivity);

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
        //commented out to prevent serial monitor clutter
        // Serial.println("Right Click has been Pressed");
      }
      else{
        //meaning it is on high and button has been released
        bleMouse.release(MOUSE_RIGHT);
        //commented out to prevent serial monitor clutter
        // Serial.println("Right click has been Released");
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
        //commented out to prevent serial monitor clutter
        // Serial.println("Left Click has been Pressed");
      }
      else{
        //meaning it is on high and button has been released
        bleMouse.release(MOUSE_LEFT);
        //commented out to prevent serial monitor clutter
        // Serial.println("Left click has been Released");
      }

      //updating the state to prevent spamming
      lastBtnDState = currBtnDState;

      //adding a delay for consistency
      delay(20);
    }

    //MB5 click logic
    
    //updating state based on the input
    int currBtnAState = digitalRead(PIN_BTN_A);

    //checking if the button state changed after the last loop
    if(currBtnAState != lastBtnAState){

      if(currBtnAState == LOW){
        //LOW shows that the button was pressed
        bleMouse.press(MOUSE_FORWARD);
        //commented out to prevent serial monitor clutter
        Serial.println("MB5 Click has been Pressed");
      }
      else{
        //meaning it is on high and button has been released
        bleMouse.release(MOUSE_FORWARD);
        //commented out to prevent serial monitor clutter
        Serial.println("MB5 click has been Released");
      }

      //updating the state to prevent spamming
      lastBtnAState = currBtnAState;

      //adding a delay for consistency
      delay(20);
    }

    //MB4 click logic
    
    //updating state based on the input
    int currBtnCState = digitalRead(PIN_BTN_C);

    //checking if the button state changed after the last loop
    if(currBtnCState != lastBtnCState){

      if(currBtnCState == LOW){
        //LOW shows that the button was pressed
        bleMouse.press(MOUSE_BACK);
        //commented out to prevent serial monitor clutter
        Serial.println("MB4 Click has been Pressed");
      }
      else{
        //meaning it is on high and button has been released
        bleMouse.release(MOUSE_BACK);
        //commented out to prevent serial monitor clutter
        Serial.println("MB4 click has been Released");
      }

      //updating the state to prevent spamming
      lastBtnCState = currBtnCState;

      //adding a delay for consistency
      delay(20);
    }

    //Sensitivity increase (in the traditional mouse sense which in code is lower sens)
    
    //updating state based on the input
    int currBtnFState = digitalRead(PIN_BTN_F);

    //checking if the button state changed after the last loop
    if(currBtnFState != lastBtnFState){

      if(currBtnFState == LOW){
        //"increasing the sens by 10"
        sensitivity -= 10;

        //just to make sure we dont go to 0
        if (sensitivity < 10) {
           sensitivity = 10; // Never let it drop below 10!
        }

        //commented out to prevent serial monitor clutter
        // Serial.print("Current Sens (rmb lower num means higher sens): ");
        // Serial.println(sensitivity);
        updateDisplay();
      }

      //updating the state to prevent spamming
      lastBtnFState = currBtnFState;

      //adding a delay for consistency
      delay(20);
    }

    //Sensitivity decrease (in the traditional mouse sense which in code is higher sens)
    
    //updating state based on the input
    int currBtnEState = digitalRead(PIN_BTN_E);

    //checking if the button state changed after the last loop
    if(currBtnEState != lastBtnEState){

      if(currBtnEState == LOW){
        //"increasing the sens by 10"
        sensitivity += 10;

        //so we dont go over a cerain amt
        if (sensitivity > 350) {
           sensitivity = 350; // Never let it go above 350!
        }
        //commented out to prevent serial monitor clutter
        // Serial.print("Current Sens (rmb higher num means lower sens): ");
        // Serial.println(sensitivity);
        updateDisplay();
      }

      //updating the state to prevent spamming
      lastBtnEState = currBtnEState;

      //adding a delay for consistency
      delay(20);
    }

    //delay for stability
    delay(10);
  }
}

void updateDisplay(){

  display.clearDisplay(); //start blank
  display.setTextSize(1); //set small text (can be adjusted based on testing)
  display.setCursor(0,0); //keeping it to the top left
  display.print("Sens:");

  //Calculations for the displayed sensitivity values
  int calculatedSens = map(sensitivity, 350, 10, 400, 3200);

  display.setCursor(0,15); //move down for the actual sensitivity value
  display.setTextSize(2); //increase number size
  display.print(calculatedSens);

  display.display(); //pushing to display everything
}
