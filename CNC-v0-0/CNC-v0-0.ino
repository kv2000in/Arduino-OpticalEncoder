#define ENCODER0PINA         2      // interrupt pin (2,3 on nano)
#define ENCODER0PINB         12    // any digital pin
#define pinENA               3  //  will change
#define pinIN1               4
#define pinIN2               5

// volatile variables - modified by interrupt service routine (ISR)
volatile long encoder0Position = 0;
long previousPosition = 0;
long currentRotationCount=0; 
long prevRotationCount=0;
int PPR=2520; //pulses per rotation
int fPWM = 150;//forward PWM
int rPWM = 150;// reverse PWM
int setRPM = 60; // to do: based on PPR - set the optimal RPM dynamically
int readRPM = 0;
int counter=0;
bool PWMdetermined = false;
bool motorARunning = false;
unsigned long perStepMicros = 0;
unsigned long prevMicros= 0;
unsigned long currentMicros = 0;
//Serial Read stuff
const byte numChars = 32;
char receivedChars[numChars]; // an array to store the received data
boolean newData = false;
char DIR[32] = {0};
int VALUE = 0;

void setup()
{
 
  // pins
  pinMode(ENCODER0PINA, INPUT_PULLUP);
  pinMode(ENCODER0PINB, INPUT);
  pinMode(pinENA,OUTPUT);
  pinMode(pinIN1,OUTPUT);
  pinMode(pinIN2,OUTPUT);
  digitalWrite(pinIN1,LOW);
  digitalWrite(pinIN2,LOW);
  //attach interrupt 
  attachInterrupt(digitalPinToInterrupt(ENCODER0PINA),onInterrupt, RISING);
  Serial.begin (9600);
  delay(1000);
  prevMicros = micros();

}
 
void loop()
{

  //Find the optimal forward and reverse PWMs so that the RPM stays around the setRPM - so that the optical encoder works reliably. 
  findingPWM();
 
  //Read the Serial and move accordingly
  recvWithStartEndMarkers();
  parseData();
  //showNewData();
  if (newData){setPosition(VALUE);}

  
  
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
  int b = digitalRead(ENCODER0PINB);
 if(b==1)
  {
      encoder0Position--;

  }
  else if (b==0)
  {
     encoder0Position++;
    
  }

}

int findPWMforSetRPM(int myPWM){
/*call this function just once - at start up. 
This will give us the optimal PWM for keeping the RPM at or below 600
So that - for both directions - appropriate PWM is determined.
*/
 
  while (abs(readRPM-setRPM)>20)
  {
    
      whatistheRPMValue();
  
        //Adjust pwm every 2nd rotation
        if (counter==2)
          {
              counter = 0;

              if (readRPM<setRPM)
                  {
                      myPWM=myPWM+10;
                      analogWrite(pinENA,myPWM);
          
                   }
                else if (readRPM>setRPM)
                   {
                       myPWM = myPWM-10;
                      analogWrite(pinENA,myPWM);
                    }

         
           }
    
    }

    //return the current PWM value
    return myPWM;
    
    
  }
void findingPWM(){
  if (!(PWMdetermined))
  {

    //Start the motor in reverse direction
    rotateR();
    Serial.print("PWM for reverse = ");
    rPWM = findPWMforSetRPM(rPWM);
    Serial.println(rPWM);
    Serial.print("For RPM value = ");
    Serial.println(readRPM);
    //Reset the readRPM
    readRPM=0;
    //Start the motor in forward direction
    rotateF();
    Serial.print("PWM for forward = ");
    fPWM = findPWMforSetRPM(fPWM);
    Serial.println(fPWM);
    Serial.print("For RPM value = ");
    Serial.println(readRPM);
    //Reset the readRPM
    readRPM=0;

    PWMdetermined = true;
    //turn off the motor for now
    rotateStop();
    //reset the position counter (will add endstop logic here)
    currentRotationCount=0;
      //Once optimal speed of rotation has been established - now zero the encoder position (will need some ENDstop logic)
    previousPosition=0;
    encoder0Position = 0;
     }
  
  }

void rotateF()
{
    analogWrite(pinENA, fPWM);
    digitalWrite(pinIN1,HIGH);
    digitalWrite(pinIN2,LOW);
    motorARunning=true;
}
void rotateR()
{
    analogWrite(pinENA, rPWM);
    digitalWrite(pinIN1,LOW);
    digitalWrite(pinIN2,HIGH);
    motorARunning=true;
}
void rotateStop()
{
  analogWrite(pinENA, 0);
  motorARunning=false;
}
void whatistheRPMValue()
  {
    // only display position info if has changed
     if (encoder0Position != previousPosition )
        {

            currentRotationCount=encoder0Position/PPR;
            previousPosition = encoder0Position;
  
          }
      if (currentRotationCount != prevRotationCount)
          {
             currentMicros = micros();
             perStepMicros = currentMicros - prevMicros;
             prevMicros=currentMicros;
             readRPM =(1000000/perStepMicros)*60;
             counter++;
             prevRotationCount=currentRotationCount;
    
          }
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

void setPosition(int pos)
{
  //This code causes some end jitters - as in either direction - there is a slight overshoot and then it goes back and forth until position is reached.
  //Also - if the encoder0Position changes - by external movement or whatever - this tries to bring it back to the pos parameter
  //I have set the RPM to 60 but this gives very little torque. Less jitters at slower speed as chances of overshooting are less.
  //Added newData if logic to only call it once per new data received.
  while (encoder0Position!=pos)
  {
   if  (encoder0Position<pos)
    {
      if (not (motorARunning)) {rotateF();}
    }
   else if ((encoder0Position>pos)) 
    {
      if (not (motorARunning)) {rotateR();}
    } 
   }
   //position reached - now stop 
   if (motorARunning){rotateStop();}
   newData = false;
   
}
