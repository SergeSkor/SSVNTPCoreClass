#pragma once

#include <Arduino.h>

#define NTP_NEVER_UPDATED 0xFFFFFFFF

typedef void (*CallbackFunction) ();
    
class SSVNTPCoreClass 
{
  private:
    SSVNTPCoreClass() = default; // Make constructor private

    uint32_t _NTPUpdateInterval = 3600000; //mS. Default 60 min (3600 sec, 3600000 mS).Not less than 15000 (15Sec)
    uint32_t _lastUpdate = NTP_NEVER_UPDATED;

    const char*   _TZString = "EST5EDT,M3.2.0,M11.1.0"; //default time zone New York.
    //see more here: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
    //and here: POSIX format, https://developer.ibm.com/articles/au-aix-posix/    
    
    const char*   _ServerName1 = "pool.ntp.org"; // Default timeserver1
    const char*   _ServerName2 = "time.nist.gov"; // Default timeserver2
    const char*   _ServerName3 = nullptr; // Default timeserver3

    time_t _timeNow = 0; 
    struct tm* _timeInfoStruct = nullptr;

    uint32_t _UpdFixTS=0; //timestamp when UpdateFix had been applied. To avoid multiple fixing within short time.

  protected:
    
  public:

    uint32_t UpdFixCNT=0; //for debug
    uint32_t UpdateCNT=0; //for debug
    
    static SSVNTPCoreClass &getInstance(); // Accessor for singleton instance

    SSVNTPCoreClass(const SSVNTPCoreClass &) = delete; // no copying
    SSVNTPCoreClass &operator=(const SSVNTPCoreClass &) = delete;

    void begin(const char* tzString, const char* server1,  const char* server2 = nullptr, const char* server3 = nullptr);
    void begin(); //use default TZ and servernames
    CallbackFunction _OnNTPTimeSetCB = NULL; //public!
    CallbackFunction _OnNTPTUpdIntervalResetCB = NULL; //public!
    
    void setOnTimeSetCB(CallbackFunction Value);
    void setOnUpdIntervalResetCB(CallbackFunction Value);
    void setUpdateInterval(uint32_t Value);
    uint32_t getUpdateInterval();
    void DoNeverCallSetLastUpdate();

    bool isNeverUpdated();
    uint32_t getLastUpdate();
    const char* getServerName(uint8_t index); //index [0,1,2]
    void setServerName(uint8_t index, const char* ServerName); //index [0,1,2]
    void setServerName(const char* server1,  const char* server2 = nullptr, const char* server3 = nullptr);
    
    bool WaitForFirstUpdate(uint32_t timeout_mS=5000, bool showProgress=false);  //sometime is not updated! Call ForceUpdate() required!
    const char* getTZString();
    void setTZString(const char* tzString, bool autoUpdate=false); //if no autoUpdate - call begin() needed to update;

    void refreshCache();
    void fixStopUpdating();
    void ForceUpdate();  //added 2/15/2021
    
    struct tm* getTimeInfoStruct();
    time_t getTimeNow(); //added 12/9/2020
    int getSeconds();
    int getMinutes();
    int getHours();   //24 format
    int getHours24(); //the same - 24 format
    int getHours12(); //12 format
    int getWeekDay(); //days of week since Sunday, [0-6], Monday=1
    int getYearDay(); //day in the year, range 0 to 365. Leap Year??
    int getYear();
    int getMonth();
    int getDay();
    bool isPM();
    bool isAM();
    bool isLeapYear();
    bool isDST(); //is summer time?
    //strings
    String getFormattedDateTimeString(const char* DateTimeFmt); //see strftime(..) for more information
    String getMonthFullStr();    //November
    String getMonthShortStr();   //Nov
    String getWeekdayFullStr();  //Sunday
    String getWeekdayShortStr(); //Sun
    String getTimeZone();  //EST (-0500)
    String getServerNames();
    
};

extern SSVNTPCoreClass &SSVNTPCore;
