/*

HIL Snake

Hardware code:
this code runs on Arduino IDE and acts as the Hardware code.
Components for this project:
- Arduino Uno microcontroler
- Joystick for controling movement in the game
- Potentiometer for controling the game speed
- Buzzer for sound effects
- RGB led for different colors according to the game state
- Button for restarting the game
- Jumper wires and resistors connecdet accordingly

*/


//joystick:
#define jsy A0
#define jsx A1

//potentiometer for game speed:
#define potentiometer A5
//buzzer for sound effets:
#define buzzer 10
//button for restart:
#define restart 6

// RGB LED Pins
#define RED 2
#define GREEN 3
#define BLUE 4

//for 10 ms timer:
unsigned long lastSerialSent = 0;

//for changing the LED colors:
void setColor(bool r, bool g, bool b) {
    digitalWrite(RED, r);
    digitalWrite(GREEN, g);
    digitalWrite(BLUE, b);
}

void setup() {
    //set buad rate:
    Serial.begin(115200);
    pinMode(buzzer, OUTPUT);
    pinMode(restart, INPUT_PULLUP);

    //setup RGB pins
    pinMode(RED, OUTPUT);
    pinMode(GREEN, OUTPUT);
    pinMode(BLUE, OUTPUT);

    //default color: blue:
    setColor(0, 0, 1);
}

void loop() {
    //DOWNLINK: listen for commands from PC:

    //read one byte from USB buffer:
    if (Serial.available() > 0) {
        byte Commmand = Serial.read();

        if (Commmand == 1) {
            //1: normal movement:
            setColor(0, 0, 1);
            tone(buzzer, 1500, 10);
        }
        else if (Commmand == 2) {
            //2: apple eaten:
            setColor(0, 1, 0);
            tone(buzzer, 988, 50);
            delay(50);
            setColor(0, 0, 0);
            delay(50);
            setColor(0, 1, 0);
            tone(buzzer, 1319, 150);
            delay(150);
            setColor(0, 0, 1);
        }
        else if (Commmand == 3 || Commmand == 4) {
            //3/4: game over:
            setColor(1, 0, 0);
            tone(buzzer, 311, 400); delay(450);
            setColor(1, 1, 0);
            tone(buzzer, 294, 400); delay(450);
            setColor(1, 0, 0);
            tone(buzzer, 277, 400); delay(450);
            setColor(1, 1, 0);
            tone(buzzer, 262, 1200); delay(1200);
            setColor(0, 0, 1);
        }
    }

    //UPLINK: send input data from components to the PC:

    //send data every 10 ms:
    if ((millis() - lastSerialSent) > 10) {
        //read input data from components:
        int xVal = analogRead(jsx);
        int yVal = 1023 - analogRead(jsy);
        int potVal = analogRead(potentiometer);
        int btnVal = (digitalRead(restart) == LOW) ? 1 : 0;

        //send data to serial monitor for game logics:
        Serial.print(xVal);
        Serial.print(",");
        Serial.print(yVal);
        Serial.print(",");
        Serial.print(potVal);
        Serial.print(",");
        Serial.println(btnVal);

        //reset 10 ms countdown:
        lastSerialSent = millis();
    }
}