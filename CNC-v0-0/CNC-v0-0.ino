#define ENCODER0PINA         2     // interrupt pin (2,3 on nano)
#define ENCODER1PINA         3     // interrupt pin (2,3 on nano)
#define pinENA               6
#define pinIN1               4
#define pinIN2               5


long encoder0Position = 0;
long encoder1Position = 0;
//int PPR=2520; //pulses per rotation
int PWM = 255;
// volatile variables - modified by interrupt service routine (ISR)
volatile long counter0=0;
volatile long counter1=0;
bool motorARunning = false;
//Serial Read stuff
const byte numChars = 32;
char receivedChars[numChars]; // an array to store the received data
boolean newData = false;
char DIR[1] = {0};
//char DIR = 'Z';
long VALUE = 0;


void setup()
{
 
  // pins
  pinMode(ENCODER0PINA, INPUT_PULLUP);
  pinMode(ENCODER1PINA, INPUT_PULLUP);
  pinMode(pinENA,OUTPUT);
  pinMode(pinIN1,OUTPUT);
  pinMode(pinIN2,OUTPUT);

  //attach interrupts 
  attachInterrupt(digitalPinToInterrupt(ENCODER0PINA),onInterrupt0, RISING);
  attachInterrupt(digitalPinToInterrupt(ENCODER1PINA),onInterrupt1, RISING); 
  Serial.begin (9600);
  //?Intialize the PWM??
  analogWrite(pinENA, 0);
  digitalWrite(pinIN1,LOW);
  digitalWrite(pinIN2,LOW);
  delay(1000);
}
 
void loop()
{
  //Read the Serial and move accordingly
  recvWithStartEndMarkers();
  
  //showNewData();
  if (newData)
    {
      setPosition(DIR[0],VALUE);
      
    }
  
  
  
}
// Minimalistic ISR

void onInterrupt0()
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
  counter0++;

}
void onInterrupt1()
{
  counter1++;

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
                parseData();
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
  VALUE = atol(strtokIndx+2);   
 }

void setPosition(char dir, long steps)
{
  //During the first run @ full speed (PWM 255, 24V PSU) - Some interrupts are missed probably - and the rotation is slightly more than (I think) the number of steps
  //Subsequent repeated  calls to this function yields accurate positions. May be something to do with function initialization
    
    if (steps>counter0)
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
     else
     {
        if (motorARunning)
            {
             rotateStop();
             if (dir=='F')
            {
                encoder0Position = encoder0Position+counter0;
                encoder1Position = encoder1Position+counter1;
             }
         else if (dir=='R')
            {
               encoder0Position = encoder0Position-counter0;
               encoder1Position = encoder1Position+counter1;
            }
        
        counter0 =0;
        counter1 =0;

            }
      newData = false;
      Serial.print("Encoder 0 is at ");
      Serial.println(encoder0Position); 
      Serial.print("Encoder 1 is at ");
      Serial.println(encoder1Position); 
     }
}
