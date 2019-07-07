#define ENCODER0PINA         2    //Atmega pin PD2/INT0 // interrupt pin (2,3 on nano)
#define ENCODER1PINA         3    //Atmega pin PD3/INT1 // interrupt pin (2,3 on nano)
#define pinENA               4    //Atmega pin PD4
#define pinINA2              5    //Atmega pin PD5
#define pinINA1              6    //Atmega pin PD6
#define pinENB               7    //Atmega pin PD7
#define pinINB1              8    //Atmega pin PB0
#define pinINB2              9    //Atmega pin PB1

long encoder0Position = 0;
long encoder1Position = 0;
//int PPR0=2520; //pulses per rotation
//int PPR1=892; //pulses per rotation
int PWMA = 170;
int PWMB = 140;
// volatile variables - modified by interrupt service routine (ISR)
volatile long counter0=0;
volatile long counter1=0;
bool motorARunning = false;
bool motorBRunning = false;
long stepsA;
long stepsB;
int offshootA=900;//To do: calculate and adjust offsoot dynamically
int offshootB=200;
char dirA;
char dirB;
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
  pinMode(pinINA1,OUTPUT);
  pinMode(pinINA2,OUTPUT);
  pinMode(pinENB,OUTPUT);
  pinMode(pinINB1,OUTPUT);
  pinMode(pinINB2,OUTPUT);
  //attach interrupts 
  attachInterrupt(digitalPinToInterrupt(ENCODER0PINA),onInterrupt0, RISING);
  attachInterrupt(digitalPinToInterrupt(ENCODER1PINA),onInterrupt1, RISING); 
  Serial.begin (9600);

  delay(1000);
  //Turn on motor A at PWMA speed, calculate offsetA, Repeat for motor B
}
 
void loop()
{
  //Read the Serial and move accordingly
  recvWithStartEndMarkers();
  
  //showNewData();

      setPosition(DIR[0],VALUE);
      
  
  
  
  
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
void rotateAF()
{
    analogWrite(pinENA, PWMA);
    digitalWrite(pinINA1,HIGH);
    digitalWrite(pinINA2,LOW);
    motorARunning=true;
}
void rotateAR()
{
    analogWrite(pinENA, PWMA);
    digitalWrite(pinINA1,LOW);
    digitalWrite(pinINA2,HIGH);
    motorARunning=true;
}
void rotateAStop()
{
  analogWrite(pinENA, 0);
  motorARunning=false;
  delay(200);//Significant overshoot - even after stopping - counter keeps running. Printing counter 100 ms after setting it to 0 - still gives some values.
              //Hence a delay outputs actual overshot position of the encoder
}

void rotateBF()
{
    analogWrite(pinENB, PWMB);
    digitalWrite(pinINB1,HIGH);
    digitalWrite(pinINB2,LOW);
    motorBRunning=true;
}
void rotateBR()
{
    analogWrite(pinENB, PWMB);
    digitalWrite(pinINB1,LOW);
    digitalWrite(pinINB2,HIGH);
    motorBRunning=true;
}
void rotateBStop()
{
  analogWrite(pinENB, 0);
  motorBRunning=false;
  delay(200); //Significant overshoot - even after stopping - counter keeps running. Printing counter 100 ms after setting it to 0 - still gives some values.
              //Hence a delay outputs actual overshot position of the encoder 

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
  //Check if this data has already been seen

  if (newData)
  {
    // It is new data from Serial - it wants to move the motors one way or the other
    // if stepsA<offshootA - no point turning the motor. Repeat for stepsB     
         if (dir=='F') //Single quotes - Double quotes don't work.
            {
                if (not (motorARunning)) {rotateAF(); stepsA=steps-offshootA; dirA=dir;}
             }
         else if (dir=='R')
            {
               if (not (motorARunning)) {rotateAR();stepsA=steps-offshootA;dirA=dir;}
            }
         else if (dir=='f') //Single quotes - Double quotes don't work.
            {
                if (not (motorBRunning)) {rotateBF();stepsB=steps-offshootB;dirB=dir;}
             }
         else if (dir=='r')
            {
               if (not (motorBRunning)) {rotateBR();stepsB=steps-offshootB;dirB=dir;}
            }
    newData=false;
    }

    //Now motors are running - count steps
    if ((motorARunning) and (stepsA<counter0))
            {
             rotateAStop();
             
             
             if (dirA=='F')
            {
                encoder0Position = encoder0Position+counter0;

             }
             else if (dirA=='R')
            {
               encoder0Position = encoder0Position-counter0;
               
            }
            
        counter0 =0;
        Serial.print("0-");
        Serial.println(encoder0Position); 
            }
     
    if ((motorBRunning) and (stepsB<counter1))
    {
              rotateBStop();
              
              
             if (dirB=='f')
            {
                
                encoder1Position = encoder1Position+counter1;
             }
         else if (dirB=='r')
            {
               
               encoder1Position = encoder1Position-counter1;
            }
        counter1 =0;
        Serial.print("1-");
        Serial.println(encoder1Position); 
            }
     


}
