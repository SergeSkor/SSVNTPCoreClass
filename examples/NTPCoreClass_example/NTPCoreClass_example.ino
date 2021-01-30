#include "SSVNTPCore.h"
#include <ESP8266WiFi.h>

#include <FastLED.h> //for led blink only

#include <SSVTimer.h>
SSVTimer Tmr1; //to check for blocked delays
#define onboard_led 2 //D4, onboard LED, see FASTLED_ESP8266_RAW_PIN_ORDER above!
SSVTimer Tmr2; //to show time


const char* ssid = "*******";
const char* password = "*********";

//https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
//POSIX format, https://developer.ibm.com/articles/au-aix-posix/

#define NYTZ "EST5EDT,M3.2.0,M11.1.0"  //automatically consider DST ???, see configTime(...)
#define KievTZ "EET-2EEST,M3.5.0/3,M10.5.0/4"

void setup() 
{
  Tmr1.SetInterval(25);
  Tmr1.SetOnTimer(TmrFuncBlinkLED);
  Tmr1.SetEnabled(true);
  
  Tmr2.SetInterval(5000);
  Tmr2.SetOnTimer(TmrFuncShowTime);
  Tmr2.SetEnabled(true);
  
  pinMode(onboard_led, OUTPUT ); //led
  digitalWrite(onboard_led, HIGH); //off
  
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("\nConnecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) 
    {
    Serial.print(".");
    delay(100);
    }
  Serial.println("WiFi connected.");  


  SSVNTPCore.setOnTimeSetCB(MyTimeIsSetCB); //cb function called every time when ntp time is successfully set.
  SSVNTPCore.setOnUpdIntervalResetCB(MyUpdIntervalResetCB); //cb function called every time when ntp update interval is expired (reset).
  //SSVNTPCore.setUpdateInterval(15000); //min is 15000 (15 sec), default 3600000 (1 hour)

  
  SSVNTPCore.setTZString(NYTZ);
  SSVNTPCore.setServerName("pool.ntp.org", "time.nist.gov" );
  SSVNTPCore.begin();

  Serial.println("\nConnecting to NTP Pool");
  //while (SSVNTPCore.isNeverUpdated())
  //  {
  //  Serial.print("*");
  //  delay(100);
  //  }
  bool NTPres = SSVNTPCore.WaitForFirstUpdate(1000);
  if (NTPres) 
    Serial.println("NTP Time sync'ed  OK.");  
    else
     Serial.println("NTP Time is NOT sync'ed.");  
     
  Serial.print("TZ String: "); Serial.println(SSVNTPCore.getTZString());
  Serial.print("NTPServer#1: "); Serial.println(SSVNTPCore.getServerName(0));
  Serial.print("NTPServer#2: "); Serial.println(SSVNTPCore.getServerName(1));
  Serial.print("NTPServer#3: "); Serial.println(SSVNTPCore.getServerName(2));
  Serial.println("*****************");
}

void loop() 
{
  Tmr1.RefreshIt(); //to check for blocked delays
  Tmr2.RefreshIt();
}

void TmrFuncBlinkLED()
{
analogWrite(onboard_led, beatsin16(60, 0, 1023, 0 ) ); //sin,
//digitalWrite(onboard_led, !digitalRead(onboard_led) ); //invert
}

void TmrFuncShowTime()
{
  // time_t now = time(nullptr); 
  //time_t now; time(&now); //also works
  //ctime() function
  // Serial.print("Not a class: "); Serial.print(ctime(&now));  //Sun Nov  8 22:48:05 2020

  Serial.printf("year-month-day : %d-%d-%d \n", SSVNTPCore.getYear(), SSVNTPCore.getMonth(), SSVNTPCore.getDay() );
  
  Serial.printf("Day of Week: %d \n", SSVNTPCore.getWeekDay());
  Serial.printf("Day of Year: %d \n", SSVNTPCore.getYearDay());
                              
  Serial.printf("time24: %d:%d:%d \n", SSVNTPCore.getHours24(), SSVNTPCore.getMinutes(), SSVNTPCore.getSeconds());
  
  Serial.printf("time12: %d%s:%d:%d \n", SSVNTPCore.getHours12(),  SSVNTPCore.isPM()?"PM":"AM",  SSVNTPCore.getMinutes(), SSVNTPCore.getSeconds());

  Serial.printf("Leap Year: %s \n", SSVNTPCore.isLeapYear()?"Yes":"No");
  
  Serial.printf("DayLight Saving Time (summer): %s \n", SSVNTPCore.isDST()?"Yes":"No");

  Serial.println(SSVNTPCore.getFormattedDateTimeString("%A %r"));

  Serial.printf("TZ: %s \n", SSVNTPCore.getTimeZone().c_str() );

  Serial.printf("Month: %s %s\n", SSVNTPCore.getMonthFullStr().c_str(),  SSVNTPCore.getMonthShortStr().c_str() );

  Serial.printf("WeekDay: %s %s\n", SSVNTPCore.getWeekdayFullStr().c_str(), SSVNTPCore.getWeekdayShortStr().c_str() ); 

  Serial.printf("Servers List: %s\n", SSVNTPCore.getServerNames().c_str());

  Serial.printf("TZ String: \"%s\"\n", SSVNTPCore.getTZString() );

//*****************************************************************************
//Switching to another TimeZone will automatically trigger NTP Time update, cause connection to NTP servers.
//To avoid excessive connecting to NTP servers - fake not-existing NTP setvers names (or nullptr) may be substituted, timezone calculations are still working.
//Do not forget to restore valid NTP servers names, when real NTP update is needed.
//*****************************************************************************

  //Switch to another TimeZone
  //SSVNTPCore.setServerName(nullptr, nullptr, nullptr); //do not forget to restore when real NTP update is needed.
  //SSVNTPCore.setServerName("p--ool.ntp.org", "t--ime.nist.gov" ); //do not forget to restore when real NTP update is needed.
  
  SSVNTPCore.setTZString(KievTZ, true);  //auto-update
  Serial.printf("Kiev Date/Time24: %s\n", SSVNTPCore.getFormattedDateTimeString("%m/%d/%Y %T").c_str()); 
  
  //Switch to another TimeZone Pacific/Honolulu
  SSVNTPCore.setTZString("HST10", true);  //auto-update
  Serial.printf("Honolulu Date/Time24: %s\n", SSVNTPCore.getFormattedDateTimeString("%m/%d/%Y %T").c_str()); 

  //Switch to another TimeZone Mexico_City
  SSVNTPCore.setTZString("CST6CDT,M4.1.0,M10.5.0", true);  //auto-update
  Serial.printf("Mexico_City Date/Time24: %s\n", SSVNTPCore.getFormattedDateTimeString("%m/%d/%Y %T").c_str()); 
  
  //Restore NY TimeZone
  SSVNTPCore.setTZString(NYTZ, true);  //auto-update
  Serial.printf("New-York Date/Time24: %s\n", SSVNTPCore.getFormattedDateTimeString("%m/%d/%Y %T").c_str()); 


  Serial.println();
}

// callback function called when time data is retrieved. 
void MyTimeIsSetCB() 
{   
  // In this case we will print the new time to serial port, so the user can see it change
  Serial.printf("%d ---------> NTP Time has been set. \n", millis()); 
}

// callback function called when time update interval has been reset (expired)
void MyUpdIntervalResetCB()
{
  Serial.printf("%d ---> NTP Update Interval set to: %dmS \n", millis(), SSVNTPCore.getUpdateInterval());
}
