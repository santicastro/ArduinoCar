#include <SPI.h>
#include <LiquidCrystal.h>

#define btnRIGHT 1
#define btnUP    2
#define btnDOWN  3
#define btnLEFT  4
#define btnLSLEFT   5 // left limit switch 
#define btnLSRIGHT   6 
#define btnNONE  0

#define ENCODER1_INT_PIN 2
#define ENCODER2_INT_PIN 3


#define M1 1
#define M2 2

#define M1_SPEED_PIN 5
#define M1_FRONT_SOFTPIN 2
#define M1_BACK_SOFTPIN 3

#define M2_SPEED_PIN 6
#define M2_FRONT_SOFTPIN 0
#define M2_BACK_SOFTPIN 1

#define LIGTHS_SOFTPIN 4
#define HORN_SOFTPIN 5

#define STOP_SPEED 0
#define SLOW_SPEED 80
#define NORMAL_SPEED 140
#define FAST_SPEED 200
#define ULTRAFAST_SPEED 255

// initialize SPI LCD on pin 8
LiquidCrystal lcd(8);


int SOFTWARE_OUTPUT_ID;

signed int speed1 = 0;
signed int speed2 = 0;

void setup() {
  Serial.begin(9600);
  /* Initialize de SPI software output on pin 7. This allows
   to use 8 extra output pin using only 1 pin */
  SOFTWARE_OUTPUT_ID = initSPISoftwareOutput(7);

  lcd.begin(16, 2);

  motorSetup();
  odometersSetup();
  drawMenu();
}

volatile signed int odo1_turns,odo2_turns;
void odometersSetup(){
  odo1_turns=0;
  odo2_turns=0;
  pinMode(ENCODER1_INT_PIN, INPUT);
  digitalWrite(ENCODER1_INT_PIN, HIGH);
  pinMode(ENCODER2_INT_PIN, INPUT);
  digitalWrite(ENCODER2_INT_PIN, HIGH);

  attachInterrupt(0, odo1_count, RISING);
  attachInterrupt(1, odo2_count, RISING);
}

void odo1_count(){
  delayMicroseconds(10);
  if(digitalRead(ENCODER1_INT_PIN)==HIGH){
    odo1_turns++;
  }
}

void odo2_count(){
  delayMicroseconds(10);
  if(digitalRead(ENCODER2_INT_PIN)==HIGH){
    odo2_turns++;
  }
}

// only for debug porpouse
void print_odo(){
  lcd.setCursor(0,1);
  lcd.print("e1:");
  lcd.print(odo1_turns);
  lcd.print("-e2:");
  lcd.print(odo2_turns);
  lcd.print(" ");
  lcd.print(millis()/1000);
}

void motorSetup(){
  digitalWrite(SOFTWARE_OUTPUT_ID, M1_FRONT_SOFTPIN, LOW);
  digitalWrite(SOFTWARE_OUTPUT_ID, M1_BACK_SOFTPIN, LOW);
  digitalWrite(SOFTWARE_OUTPUT_ID, M2_FRONT_SOFTPIN, LOW);
  digitalWrite(SOFTWARE_OUTPUT_ID, M2_BACK_SOFTPIN, LOW);
  motorControl(M1|M2 ,STOP_SPEED);
}

/* There are 4 buttons connected to Analog 0 input.
 This functions reads the analog value and returns the
 code of the button pressed.
 */
int read_buttons()
{
  int read_value  = analogRead(0); // values centered on: 0, 91, 359, 510, 613, 838 1023
  if (read_value > 1000) return btnNONE;

  int result = btnNONE;
  if (read_value < 50) result = btnLEFT;
  else if (read_value < 195)  result = btnDOWN;
  else if (read_value < 400)  result = btnUP;
  else if (read_value < 555)  result = btnRIGHT;
  else if (read_value < 650)  result = btnLSLEFT;
  else if (read_value < 900)  result = btnLSRIGHT;
  delay(5); // debounce
  if ( abs( analogRead(0) - read_value ) > 20 ) return btnNONE;
  return result;
}

/* This function controls the output.
 You can control more than one motor calling the function like this:
 motorControl( M1 | M2, ULTRA_FAST );
 */
void motorControl(int motor, signed int speed){
  if(motor&M1){
    if(speed>=0){
      digitalWrite(SOFTWARE_OUTPUT_ID, M1_BACK_SOFTPIN, HIGH);
      digitalWrite(SOFTWARE_OUTPUT_ID, M1_FRONT_SOFTPIN, LOW);
      analogWrite(M1_SPEED_PIN,speed);
    }
    else if(speed<0){
      digitalWrite(SOFTWARE_OUTPUT_ID, M1_BACK_SOFTPIN, LOW);
      digitalWrite(SOFTWARE_OUTPUT_ID, M1_FRONT_SOFTPIN, HIGH);
      analogWrite(M1_SPEED_PIN,-speed);
    }
    speed1=speed;
  }

  if(motor&M2){
    if(speed>=0){
      digitalWrite(SOFTWARE_OUTPUT_ID, M2_BACK_SOFTPIN, HIGH);
      digitalWrite(SOFTWARE_OUTPUT_ID, M2_FRONT_SOFTPIN, LOW);
      analogWrite(M2_SPEED_PIN,speed);
    }
    else if(speed<0){
      digitalWrite(SOFTWARE_OUTPUT_ID, M2_BACK_SOFTPIN, LOW);
      digitalWrite(SOFTWARE_OUTPUT_ID, M2_FRONT_SOFTPIN, HIGH);
      analogWrite(M2_SPEED_PIN,-speed);
    }
    speed2=speed;
  }

  lcd.setCursor(0,1);
  lcd.print("M1:");
  lcd.print(speed1);
  lcd.print(",M2:");
  lcd.print(speed2);
  lcd.print("       ");

}

/* Interprets the command and executes the required action
 There are 3 posible commands:
 - Change speed
 - Switch lights
 - Honking the horn
 */
void processCommand(unsigned char command[], int length){
  int i;
  if(length>0){
    switch(command[0]){
    case 'S': //speed
      if(length==4 && command[2]==','){
        changeSpeed(command[1],command[3]);
      }
      else{
        showInvalidCommand("Speed not valid ");
      }
      break;
    case 'L': //lights
      switchLights(!getSwitchLightsState());
      break;
    case 'H': //horn
      horn();
      break;
    }
  }
}
void showInvalidCommand(char comment[]){
  lcd.setCursor(0,1);
  lcd.print("Unknown command ");
}

void switchLights(int state){
  lcd.setCursor(0,1);
  lcd.print("Lights switch   ");
  digitalWrite(SOFTWARE_OUTPUT_ID, LIGTHS_SOFTPIN, state);
}

int getSwitchLightsState(){
  return digitalRead(SOFTWARE_OUTPUT_ID, LIGTHS_SOFTPIN);
}

void horn(){
  lcd.setCursor(0,1);
  lcd.print("Horn            ");
  digitalWrite(SOFTWARE_OUTPUT_ID, HORN_SOFTPIN, HIGH);
  delay(10);
  digitalWrite(SOFTWARE_OUTPUT_ID, HORN_SOFTPIN, LOW);
}

/* Calculates the speed of the 2 motors based on the position of the stick.
 - pan is the vertical position, from 0 (down) to 255 (up). Center is 128
 - tilt is the horizontal position, from 0 (left) to 255 (right). Center is 128
 */
void changeSpeed(byte pan, byte tilt){
  int pan_int = ((int)pan-128) * 2;
  int tilt_int =  ((int)tilt-128)*2;
  speed1= min(tilt_int + pan_int, 255);
  speed1= max(speed1, -255);
  motorControl(M1, speed1);
  speed2= min(tilt_int-pan_int, 255);
  speed2= max(speed2, -255);
  motorControl(M2, speed2);

}

const int TOTAL_MENU_COUNT = 2;
const char menus[TOTAL_MENU_COUNT][17]={
  "1.Remote control","2.Vacuum cleaner"};
int selectedMenu=0;

void drawMenu(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Working mode:   ");
  lcd.setCursor(0,1);
  lcd.print(menus[selectedMenu]); 
}

void executeSelectedMenu(){
  switch(selectedMenu){
  case 0:
    loopRemoteControl();
    break;
  case 1:
    loopVacuum();
    break;
  }
}

void waitButtonRelease(){
  while(read_buttons()!=btnNONE)
    delay(50);
}

void loop()
{
  switch(read_buttons()){
  case btnUP:
    selectedMenu = TOTAL_MENU_COUNT-1;
    drawMenu();
    waitButtonRelease();
    break;
  case btnDOWN:
    selectedMenu =  (selectedMenu+1) % TOTAL_MENU_COUNT;
    drawMenu();
    waitButtonRelease();
    break;
  case btnRIGHT:
    executeSelectedMenu();
    drawMenu();
    waitButtonRelease();
    break;
  }



  /*
while(true){
   int button = read_buttons();
   lcd.setCursor(0,0);
   lcd.print(analogRead(A0));
   lcd.print(" ");
   lcd.print(button);
   lcd.setCursor(0,1);
   switch(button){
   case btnRIGHT:
   lcd.print("->");
   break;
   case btnLEFT:
   lcd.print("<-");
   break;
   case btnUP:
   lcd.print("^ ");
   break;
   case btnDOWN:
   lcd.print("_ ");
   break;
   case btnNONE:
   lcd.print("  ");
   break;
   case btnLSLEFT:
   lcd.print("L<");
   break;
   case btnLSRIGHT:
   lcd.print("L>");
   break;
   }
   }
   delay(200);
   */
  /*  while(true){
   print_enc();
   delay(100);
   }*/
  /*
  for (int level = 0; level < 8; level++) {
   digitalWrite(SOFTWARE_OUTPUT_ID, level ,HIGH);
   delay(50);
   }
   for (int level = 7; level >=0; level--) {
   digitalWrite(SOFTWARE_OUTPUT_ID, level ,LOW);
   delay(50);
   }
   */
  /*
while(true){
   lcd.setCursor(0,1);
   lcd.print("e1:");
   lcd.print(digitalRead(ENCODER1_INT_PIN));
   lcd.print(",e2:");
   lcd.print(digitalRead(ENCODER2_INT_PIN));
   lcd.print("   ");
   }
   */
  /*
  
   
   */

}
void loopRemoteControl(){
  stopAll();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("# Remote Control");

  while(read_buttons()!=btnLEFT){
    const int MAX_COMMAND_LENGTH = 5;
    unsigned char buffer[MAX_COMMAND_LENGTH], bufferLength;
    int incomingByte = 0;
    if (Serial.available()) {
      // wait a bit for the entire message to arrive
      delay(50);
      // read all the available characters
      while (Serial.available() > 0) {
        incomingByte = Serial.read();
        if(incomingByte=='$'){ // $ is the command terminator.
          //If MAX_COMMAND_LENGTH is reached, then ignore
          if(bufferLength < MAX_COMMAND_LENGTH){
            processCommand(buffer, bufferLength);
            bufferLength = MAX_COMMAND_LENGTH;
          }
        }
        else{
          if(incomingByte=='^') // ^ is the command starter
          {
            bufferLength=0;
          }
          else if(bufferLength < MAX_COMMAND_LENGTH){
            buffer[bufferLength] = incomingByte;
            bufferLength++;
          }
        }
      }
    }
  }
  stopAll();
}

void loopVacuum(){
  stopAll();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("# Vacuun cleaner");

  motorControl(M1|M2, NORMAL_SPEED); 
  int last_read;
  while((last_read=read_buttons())!=btnLEFT){
    switch(last_read){
    case btnLSLEFT:
      avoid_obstacle(M1);
      break;
    case btnLSRIGHT:
      avoid_obstacle(M2);
      break;
    }
  }
  stopAll();

}

// stops, reverse, turn and run
void avoid_obstacle(int motor_on_obstacle_side){
  horn();
  motorControl(M1|M2, STOP_SPEED); 
  int oposite_motor = (motor_on_obstacle_side&M1)?M2:M1;
  delay(500);
  motorControl(M1|M2, -SLOW_SPEED); 
  delay(1000);
  motorControl(motor_on_obstacle_side, NORMAL_SPEED); 
  motorControl(oposite_motor, -NORMAL_SPEED); 
  delay(1500);
  motorControl(M1|M2, STOP_SPEED); 
  delay(500);
  motorControl(M1|M2, NORMAL_SPEED); 

}

void stopAll(){
  motorControl(M1|M2, STOP_SPEED);
  switchLights(LOW);
}
/////////////////////////////////////////////////////////////
/////// SoftwareOutput //////////////////////////////////////
/////////////////////////////////////////////////////////////

int outputState=0;
//returns software output id
int initSPISoftwareOutput(int slaveSelectPin){
  pinMode (slaveSelectPin, OUTPUT);
  return slaveSelectPin;
}

void digitalWrite(int softwareOutputId, int outputNumber, int value){
  bitWrite(outputState, outputNumber, value);
  SPI.transfer(outputState);
  digitalWrite(softwareOutputId, LOW);
  digitalWrite(softwareOutputId, HIGH);
}

int digitalRead(int softwareOutputId, int outputNumber){
  return bitRead(outputState, outputNumber);
}

void softwareOutputClear(int softwareOutputId){
  SPI.transfer(0);
  digitalWrite(softwareOutputId, LOW);
  delayMicroseconds(1);
  digitalWrite(softwareOutputId, HIGH);
}













