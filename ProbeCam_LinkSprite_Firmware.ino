#include <SoftwareSerial.h>
#include <SPI.h>
#include <SD.h>

// Pins
#define CAM_ON 4
#define ARDUINO_TX 2
#define ARDUINO_RX 3
#define LED 7

SoftwareSerial arduino(ARDUINO_TX, ARDUINO_RX);

int questionPositions[50];

String incomingCommand;

// Camera saving
#define ZERO 0x00
long int j = 0, k = 0, count = 0, i = 0x0000;
uint8_t MH, ML;
File  myFile;

void resetCamera() {
  Serial.write(0x56);
  Serial.write(ZERO);
  Serial.write(0x26);
  Serial.write(ZERO);
}

/* Set image size
 * <1> 0x22 : 160*120 
 * <2> 0x11 : 320*240
 * <3> 0x00 : 640*480
 * <4> 0x1D : 800*600
 * <5> 0x1C : 1024*768
 * <6> 0x1B : 1280*960
 * <7> 0x21 : 1600*1200
 */
void setImageSize(byte imageSize) {
  Serial.write(0x56);
  Serial.write(ZERO);
  Serial.write(0x54);
  Serial.write(0x01);
  Serial.write(imageSize);
}

/* Set baud rate
 * <1> 0xAE  :   9600
 * <2> 0x2A  :   38400
 * <3> 0x1C  :   57600
 * <4> 0x0D  :   115200
 * <5> 0xAE  :   128000
 * <6> 0x56  :   256000
 */
void setBaudRate(byte baudrate) {
  Serial.write(0x56);
  Serial.write(ZERO);
  Serial.write(0x24);
  Serial.write(0x03);
  Serial.write(0x01);
  Serial.write(baudrate);
}

void capturePhoto() {
  Serial.write(0x56);
  Serial.write(ZERO);
  Serial.write(0x36);
  Serial.write(0x01);
  Serial.write(ZERO);
}

void requestData() {
  MH = i / 0x100;
  ML = i % 0x100;
  Serial.write(0x56);
  Serial.write(ZERO);
  Serial.write(0x32);
  Serial.write(0x0c);
  Serial.write(ZERO);
  Serial.write(0x0a);
  Serial.write(ZERO);
  Serial.write(ZERO);
  Serial.write(MH);
  Serial.write(ML);
  Serial.write(ZERO);
  Serial.write(ZERO);
  Serial.write(ZERO);
  Serial.write(0x20);
  Serial.write(ZERO);
  Serial.write(0x0a);
  i += 0x20;
}

void StopTakePhotoCmd() {
  Serial.write(0x56);
  Serial.write(ZERO);
  Serial.write(0x36);
  Serial.write(0x01);
  Serial.write(0x03);
}

void disableCamera() {
  digitalWrite(CAM_ON, LOW);
  digitalWrite(0, LOW);
  digitalWrite(1, LOW);
}

void enableCamera() {
  digitalWrite(CAM_ON, HIGH);
}

void setup() {
  pinMode(LED, OUTPUT);
  pinMode(CAM_ON, OUTPUT);
  
  arduino.begin(9600);

  // Initialise SD card
  pinMode(10, OUTPUT);
  if (!SD.begin(10)) {
    digitalWrite(LED, HIGH);
    arduino.println(-2);
    while (1);
  }

  // Success LED
  for (int i = 0; i < 10; i++) {
    digitalWrite(LED, HIGH);
    delay(50);
    digitalWrite(LED, LOW);
    delay(50);
  }

  enableCamera();
  Serial.begin(115200);
  delay(100);
  //setImageSize(0x21);
  resetCamera();
  delay(2000);
  //setBaudRate(0x2A);
  //delay(500);
  //Serial.begin(38400);
  disableCamera();

  arduino.write('R');
}

void loop()
{
  while (arduino.available() > 0) {
    char inByte = arduino.read();
    //arduino.write(inByte);
    if (inByte == 0x0A) {
      processCommand(incomingCommand);
      incomingCommand = "";
    }
    else incomingCommand += (char)inByte;
  }
}

// Process incoming command from the Arduino.
void processCommand(String cmd) {
  if (cmd.charAt(0) == '?' ) arduino.println(getQuestion(cmd.substring(1).toInt()));
  else if (cmd.charAt(0) == '!') takePicture(cmd.substring(1).toInt());
  else if (cmd.charAt(0) == '$') arduino.println(getNumOfQuestions());
  else if (cmd.charAt(0) == 'S') sleep();
}

// Retrieve a question.
String getQuestion(int q) {
  String question;
  int currentQuestion = -1;

  myFile = SD.open("q.txt");

  if (!myFile) return "No questions found.";

  do {
    question = myFile.readStringUntil('\n');
    currentQuestion++;
  } while (currentQuestion != q);
  
  myFile.close();

  return question;
}

// Retrieve the total number of questions in the SD.
int getNumOfQuestions() {
  int result = 0;

  myFile = SD.open("q.txt");

  if (!myFile) return -1;

  // 
  while (myFile.available()) {
    char inChar = myFile.read();
    if(inChar == '\n') {
      result++;
      questionPositions[result] = myFile.position();
    }
  }
  myFile.close();
  
  return result + 1;
}

// Mark a question as completed.
void answerQuestion(int q) {
  myFile = SD.open("q.txt", FILE_WRITE);
  char thisChar;
  char nextChar;
  
  myFile.seek(questionPositions[q]);
  nextChar = myFile.peek();
  myFile.print("#");

  while (myFile.available()) {
    thisChar = nextChar;
    nextChar = myFile.peek();
    myFile.print(thisChar);
  }
  myFile.close();

  // Call this to reindex the questions.
  getNumOfQuestions();
}

// Take a picture.
void takePicture(int q) {
  byte incomingbyte;
  j = 0; k = 0; count = 0; i = 0x0000;
  boolean EndFlag = 0;
  byte a[32];
  int ii;

  // Capture photograph
  enableCamera();
  delay(10);
  resetCamera();
  delay(50);
  capturePhoto();
  delay(1000);
  while (Serial.available() > 0)
  {
    incomingbyte = Serial.read();
  }

  // Construct file name
  char filename[13];
  strcpy(filename, "Q00-00.JPG");
  for (int i = 0; i < 100; i++) {
    filename[1] = '0' + q / 10;
    filename[2] = '0' + q % 10;
    filename[4] = '0' + i / 10;
    filename[5] = '0' + i % 10;
    // create if does not exist, do not open existing, write, sync after write
    if (!SD.exists(filename)) {
      break;
    }
  }

  // Open file
  myFile = SD.open(filename, FILE_WRITE);

  // Request data and write to SD
  while (!EndFlag)
  {
    j = 0;
    k = 0;
    count = 0;
    //Serial.flush();
    requestData();
    delay(30);
    while (Serial.available() > 0)
    {
      incomingbyte = Serial.read();
      k++;
      //            delay(1); //250 for regular
      if ((k > 5) && (j < 32) && (!EndFlag))
      {
        a[j] = incomingbyte;
        if ((a[j - 1] == 0xFF) && (a[j] == 0xD9)) //tell if the picture is finished
        {
          EndFlag = 1;
        }
        j++;
        count++;
      }
      digitalWrite(7, !digitalRead(7));
    }

    for (ii = 0; ii < count; ii++)
      myFile.write(a[ii]);
  }

  myFile.close();

  answerQuestion(q);

  digitalWrite(7, HIGH);
  delay(100);
  digitalWrite(7, LOW);
  delay(400);
  digitalWrite(7, HIGH);
  delay(100);
  digitalWrite(7, LOW);
  delay(400);
  digitalWrite(7, HIGH);
  delay(100);
  digitalWrite(7, LOW);
  delay(400);

  disableCamera();

  arduino.println("R");
}

void sleep() {
  digitalWrite(LED, HIGH);
  delay(10);
  digitalWrite(LED, LOW);
  //disableADC();
  //enableSleepMode();
  //disableBOD();
  //__asm__ __volatile__("sleep");
}

void disableADC() {
  ADCSRA &= ~(1 << 7);
}

void enableADC() {
  ADCSRA |= (1 << 7);
}

void enableSleepMode() {
  SMCR |= (1 << 2);
  SMCR |= 1;
}

void disableBOD() {
  MCUCR |= (3 << 5);
  MCUCR = (MCUCR & ~(1 << 5)) | (1 << 6);
}

