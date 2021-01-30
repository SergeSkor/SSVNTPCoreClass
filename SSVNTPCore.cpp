#include "SSVNTPCore.h"
#include <Arduino.h>
#include <coredecls.h>   // settimeofday_cb()
#include <ESP8266WiFi.h>  //otherwise, sntp_update_delay_MS_rfc_not_less_than_15000() is not working, but compiled.

#define BUFFER_SIZE_SSVNTPCoreClass 40 //buffer size to convert to Strings
char buff [BUFFER_SIZE_SSVNTPCoreClass]; 

//Global callback function to change NTP update interval. Default is 60 min. 
//Do not need to setup, but requires #include <ESP8266WiFi.h> to work.
uint32_t sntp_update_delay_MS_rfc_not_less_than_15000 ()
{
  if (SSVNTPCore._OnNTPTUpdIntervalResetCB != NULL) SSVNTPCore._OnNTPTUpdIntervalResetCB();//call callback
  return SSVNTPCore.getUpdateInterval();
}

////Global callback function called every time when ntp time is successfully set. See settimeofday_cb()
void gTimeIsSet() 
{   
  SSVNTPCore.DoNeverCallSetLastUpdate();
  if (SSVNTPCore._OnNTPTimeSetCB != NULL) SSVNTPCore._OnNTPTimeSetCB();
}

//-------------------- begin of SSVNTPCoreClass ---------
void SSVNTPCoreClass::begin(const char* tzString, const char* server1,  const char* server2, const char* server3) 
{
  _TZString = tzString;
  _ServerName1 = server1;
  _ServerName2 = server2;
  _ServerName3 = server3;

  begin();
}

void SSVNTPCoreClass::begin() //use default (already set) TZ and servernames
{

  //configtime() is declared in C:\Documents and Settings\Serge\AppData\Local\Arduino15\packages\esp8266\hardware\esp8266\2.7.4\cores\esp8266\Arduino.h
  // void configTime(int timezone, int daylightOffset_sec, const char* server1,  const char* server2 = nullptr, const char* server3 = nullptr);
  // void configTime(const char* tz, const char* server1,  const char* server2 = nullptr, const char* server3 = nullptr);
  // examples:  
  //  configTime(timezone*3600, dst*60, "pool.ntp.org", "time.nist.gov");
  //  configTime(MYTZ, "pool.ntp.org", "time.nist.gov" );

  configTime(_TZString, _ServerName1, _ServerName2 , _ServerName3);
  //
  _timeNow = time(nullptr); //time_t now; time(&now); //also works
  _timeInfoStruct = localtime(&_timeNow); //fill timeinfo structure for specified time-zone
}

SSVNTPCoreClass &SSVNTPCoreClass::getInstance() 
{
  static SSVNTPCoreClass instance;
  settimeofday_cb(gTimeIsSet);  //set cb function to be called every time when time is set.
  return instance;
}

void SSVNTPCoreClass::setOnTimeSetCB(CallbackFunction Value)
{
  _OnNTPTimeSetCB = Value;
}

void SSVNTPCoreClass::setOnUpdIntervalResetCB(CallbackFunction Value)
{
  _OnNTPTUpdIntervalResetCB = Value;
}

void SSVNTPCoreClass::setUpdateInterval(uint32_t Value) //Not less than 15000 (15Sec)
{
  uint16_t MinUpdInterval = 15000;
  if (Value >= MinUpdInterval) _NTPUpdateInterval = Value;
   else _NTPUpdateInterval = MinUpdInterval;
}

uint32_t SSVNTPCoreClass::getUpdateInterval()
{
  return _NTPUpdateInterval;
}

bool SSVNTPCoreClass::isNeverUpdated()
{
  return (_lastUpdate == NTP_NEVER_UPDATED);
}

uint32_t SSVNTPCoreClass::getLastUpdate()
{
  return _lastUpdate;
}

void SSVNTPCoreClass::DoNeverCallSetLastUpdate()
{
  _lastUpdate = millis();
  UpdateCNT++;
}

const char* SSVNTPCoreClass::getServerName(uint8_t index) //index [0,1,2]
{
  switch (index)
    {
    case 0: return _ServerName1; break;
    case 1: return _ServerName2; break;
    case 2: return _ServerName3; break;
    default: return nullptr;
    }
}

void SSVNTPCoreClass::setServerName(uint8_t index, const char* ServerName) //index [0,1,2]
{
  switch (index)
    {
      case 0: _ServerName1 = ServerName; break;
      case 1: _ServerName1 = ServerName; break;
      case 2: _ServerName1 = ServerName; break;
      default: ;
    }
}

void SSVNTPCoreClass::setServerName(const char* server1,  const char* server2, const char* server3)
{
  _ServerName1 = server1;
  _ServerName2 = server2;
  _ServerName3 = server3;
}

bool SSVNTPCoreClass::WaitForFirstUpdate(uint32_t timeout_mS, bool showProgress)
{
////////////////////////
uint32_t aa=millis();
bool res = true;
while (isNeverUpdated())
  {
  if (showProgress) {Serial.print("*");}
  if ((millis() - aa) > timeout_mS) 
    {
    res = false;
    break;
    }
  delay(100);
  } 
if (showProgress) {Serial.println();}
return res;
}

const char* SSVNTPCoreClass::getTZString()
{
  return _TZString;
}

void SSVNTPCoreClass::setTZString(const char* tzString, bool autoUpdate)  //if no autoUpdate - call begin() needed to update;
{
  _TZString = tzString;
  if (autoUpdate) begin();
}

void SSVNTPCoreClass::refreshCache()
{
  fixStopUpdating(); //fix, if no updating.
  time_t t = time(nullptr); //time_t now; time(&now); //also works
  if (t != _timeNow) 
    {
    _timeNow = t;
    _timeInfoStruct = localtime(&_timeNow); //fill timeinfo structure for specified time-zone
    }
}

void SSVNTPCoreClass::fixStopUpdating()
{
//NTP time sometimes stop to update (seems like after 12 hours of running)
//To restore auto-updating (for next 12 hours) configTime(...) needs to be called once. This function is to do it automatically.
//By default it is called when cache is updating, so when actual time is requested. Such as updating needs some time - actual time may be reported before update recovered, 
//   which does not seems to be a problem, but theoretically may affect accuracy. 
//As an alternative - this function also may be called from loop()

uint32_t NowIs=millis();
if (  (NowIs - getLastUpdate()) >= (getUpdateInterval() + 100000) )  //if update age is more than by 100 sec bigger than update interval - run the fix. 100 sec - extra time gap
  {
  if (NowIs - _UpdFixTS >= 1000) //do not run fix too often, not frequently than once in a second
    {
    configTime(_TZString, _ServerName1, _ServerName2 , _ServerName3);  //to fix non-updating
    UpdFixCNT++;
    _UpdFixTS=NowIs;
    }
  }
}


struct tm* SSVNTPCoreClass::getTimeInfoStruct()
{
  //if (isNeverUpdated()) return //???
  refreshCache();
  return _timeInfoStruct;
}

time_t SSVNTPCoreClass::getTimeNow() //added 12/9/2020
{
  if (isNeverUpdated()) return 0; 
  refreshCache();
  return _timeNow;
}


int SSVNTPCoreClass::getSeconds()
{
  if (isNeverUpdated()) return 0;
  refreshCache();
  return _timeInfoStruct->tm_sec;
}

int SSVNTPCoreClass::getMinutes()
{
  if (isNeverUpdated()) return 0;
  refreshCache();
  return _timeInfoStruct->tm_min;
}

int SSVNTPCoreClass::getHours() //24 format
{
  return getHours24();
}

int SSVNTPCoreClass::getHours24() //24 format
{
  if (isNeverUpdated()) return 0;
  refreshCache();
  return _timeInfoStruct->tm_hour;
}

int SSVNTPCoreClass::getHours12() //12 format
{
  if (isNeverUpdated()) return 0;
  refreshCache();
  if( _timeInfoStruct->tm_hour == 0 ) return 12; // 12 midnight
    else if( _timeInfoStruct->tm_hour  > 12)  return _timeInfoStruct->tm_hour - 12 ;
      else return _timeInfoStruct->tm_hour ;  
}

int SSVNTPCoreClass::getWeekDay() //days since Sunday 0-6, Monday=1
{
  if (isNeverUpdated()) return 0;
  refreshCache();
  return _timeInfoStruct->tm_wday;
}

int SSVNTPCoreClass::getYearDay() //day in the year, range 0 to 365. Leap Year??
{
  if (isNeverUpdated()) return 0;
  refreshCache();
  return _timeInfoStruct->tm_yday + 1;
}

int SSVNTPCoreClass::getYear()
{
  if (isNeverUpdated()) return 0;
  refreshCache();
  return _timeInfoStruct->tm_year + 1900;
}

int SSVNTPCoreClass::getMonth()
{
  if (isNeverUpdated()) return 0;
  refreshCache();
  return _timeInfoStruct->tm_mon + 1;
}

int SSVNTPCoreClass::getDay()
{
  if (isNeverUpdated()) return 0;
  refreshCache();
  return _timeInfoStruct->tm_mday;
}

bool SSVNTPCoreClass::isPM()
{
  if (isNeverUpdated()) return false;
  refreshCache();
  return (_timeInfoStruct->tm_hour >= 12);
}

bool SSVNTPCoreClass::isAM()
{
  return !(isPM());
}

bool SSVNTPCoreClass::isLeapYear()
{
  if (isNeverUpdated()) return false;
  int Y = getYear();
  //return ( (Y>0) && !(Y%4) && ( (Y%100) || !(Y%400) ) );
  if (Y % 400 == 0) return true;
  if (Y % 100 == 0) return false;
  if (Y % 4 == 0) return true;
  return false;
}

bool SSVNTPCoreClass::isDST()  //is summer time?
{
  if (isNeverUpdated()) return false;
  refreshCache();
  return (_timeInfoStruct->tm_isdst);
 
}

String SSVNTPCoreClass::getFormattedDateTimeString(const char* DateTimeFmt)
{
  if(isNeverUpdated()) return String("N/A");
  refreshCache();
  strftime (buff, BUFFER_SIZE_SSVNTPCoreClass, DateTimeFmt, _timeInfoStruct);  //fill the buffer
  return String(buff);
}

String SSVNTPCoreClass::getMonthFullStr()
{
  return getFormattedDateTimeString("%B");
}

String SSVNTPCoreClass::getMonthShortStr()
{
  return getFormattedDateTimeString("%b");
}

String SSVNTPCoreClass::getWeekdayFullStr()
{
  return getFormattedDateTimeString("%A");
}

String SSVNTPCoreClass::getWeekdayShortStr() 
{
  return getFormattedDateTimeString("%a");
}

String SSVNTPCoreClass::getTimeZone()
{
  return getFormattedDateTimeString("%Z (%z)");
}

String SSVNTPCoreClass::getServerNames()
{
  //buffer size 40 is not enough ????
  String S1, S2, S3;
  if (getServerName(0) != NULL) S1=String(getServerName(0)); else S1=String("N/A");
  if (getServerName(1) != NULL) S2=String(getServerName(1)); else S2=String("N/A");
  if (getServerName(2) != NULL) S3=String(getServerName(2)); else S3=String("N/A");
  
  snprintf(buff, BUFFER_SIZE_SSVNTPCoreClass, "%s; %s; %s.", S1.c_str(), S2.c_str(), S3.c_str() );
  return String(buff);
}

SSVNTPCoreClass &SSVNTPCore = SSVNTPCore.getInstance(); //create object
//-------------------- end of SSVNTPCoreClass ---------


// https://www.jtbullitt.com/tech/cheats/ctime-cheat-sheet.pdf

/*
struct tm
Time structure
Structure containing a calendar date and time broken down into its components.

The structure contains nine members of type int (in any order), which are:

Member  Type  Meaning Range
tm_sec  int seconds after the minute  0-60*
tm_min  int minutes after the hour  0-59
tm_hour int hours since midnight  0-23
tm_mday int day of the month  1-31
tm_mon  int months since January  0-11
tm_year int years since 1900  
tm_wday int days since Sunday 0-6
tm_yday int days since January 1  0-365
tm_isdst  int Daylight Saving Time flag 

struct tm {
   int tm_sec;         // seconds,  range 0 to 59 
   int tm_min;         // minutes, range 0 to 59  
   int tm_hour;        // hours, range 0 to 23    
   int tm_mday;        // day of the month, range 1 to 31
   int tm_mon;         // month, range 0 to 11           
   int tm_year;        // The number of years since 1900 
   int tm_wday;        // day of the week, range 0 to 6  
   int tm_yday;        // day in the year, range 0 to 365
   int tm_isdst;       // daylight saving time           
};
*/

/*
strftime format
C string containing any combination of regular characters and special format specifiers. These format specifiers are replaced by the function to the corresponding values to represent the time specified in timeptr. They all begin with a percentage (%) sign, and are:
specifier  Replaced by Example
%a  Abbreviated weekday name *  Thu
%A  Full weekday name * Thursday
%b  Abbreviated month name *  Aug
%B  Full month name * August
%c  Date and time representation *  Thu Aug 23 14:55:02 2001
%C  Year divided by 100 and truncated to integer (00-99)  20
%d  Day of the month, zero-padded (01-31) 23
%D  Short MM/DD/YY date, equivalent to %m/%d/%y 08/23/01
%e  Day of the month, space-padded ( 1-31)  23
%F  Short YYYY-MM-DD date, equivalent to %Y-%m-%d 2001-08-23
%g  Week-based year, last two digits (00-99)  01
%G  Week-based year 2001
%h  Abbreviated month name * (same as %b) Aug
%H  Hour in 24h format (00-23)  14
%I  Hour in 12h format (01-12)  02
%j  Day of the year (001-366) 235
%m  Month as a decimal number (01-12) 08
%M  Minute (00-59)  55
%n  New-line character ('\n') 
%p  AM or PM designation  PM
%r  12-hour clock time *  02:55:02 pm
%R  24-hour HH:MM time, equivalent to %H:%M 14:55
%S  Second (00-61)  02
%t  Horizontal-tab character ('\t') 
%T  ISO 8601 time format (HH:MM:SS), equivalent to %H:%M:%S 14:55:02
%u  ISO 8601 weekday as number with Monday as 1 (1-7) 4
%U  Week number with the first Sunday as the first day of week one (00-53)  33
%V  ISO 8601 week number (01-53)  34
%w  Weekday as a decimal number with Sunday as 0 (0-6)  4
%W  Week number with the first Monday as the first day of week one (00-53)  34
%x  Date representation * 08/23/01
%X  Time representation * 14:55:02
%Y  Year  2001
%z  ISO 8601 offset from UTC in timezone (1 minute=1, 1 hour=100)
If timezone cannot be determined, no characters +100
%Z  Timezone name or abbreviation *
If timezone cannot be determined, no characters CDT
%%  A % sign  %
* The specifiers marked with an asterisk (*) are locale-dependent.
Note: Yellow rows indicate specifiers and sub-specifiers introduced by C99. Since C99, two locale-specific modifiers can also be inserted between the percentage sign (%) and the specifier proper to request an alternative format, where applicable:
Modifier  Meaning Applies to
E Uses the locale's alternative representation  %Ec %EC %Ex %EX %Ey %EY
O Uses the locale's alternative numeric symbols %Od %Oe %OH %OI %Om %OM %OS %Ou %OU %OV %Ow %OW %Oy
*/
