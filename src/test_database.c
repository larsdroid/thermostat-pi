#include <stdio.h>

#include "ini_file.h"
#include "db.h"
#include "util.h"

/*
* Possible TODO's:
*  - print DB version number
*  - print table list
*  - print row count per table (this should be a separate job from the previous one)
*/

int main(int argc, char const *argv[]) {
    readIniFile();
    initalizeDatabase();

    insertTempHumFrom1900();
    printTempHumFrom1900();
    printCountTempHumFrom1900();
    deleteTempHumFrom1900();
    printCountTempHumFrom1900();

    printf("\n");

    ////////////////////////////////////////////////////////////////////////////////
    //             TEST ONE TIME SCHEDULE PARENT SEARCH FOR RIGHT NOW             //
    ////////////////////////////////////////////////////////////////////////////////
    struct tm localDateTime = getLocalTime();

    long long int schedId = fetchFixedScheduleParent(&localDateTime);
    printf("Current schedule parent record id: %d\n", schedId);

    ////////////////////////////////////////////////////////////////////////////////
    //              TEST ONE TIME SCHEDULE PARENT SEARCH FOR SUMMER               //
    ////////////////////////////////////////////////////////////////////////////////
    struct tm summerTime;
    summerTime.tm_year = 116;
    summerTime.tm_mon = 6;
    summerTime.tm_mday = 10;
    summerTime.tm_hour = 12;
    summerTime.tm_min = 0;
    summerTime.tm_sec = 0;
    mktime(&summerTime);

    long long int summerSchedId = fetchFixedScheduleParent(&summerTime);
    printf("Summer schedule parent record id: %d\n", summerSchedId);

    ////////////////////////////////////////////////////////////////////////////////
    //                  TEST FIXED SCHEDULE SEARCH FOR RIGHT NOW                  //
    ////////////////////////////////////////////////////////////////////////////////
    SCHEDULE schedule = fetchFixedSchedule(schedId, &localDateTime);

    printf("Current local date/time:\n");
    printf("                 ");
    printDateTime(&localDateTime);
    printf("\n");

    printf("Current fixed schedule:\n");
    printf("    temperature: %.1f\n", schedule.requestedTemperature);
    printf("    starts at:   ");
    printDateTime(&(schedule.from));
    printf("\n");

    ////////////////////////////////////////////////////////////////////////////////
    //                 TEST MONDAY MORNING FIXED SCHEDULE SEARCH                  //
    ////////////////////////////////////////////////////////////////////////////////
    struct tm mondayMorning;
    mondayMorning.tm_year = 116;
    mondayMorning.tm_mon = 9;
    mondayMorning.tm_mday = 31;
    mondayMorning.tm_hour = 1;
    mondayMorning.tm_min = 0;
    mondayMorning.tm_sec = 0;
    mktime(&mondayMorning);

    schedule = fetchFixedSchedule(schedId, &mondayMorning);

    printf("Fixed schedule for monday 31 OCT at 1AM:\n");
    printf("    temperature: %.1f\n", schedule.requestedTemperature);
    printf("    starts at:   ");
    printDateTime(&(schedule.from));
    printf("\n");

    ////////////////////////////////////////////////////////////////////////////////
    //                TEST ONE TIME SCHEDULE SEARCH FOR RIGHT NOW                 //
    ////////////////////////////////////////////////////////////////////////////////
    schedule = fetchOneTimeSchedule(&localDateTime);

    printf("Current one time schedule:\n");
    if (schedule.requestedTemperature != INVALID_FETCHED_TEMPERATURE) {
        printf("    temperature: %.1f\n", schedule.requestedTemperature);
        printf("    starts at:   ");
        printDateTime(&(schedule.from));
        printf("\n");
    }
    else {
        printf("    NOT FOUND\n");
    }

    ////////////////////////////////////////////////////////////////////////////////
    //                           FETCH CURRENT SCHEDULE                           //
    ////////////////////////////////////////////////////////////////////////////////
    schedule = fetchSchedule(&localDateTime);

    printf("Current schedule:\n");
    printf("    temperature: %.1f\n", schedule.requestedTemperature);
    printf("    starts at:   ");
    printDateTime(&(schedule.from));
    printf("\n");

    ////////////////////////////////////////////////////////////////////////////////
    //                 FETCH TEMPERATURE REQUEST FOR RIGHT NOW                    //
    ////////////////////////////////////////////////////////////////////////////////
    schedule = fetchTemperatureRequest(&localDateTime);

    printf("Current temperature request:\n");
    if (schedule.requestedTemperature != INVALID_FETCHED_TEMPERATURE) {
        printf("    temperature: %.1f\n", schedule.requestedTemperature);
        printf("    starts at:   ");
        printDateTime(&(schedule.from));
        printf("\n");
    }
    else {
        printf("    NOT FOUND\n");
    }

    ////////////////////////////////////////////////////////////////////////////////
    //              FETCH TEMPERATURE REQUEST FOR A SPECIFIC TIME                 //
    ////////////////////////////////////////////////////////////////////////////////
    struct tm thisMorning;
    thisMorning.tm_year = 116;
    thisMorning.tm_mon = 10;
    thisMorning.tm_mday = 04;
    thisMorning.tm_hour = 8;
    thisMorning.tm_min = 0;
    thisMorning.tm_sec = 0;
    mktime(&thisMorning);

    schedule = fetchTemperatureRequest(&thisMorning);

    printf("Specific temperature request:\n");
    if (schedule.requestedTemperature != INVALID_FETCHED_TEMPERATURE) {
        printf("    temperature: %.1f\n", schedule.requestedTemperature);
        printf("    starts at:   ");
        printDateTime(&(schedule.from));
        printf("\n");
    }
    else {
        printf("    NOT FOUND\n");
    }

    ////////////////////////////////////////////////////////////////////////////////
    //                            TEST DATE ARITHMETIC                            //
    ////////////////////////////////////////////////////////////////////////////////
    struct tm march1;
    march1.tm_year = 116;
    march1.tm_mon = 2;
    march1.tm_mday = 1;
    march1.tm_hour = 12;
    march1.tm_min = 0;
    march1.tm_sec = 0;
    mktime(&march1);

    printf("2016-03-01:\n");
    printf("                 ");
    printDateTime(&march1);
    printf("\n");

    addDays(&march1, -1);

    printf("2016-03-01 MINUS ONE DAY:\n");
    printf("                 ");
    printDateTime(&march1);
    printf("\n");

    closeDatabase();
    return 0;
}
