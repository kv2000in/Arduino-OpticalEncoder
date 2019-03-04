#define ENCODER0PINA         2      // interrupt pin (2,3 on nano)
#define pinENA               3  //  will change
#define pinIN1               4
#define pinIN2               5


int encoder0Position = 0;
//int PPR=2520; //pulses per rotation
int PWM = 255;
// volatile variables - modified by interrupt service routine (ISR)
volatile int counter=0;
bool motorARunning = false;
//Serial Read stuff
const byte numChars = 32;
char receivedChars[numChars]; // an array to store the received data
boolean newData = false;
char DIR[32] = {0};
//char DIR = "Z";
int VALUE = 0;

void setup()
{
 
  // pins
  pinMode(ENCODER0PINA, INPUT_PULLUP);
  pinMode(pinENA,OUTPUT);
  pinMode(pinIN1,OUTPUT);
  pinMode(pinIN2,OUTPUT);
  digitalWrite(pinIN1,LOW);
  //attach interrupt 
  attachInterrupt(digitalPinToInterrupt(ENCODER0PINA),onInterrupt, RISING);
  Serial.begin (9600);
  delay(1000);

}
 
void loop()
{
  //Read the Serial and move accordingly
  recvWithStartEndMarkers();
  parseData();
  //showNewData();
  if (newData){setPosition(DIR[0],VALUE);}

  
  
}
// Minimalistic ISR

void onInterrupt()
{
  // read both inputs - Why?? Interrupt was triggered by RISING edge on a
  // So we know that a =1 and that should be the case always.
  // So just read b. If it is 0 - moving counterclockwise, if it is 1 - moving clockwise.
 //Different readings of b at different speeds. When PWM set to 180 b = 1, position decreasing
 //When PWM is set to 255 - b =0 and position increasing
 //For the test setup motor - canon feed motor running from 24 V off canon psu - PWM = 200 seems to be transition point for same direction of rotation. 
 //For PWM >200 when interrupt is triggerred - b = 0
 //For PWM <200 when interrupt is triggerred - b = 1
 // So, this would have to be individualized for different motors and encoders. 
  //int a = digitalRead(ENCODER0PINA);
  counter++;

}

void rotateF()
{
    analogWrite(pinENA, PWM);
    digitalWrite(pinIN1,HIGH);
    digitalWrite(pinIN2,LOW);
    motorARunning=true;
}
void rotateR()
{
    analogWrite(pinENA, PWM);
    digitalWrite(pinIN1,LOW);
    digitalWrite(pinIN2,HIGH);
    motorARunning=true;
}
void rotateStop()
{
  analogWrite(pinENA, 0);
  motorARunning=false;
}

//Serial Read Functions
void recvWithStartEndMarkers() {
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char rc;
 
 // if (Serial.available() > 0) {
    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();

        if (recvInProgress == true) {
            if (rc != endMarker) {
                receivedChars[ndx] = rc;
                ndx++;
                if (ndx >= numChars) {
                    ndx = numChars - 1;
                }
            }
            else {
                receivedChars[ndx] = '\0'; // terminate the string
                recvInProgress = false;
                ndx = 0;
                newData = true;
            }
        }

        else if (rc == startMarker) {
            recvInProgress = true;
        }
    }
}

void showNewData() {
    if (newData == true) {
        Serial.print("This just in ... ");
        Serial.println(receivedChars);
        Serial.print("DIRECTION=");
        Serial.println(DIR);
        Serial.print("VALUE=");
        Serial.println(VALUE);
        newData = false;
    }
}


 
void parseData() {

    // split the data into its parts
    
  char * strtokIndx; // this is used by strtok() as an index
  
  strtokIndx = strtok(receivedChars,"-");      // get the first part - the string
  strcpy(DIR, strtokIndx); // copy it to DIR
  //strcpy(DIR,0);
  //strtokIndx = strtok(NULL, "-"); // this continues where the previous call left off
  //VALUE = atoi(strtokIndx); // convert this part to an integer
  VALUE = atoi(strtokIndx+2);   
 }

void setPosition(char dir,int steps)
{
  //This code causes some end jitters - as in either direction - there is a slight overshoot and then it goes back and forth until position is reached.
  //Also - if the encoder0Position changes - by external movement or whatever - this tries to bring it back to the pos parameter
  //I have set the RPM to 60 but this gives very little torque. Less jitters at slower speed as chances of overshooting are less.
  //Added newData if logic to only call it once per new data received.

    while (steps>counter)
    {
         if (dir=='F') //Single quotes - Double quotes don't work.
            {
                if (not (motorARunning)) {rotateF();}
             }
         else if (dir=='R')
            {
               if (not (motorARunning)) {rotateR();}
            }
     }
    if (motorARunning){rotateStop();}
             if (dir=='F')
            {
                encoder0Position = encoder0Position+counter;
             }
         else if (dir=='R')
            {
               encoder0Position = encoder0Position-counter;
            }
    newData = false;
    counter =0;
    Serial.print("Encoder position = ");
    Serial.println(encoder0Position);
        
    
}
