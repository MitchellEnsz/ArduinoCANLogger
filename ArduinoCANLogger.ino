#include <Canbus.h>
#include <mcp2515.h>
#include <SD.h>
#include <RTClib.h>


const int sdSelect = 5;
char dataFileName[32];
RTC_DS1307 rtc;
tCAN message;
unsigned long startTime = 0; // Program start time in ms since midnight
unsigned long lastFileSave = 0;
bool dataInBuffer;
File dataFile;

void dateTime(uint16_t* date, uint16_t* time); // Function for telling SD how to timestamp new files


void setup() {
  Serial.begin(9600); 

  // CAN
  //////////////////////////////////////////////////////////
  Serial.print("Initializing CAN... ");
  if(!Canbus.init(CANSPEED_500))
  {
    Serial.println("Can't init CAN");
    while(1);
  }
  Serial.println("CAN initialized");
  //////////////////////////////////////////////////////////
    
  // SD  
  //////////////////////////////////////////////////////////    
  Serial.print("Initializing SD card... ");
  if(!SD.begin(sdSelect))
  {
    Serial.println("Card failed, or not present");
    while(1);
    //TODO Serial logging mode
  }
  Serial.println("card initialized");
  //////////////////////////////////////////////////////////

  // RTC
  //////////////////////////////////////////////////////////
  Serial.print("Initializing RTC... ");
  if (!rtc.begin()) 
  {
    Serial.println("Couldn't find RTC");
    while (1);
    //TODO Use millis()
  }
  Serial.println("rtc initialized ");

  if (!rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
  //////////////////////////////////////////////////////////  

  DateTime now = rtc.now(); // get current time
  startTime = millis(); // get time since program started
  startTime = now.hour()*3600000 + now.minute()*60000 + now.second()*1000 - startTime; // now startTime + millis() is milliseconds since midnight
  sprintf(dataFileName, "%d%d%d%d.asc",now.month(),now.day(),now.hour(),now.minute()); // name file based on date&time
  Serial.print("Opening file: "); Serial.println(dataFileName); // debug
  SdFile::dateTimeCallback(dateTime); // tell SD how to get file timestamp
  dataFile = SD.open(dataFileName, FILE_WRITE); // create new logging file
  if(!dataFile)
  {
    Serial.println("Error opening file");
    while(1);
  }
  dataFile.println("base hex timestamps absolute"); // first line in .asc file idk
  dataFile.flush();
  lastFileSave = millis();

  char currentTime[32];
  sprintf(currentTime, "%d/%d/%d %d:%d:%d", now.month(),now.day(),now.year(),now.hour(),now.minute(),now.second());
  Serial.print("Date: ");Serial.println(currentTime);  
}

//********************************Main Loop*********************************//

void loop()
{
  if (mcp2515_check_message()) 
  {
    if (mcp2515_get_message(&message)) 
    {
      char messageString[64];
      sprintf(messageString, "%lu.%.3lu 1 %X Rx d %d", (startTime+millis())/1000, (startTime+millis())%1000, message.id, message.header.length);
      for(int i=0;i<message.header.length;i++) sprintf(messageString + strlen(messageString), " %.2X", message.data[i]);
      dataFile.println(messageString);
      dataInBuffer = true;
    }
  }
  if(dataInBuffer && (millis()-lastFileSave > 5000)) // Save data at least every 5s
  {
    dataFile.flush();
    lastFileSave = millis();
    dataInBuffer = false;
  }
}


void dateTime(uint16_t* date, uint16_t* time)
{
  DateTime now = rtc.now();

  // return date using FAT_DATE macro to format fields
  *date = FAT_DATE(now.year(), now.month(), now.day());

  // return time using FAT_TIME macro to format fields
  *time = FAT_TIME(now.hour(), now.minute(), now.second());
}