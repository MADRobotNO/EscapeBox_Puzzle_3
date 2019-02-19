 // Puzzle 3
//By MAD Robot, Martin Agnar Dahl
//08.02.2019
//LCD: TM1637
//RFID: Start/Pause function
//ver 1.00 first run
//ver 1.05 Stabile

#include <Wiegand.h>
#include <TM1637.h>
#include <SPI.h>
#include <DFRobotDFPlayerMini.h>
#include <SoftwareSerial.h>

//SD
#include <SPI.h> //SD Card uses SPI
#include <SD.h>  //SD Card

#define CLK 4
#define DIO 5
#define DOORCHECK 42
#define KEYSAFE 7
#define CHESSCHECK 6

#define DEBUG

///Mp3
DFRobotDFPlayerMini mp3;

//sd
int chipSelect = 53;
File dataFile;

//Rounds counter
int counter1 = 0; //starts
int counter2 = 0; //wins
int counter3 = 0; //loses

//Timer variables
int sec = 1000;
long oldTimeS = 0;
int secCounter = 00;
int minCounter = 60;
char secChar[2];

//LCD
int8_t TimeDisp[] = {0x00,0x00,0x00,0x00};
TM1637 lcd(CLK,DIO);

//RFID
WIEGAND wg; //D0 - PIN2, D1 - PIN3

const char* gameStartTag = "1B02C8A7";  //Batman
const char* counterTag = "A315096B";
const char* counterResetTag = "43442B87";

//game variables
boolean gameStart = false;
boolean gamePause = false;
boolean onlyPauseDoor = true;
boolean chessOk = false;

//mp3 timer
long mp3timer = 0;

void setup() {
  
  #ifdef DEBUG
  Serial.begin(9600);
  Serial.println("Setup starts...");
  Serial.println();
  #endif
  
  //LCD
  lcd.init();
  lcd.set(BRIGHT_TYPICAL);
  //lcd.point(POINT_OFF);
  Serial.println("LCD done");

  //SPI
  SPI.begin();
  Serial.println("SPI done");
    
  //RFID
  wg.begin();
  Serial.println("RFID done");
  
  //Timer
  long timer = millis();

  //MP3 setup
  Serial2.begin(9600);
  mp3.begin(Serial2);
  mp3.outputDevice(DFPLAYER_DEVICE_SD);
  mp3.volume(25);
  
  //SD card and file
  Serial.println("Initializing SD card... ");
  if (!SD.begin(chipSelect)) {
    Serial.println("*****ERROR**********Card failed, or not present*****ERROR**********");
    Serial.println(' ');  
    delay(2000);
  }
  else{
    Serial.println("SD Card initialized successfully!");  
    Serial.println(' ');
  }

  //check if file exists
  if (SD.exists("data.txt")) {
    Serial.println("File data.txt exists....");
  } 
  else {
    dataFile = SD.open("data.txt", FILE_WRITE);
    Serial.println("File data.txt is now created...");
    dataFile.close();
    String startLogg = "0,0,0";
    logg(startLogg);
  }
  Serial.println("SD card and file are ready!");
  Serial.println("*************************************");
  
  String firstRead = readFile(); 
  readValues(firstRead);

  Serial.println("Counters are set up to:");
  Serial.println(counter1);
  Serial.println(counter2);
  Serial.println(counter3);
  Serial.println("*************************************");

  pinMode(KEYSAFE, OUTPUT);
  digitalWrite(KEYSAFE, HIGH);
  pinMode(CHESSCHECK, INPUT_PULLUP);
  pinMode(DOORCHECK, INPUT_PULLUP);

  pinMode(14, INPUT_PULLUP);
  
  Serial.println("Setp done!");
  Serial.println("");

}

void loop() {

  
  runFunction(readTag());
  
  while(gameStart){  
    long checkTimePassed = millis() - mp3timer;
    if(checkTimePassed> 185000 && checkTimePassed< 185050){
      mp3.stop();
      mp3.play(2);
      //Serial.println(millis());
      //Serial.println("play next");
    }
    
    //check if game is paused due to open door
    while(checkForPause()){
      Serial.print("GAME PAUSED...");
      delay(200);
    }
    
    timer();

    checkChess();
    checkForWin();
  }
  
}

void checkChess(){                                                       //CHECK IF CHESS is correct
  if (!chessOk){
    int check = digitalRead(CHESSCHECK);
    //Serial.print("Chesscheck: ");Serial.println(check);
    if (check==0){
      mp3.stop();
      mp3.play(3);
      digitalWrite(KEYSAFE, LOW);
      Serial.print("Chess correct! OPEN THE DOOR AND RUN FOR YOUR LIFE!");
      onlyPauseDoor = false;
      chessOk = true;
    }
  }
}

void checkForWin(){
  if (chessOk){
    int check = digitalRead(DOORCHECK);
    if(check == 1){
      mp3.stop();
      mp3.play(4);
      counter2++;
      String toLogg = String(counter1) + "," + String(counter2) + "," +  String(counter3);
      Serial.print("WIN - logged data: ");Serial.println(toLogg);
      logg(toLogg); 
      onlyPauseDoor = true;
      while(true){
        Serial.print(".");
        delay(2000);
        //infinite loop
      }
    }
  }
}

boolean checkForPause(){

  int check = digitalRead(DOORCHECK);
  Serial.print("DOORCHECK: ");Serial.print(check);Serial.print(" only pause: ");Serial.println(onlyPauseDoor);
  if(check==1 && onlyPauseDoor){
    return true;
  }
  return false;
  
}

void runFunction(String tagID){
  
  //check Counters
  if (tagID.equals(counterTag)){
    Serial.println("Check tag registered, showing counters on LCD");
    Serial.println(counter1);
    Serial.println(counter2);
    Serial.println(counter3);
    showCounters(counter1);
    delay(3000);
    showCounters(counter2);
    delay(3000);
    showCounters(counter3);
    delay(3000);
    tagID="";
  }

  //reset Counters
  else if (tagID.equals(counterResetTag)){
    Serial.println("Reset tag registered, Reseting all counters to 0");
    String toLogg = "0,0,0";
    Serial.print("RESET - logged data: ");Serial.println(toLogg);
    logg(toLogg );
    tagID="";
  }

  //game starts
  else if (tagID.equals(gameStartTag)){
    Serial.println("Start tag registered, game starts");
    counter1++;
    String toLogg = String(counter1) + "," + String(counter2) + "," +  String(counter3);
    Serial.print("START - logged data: ");Serial.println(toLogg);
    logg(toLogg);
    tagID="";
    gameStart = true;
    
    mp3.play(1);
    mp3timer = millis();
  }
  
}

String readTag(){
  String tagsID = "";
  if(wg.available()){
    delay(10);
    Serial.print("TagsID: ");Serial.println(wg.getCode(), HEX); //debug - cardID
    tagsID=String(wg.getCode(), HEX);     
    tagsID.toUpperCase();  
  }
  return tagsID;
}

void timer(){  
  long nowTime = millis();
  if (secCounter < 10){
    TimeDisp[3] = secCounter;
    TimeDisp[2] = 0;
  }
  
  else{
    char cstrS[2];
    itoa(secCounter, cstrS, 10);
    TimeDisp[2] = cstrS[0] - '0';
    TimeDisp[3] = cstrS[1] - '0';
  }
  
  if (minCounter < 10){
    TimeDisp[1] = minCounter;
    TimeDisp[0] = 0;
  }
  else{
    char cstrM[2];
    itoa(minCounter, cstrM, 10);
    TimeDisp[0] = cstrM[0] - '0';
    TimeDisp[1] = cstrM[1] - '0';
  }

  if ((nowTime - oldTimeS) >= sec){
    secCounter--;
    Serial.print(minCounter);
    Serial.print(":");
    Serial.print(TimeDisp[2]);
    Serial.println(TimeDisp[3]);
    oldTimeS=nowTime;
    updateTimer();
  }
  
  if (secCounter < 0){
    minCounter--;
    secCounter = 59;
  }
  
  if (minCounter <0){                       // GAME OVER
    counter3++;
    String toLogg = String(counter1) + "," + String(counter2) + "," +  String(counter3);
    Serial.print("LOOSE - logged data: ");Serial.println(toLogg);
    logg(toLogg); 
    minCounter=0;
    secCounter=0;    
    TimeDisp[1] = 0;
    TimeDisp[2] = 0;
    TimeDisp[3] = 0;
    TimeDisp[0] = 0;
    updateTimer();      
  }

  //Warnings
//  if (minCounter == 30 && secCounter == 00){
//    tone(buzzerPin, 700);
//    delay(500);
//    noTone(buzzerPin);
//  }
//
//  else if (minCounter == 15 && secCounter == 00){
//    tone(buzzerPin, 700);
//    delay(500);
//    noTone(buzzerPin);
//  }
//  
//  else if (minCounter == 10 && secCounter == 00){
//    tone(buzzerPin, 700);
//    delay(500);
//    noTone(buzzerPin);
//  }
//
//  else if (minCounter == 5 && secCounter == 00){
//    tone(buzzerPin, 700);
//    delay(500);
//    noTone(buzzerPin);
//  }
//
//  else if (minCounter == 0 && secCounter <= 3){
//    tone(buzzerPin, 500);
//    delay(500);
//    noTone(buzzerPin);
//  }
}

void updateTimer(){
  lcd.point(POINT_OFF);
  lcd.display(0, TimeDisp[0]);
  lcd.point(POINT_ON);
  lcd.display(1, TimeDisp[1]);
  lcd.point(POINT_OFF);
  lcd.display(2, TimeDisp[2]);
  lcd.display(3, TimeDisp[3]);
}

//***************************SD Logg Methods********************************//

void logg(String dataString){
  //String dataString = String(rounds);
  dataFile = SD.open("data.txt", O_WRITE | O_CREAT | O_TRUNC);
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
  }

  else {
    Serial.println("Error opening data file...");
  }
  
}

String readFile(){
  //Serial.println("Reading file");
  dataFile = SD.open("data.txt", FILE_READ);
  
  if (dataFile) {

    //check number of bytes and declare size of a buffer
    int bufSize = dataFile.available();
    char buf[bufSize-2];

    //read data from file to buffer array
    for (int i = 0; i<bufSize; i++){
      dataFile.read(buf, bufSize);
    }
    
    dataFile.close();

    //create an output String from buffer array
    String return_value = "";
    for (int i = 0; i<sizeof(buf); i++){
        return_value += String(buf[i]);
    }

    return return_value;
  } else {
    Serial.println("error opening test.txt");
  }
}

void readValues(String fileData){
  
  String val1 = "0";
  String val2 = "0";
  String val3 = "0";
  
  if(fileData.indexOf(",") > 0){
    val1 = fileData.substring(0, fileData.indexOf(","));
      fileData = fileData.substring(fileData.indexOf(",")+1);
      if(fileData.indexOf(",") > 0){
        val3 = fileData.substring(fileData.indexOf(",")+1);
        fileData = fileData.substring(0,fileData.indexOf(","));
        val2 = fileData;
      }
      else{
        val2 = fileData;
      }
  }
  else{
    val1 = fileData;
  }
  counter1 = val1.toInt();
  counter2 = val2.toInt();
  counter3 = val3.toInt();
  Serial.print("Readed values are: ");Serial.print(val1.toInt());Serial.print(val2.toInt());Serial.println(val3.toInt());
}

void showCounters(int number){
  int8_t dispVal[] = {0x00,0x00,0x00,0x00};

  if (number <10){
    char cstrM[1];
    itoa(number, cstrM, 10);
    dispVal[0] = 0;
    dispVal[1] = 0;
    dispVal[2] = 0;
    dispVal[3] = cstrM[0] - '0';
  }
  
  else if (number>9){
    char cstrM[2];
    itoa(number, cstrM, 10);
    dispVal[0] = 0;
    dispVal[1] = 0;
    dispVal[2] = cstrM[0] - '0';
    dispVal[3] = cstrM[1] - '0';
  }

  else if (number>99){
    char cstrM[3];
    itoa(number, cstrM, 10);
    dispVal[0] = 0;
    dispVal[1] = cstrM[0] - '0';
    dispVal[2] = cstrM[1] - '0';
    dispVal[3] = cstrM[2] - '0';
  }

  else if (number>999){
    char cstrM[4];
    itoa(number, cstrM, 10);
    dispVal[0] = cstrM[0] - '0';
    dispVal[1] = cstrM[1] - '0';
    dispVal[2] = cstrM[2] - '0';
    dispVal[3] = cstrM[3] - '0';
  }
    
  lcd.point(POINT_OFF);

  lcd.display(0, dispVal[0]);
  lcd.display(1, dispVal[1]);
  lcd.display(2, dispVal[2]);
  lcd.display(3, dispVal[3]);
  
}
