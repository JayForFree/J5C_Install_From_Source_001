//  j5c_install_from_source_001 -- build a LAMP+ system from source
//  Copyright (C) 2018  Jay A Carlson of J5C Marketing LLC.
//
// This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, version 3.
//
// This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//
// This program can automatically install a LAMP system + other stuff
// LAMP        := Linux, Apache Web Server, MariaDB, Php
// Other stuff := Perl, Ruby, Tcl/Tk, POCO C++ Libraries

#include <math.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <map>
#include <vector>
#include <limits>

#include "source/j5c_date.h"

using namespace J5C_DSL_Code;
using sstr = std::string;

//constant expressions
constexpr auto commandPosition = 19;
constexpr auto daysIn400Years = 146097L;
constexpr auto seconds_in_minute = 60;
constexpr auto seconds_in_hour   = 60 * seconds_in_minute;
constexpr auto seconds_in_day    = 24 * seconds_in_hour;
// the actual value is not constant, but we can average over 400 years to make it closer
constexpr auto average_seconds_in_year = (daysIn400Years * seconds_in_day)/400.0;


//constants
const sstr skip_value = "Skip_This_Install";

const sstr STG_NAME         = "stg";
const sstr KEY_COMPANY_NAME = "Section_a1_Abbr_CompanyName";
const sstr VAL_COMPANY_NAME = "zzz";
const sstr KEY_DEFLT_PREFIX = "Section_a2_The_Default_Prefix";
const sstr VAL_DEFLT_PREFIX = "/usr/local";
const sstr KEY_TOUSE_PREFIX = "Section_a3_Use_Prefix";
const sstr VAL_TOUSE_PREFIX = "false";
const sstr KEY_PATH_VERSION = "Section_a4_The_Path_Version";
const sstr VAL_PATH_VERSION = "001";
const sstr KEY_AN_OS_SYSTEM = "Section_a5_The_Operating_System";
const sstr VAL_AN_OS_SYSTEM = "Red Hat";
const sstr KEY_RUN_DEPENDCS = "Section_a6_Run_Dependencies";
const sstr VAL_RUN_DEPENDCS = "false";
const sstr KEY_PROTECT_MODE = "Section_a7_Enable_Protect_Mode";
const sstr VAL_PROTECT_MODE = "true";

const sstr APACHE_GROUP = "apache_ws";
const sstr APACHE_USER  = "apache_ws";

const sstr MARIADB_OWNER = "mysql";
const sstr MARIADB_GROUP = "mysql";
const sstr MARIADB_USER  = "mysql";

enum class OS_type { Selection_Not_Available = -1, No_Selection_Made = 0, CentOS = 1, Fedora = 2, Linux_Mint = 3, RedHat = 4, MaxOSX = 5};
enum class Mac_PM  { Selection_Not_Available = -1, No_Selection_Made = 0, Home_Brew = 0, MacPorts = 1 };


struct an_itemValues
{
    bool bDebug;
    bool bDoTests;
    bool bInstall;
    bool bProtectMode;
    bool bScriptOnly;
    bool bCompileForDebug;
    bool bSkip;

    int     result     = -1;
    int     step       = -1;
    int     debugLevel =  0;

    OS_type thisOSType = OS_type ::RedHat;

    time_t itemStartTime;
    time_t itemStop_Time;

    sstr compression;
    sstr sha256sum_Config;
    sstr sha256sum_FileName;
    sstr sha256sum_Real;

    //Paths
    sstr bldPath;
    sstr etcPath;
    sstr getPath;
    sstr rtnPath;
    sstr srcPath;
    sstr srcPathPNV;
    sstr srcPathInstallOS;
    sstr cpyStgPath;
    sstr stgPath;
    sstr tlsPath;
    sstr tmpPath;
    sstr usrPath;
    sstr wwwPath;

    //fileNames
    sstr fileName_Compressed;
    sstr fileName_Staged;
    sstr fileName_Build;
    sstr fileName_Notes;
    sstr fileName_Results;
    sstr fileName_Protection;

    sstr company;
    sstr programName;
    sstr programNameVersion;
    sstr ProperName;
    sstr version;
    sstr pathVersion;

    sstr cmakeRunPath;
    sstr perl5RunPath;
};

sstr multilineCommand(std::vector<sstr>& commands, bool remove_last_continuation_char = false) {

    sstr result = "";


    if (commands.size() > 0) {
        // this is to just get the type
        auto maxLen = commands[0].length();
        auto curLen = maxLen;
        auto padLen = maxLen;
        auto constPad = maxLen;
        constPad = 2;
        sstr padding = "";

        for (auto element : commands) {
            curLen = element.length();
            if (curLen > maxLen) {
                maxLen = curLen;
            }
        }

        bool first = true;
        for (auto element : commands) {
            curLen = element.length();
            padLen = maxLen - curLen + constPad;
            result.append(element);
            padding.clear();

            //This (padLen > 0) should always be true as long as constPad > 0
            // but if constPad is changed by a programmer in the future this will protect
            // future code changes from breaking code.

            if (padLen > 0) {
                padding = std::string(padLen, ' ');
                result.append(padding);
            }
            result.append("\\\n");
        }
    }
    if (remove_last_continuation_char) {
        result = result.substr(0, result.length() - 2);
    }
    return result;
}




sstr lowerCaseString(sstr& some_value)
{
    // unicode lower case conversions require
    // very specialized code, and this is not it
    // but it will handle the english words that
    // we need for this program.

    // I tested 6 versions of lowercase
    //   (this was code test number 4 and the fastest version)
    //   (turns out c++ string operations are much slower than c string operations)
    // Note you have to be in the millions of operations
    //   for it to even matter, and we only do a less than
    //   a thousand here.

    auto len = some_value.length();
    char strChar[len+1];
    strcpy(strChar, some_value.c_str());
    for (auto idx = 0ul; idx < len; ++idx)
    {
        strChar[idx] =  std::tolower(strChar[idx]);
    }
    sstr result {strChar};
    return result;
}

sstr upperCaseString(sstr& some_value)
{
    // unicode upper case conversions require
    // very specialized code, and this is not it
    // but it will handle the english words that
    // we need for this program.

    auto len = some_value.length();
    char strChar[len+1];
    strcpy(strChar, some_value.c_str());
    for (auto idx = 0ul; idx < len; ++idx)
    {
        strChar[idx] =  std::toupper(strChar[idx]);
    }
    sstr result {strChar};
    return result;

}

sstr trimLeftAndRight(sstr& inString, sstr& ws)
{
    sstr result;
    if (ws.length() == 0)
    {
        ws = " \n\r\t";
    }
    auto max = std::numeric_limits<unsigned long>::max();
    auto start = inString.find_first_not_of(ws);
    if (start != max)
    {
        result = inString.substr(start, inString.find_last_not_of(ws) - start + 1);
    }
    return result;
}

sstr getDigitsInStringAsString(sstr& some_value)
{
    sstr result;
    int oneChar;
    if (some_value.length() > 0) {
        for (auto idx = 0ul; idx < some_value.length(); ++idx) {
            if (std::isdigit(some_value.at(idx))) {
                oneChar = some_value.at(idx);
                result.append({static_cast<char>(oneChar)});
            }
        }
    }
    else
    {
        result = "";
    }
    return result;
}

sstr getValid_pVersion(sstr& some_value)
{
    // explanation of what this does...
    // input ""         output "001"
    // input "rr4gre4"  output "00044"
    // input "2"        output "002"
    // input "45"       output "045"
    // input "652"      output "652"
    // input "843xc"    output "00843"

    // in essence pad leading zeros to max length of string
    // only accept digits in string, except all letters.
    // prefer a length of 3 digits, but enable to grow to a max of 5 digits

    sstr result;
    int oneChar;
    auto maxWidth = some_value.length();
    if (maxWidth > 5) { maxWidth  = 5; }
    if (maxWidth < 3) { maxWidth  = 3; }

    for (auto idx = 0ul; idx < some_value.length(); ++idx)
    {
        if (std::isdigit(some_value.at(idx)))
        {
            oneChar =  some_value.at(idx);
            result.append({static_cast<char>(oneChar)});
        }
    }
    if ((result.length() < maxWidth) && (result.length() > 0))
    {
        auto pad_length = maxWidth - result.length();
        sstr pad_string = sstr(pad_length, '0');
        result = pad_string.append(result);
    }
    if (result.length() < 3)
    {
        // We want to make this safe so no-one deletes any real data
        //    by overwriting a good directory
        result = "x001";
    }
    if (result.length() > maxWidth)
    {
        result = result.substr(0, maxWidth);
    }
    return result;
}

sstr padLeftZeros(int max_width, int number)
{
    auto max = pow(10,max_width);
    bool preConditionsMet = false;
    sstr result;
    //pre setup
    sstr strNumber = std::to_string(number);
    unsigned long pad_length = static_cast<unsigned long>(max_width);

    //check preConditions
    if ((number > -1) && (number < max)) {
        preConditionsMet = true;
        pad_length = max_width - strNumber.length();
    }

    if (preConditionsMet) {
        if (pad_length > 0) {
            sstr pad_string = sstr(pad_length, '0');
            result = pad_string;
            result.append(strNumber);
        } else
        {
            result = strNumber;
        }
    }
    else
    {
        result = "Error in padLeftZeros(int max_width, int number);";
    }
    return result;
};

sstr stripCharFromString(sstr& inString, const char c)
{
    sstr result;
    auto max = inString.length();
    auto current = max - max;
    while (current < max)
    {
        if (inString.at(current) != c)
        {
            result.append(inString.substr(current, 1));
        }
        current += 1;
    }
    return result;
}

sstr removeCharFromStartOfString(sstr& str, char c )
{
    sstr result;
    if (((str.length() == 1) && (str.at(0) = c)) || (str.length() == 0))
    {
        result = "";
    }
    else {
        auto first = str.find_first_not_of(c);
        result = str.substr(first, str.length());
    }
    return result;
}

sstr removeCharFromEnd__OfString(sstr &str, char c)
{
    sstr result;
    if (((str.length() == 1) && (str.at(0) = c))  || (str.length() == 0))
    {
        result = "";
    }
    else {
        auto last = str.find_last_not_of(c);
        result = str.substr(0, last + 1);
    }
    return result;
}

sstr safeFilePath(sstr& path)
{
    sstr resultPath  = path;
    auto startPos1 = resultPath.find(' ');
    auto startPos2 = resultPath.find('\\');
    bool cont = true;
    int failSafe = 0;
    if (startPos1 == -1) { cont = false; }
    while (cont)
    {
        if (startPos1 != -1)
        {
            if (startPos2 + 1 == startPos1)
            {
                cont = false;
            }
            else
            {
                resultPath.replace(startPos1, 1, "\\ ");
                startPos1+=2;
                startPos1 = resultPath.find(' ',  startPos1);
                startPos2 = resultPath.find('\\', startPos1);
            }
        }
        failSafe+=1;

        // this should be adequate for most situations
        // it would probably be safe to reduce this
        // number to 100
        if (failSafe > 999) break;
    }
    return resultPath;
}

sstr unDelimitPath(sstr& path)
{
    sstr newPath = removeCharFromStartOfString(path, '\'');
    newPath = removeCharFromEnd__OfString(newPath, '\'');
    return newPath;
}

sstr delimitPath(sstr& path)
{
    sstr temp;
    temp = removeCharFromStartOfString(path, '\'');
    temp = removeCharFromEnd__OfString(temp, '\'');
    sstr newPath  = "'";
    newPath.append(temp);
    newPath.append("'");
    return newPath;
}

sstr joinPathParts(sstr& partA, sstr& partB)
{
    sstr fixed1, fixed2, path;
    fixed1 = removeCharFromStartOfString(partA,  '/');
    fixed1 = removeCharFromEnd__OfString(fixed1, '/');
    fixed2 = removeCharFromStartOfString(partB,  '/');
    fixed2 = removeCharFromEnd__OfString(fixed2, '/');
    return path = "/" + fixed1 + "/" + fixed2 +  "/";
}

sstr joinPathWithFile(sstr& partA, sstr& fileName)
{
    sstr fixed1, fixed2, path;
    fixed1 = removeCharFromStartOfString(partA,    '/');
    fixed1 = removeCharFromEnd__OfString(fixed1,   '/');
    fixed2 = removeCharFromStartOfString(fileName, '/');
    fixed2 = removeCharFromEnd__OfString(fixed2,   '/');
    return path = "/" + fixed1 + "/" + fixed2;
}

int ensure_Directory_main(sstr& fullCommand, int result, bool show = false)
{
    std::cout << "  " << fullCommand << std::endl;
    result = system(fullCommand.c_str());
    if (show) {
        if (result == 0) {
            std::cout << "  ->success on running command:" << std::endl;
        } else {
            std::cout << "  ->failure on running command:" << std::endl;
        }
    }
    return result;
}

int ensure_Directory_exists1(sstr& path)
{
    int result = 1;
    sstr command  = "mkdir -p ";
    sstr fullCommand = command + path;
    ensure_Directory_main(fullCommand, result);
    return result;
}

int ensure_Directory_exists2(sstr& basePath, sstr& path)
{
    int result = 1;
    sstr fullpath = basePath + path;
    if (basePath.substr(basePath.length()-1, 1) != "/")
    {
        sstr fullPath = basePath + "/" + path;
    }
    sstr command  = "mkdir -p ";
    sstr fullCommand = command + fullpath;
    ensure_Directory_main(fullCommand, result);
    return result;
}

bool directoryExists(const sstr& path)
{

    bool result = false;
    sstr command = "cd " + path;
    command.append(" 2> /dev/null ");
    int commandResult = system(command.c_str());
    if (commandResult == 0) result = true;
    return result;
}

time_t get_Time()
{
    time_t rawtime;
    time (&rawtime);
    return rawtime;
}

template <typename T>
sstr get_Time_Part(T timePart)
{
    sstr strTimePart = std::to_string(timePart);
    if (timePart < 10) {
        strTimePart = "0" + strTimePart;
    }
    return strTimePart;
}

sstr get_GmtOffset()
{
    time_t theTime;
    struct tm * timeinfo;
    time (&theTime);
    timeinfo = localtime (&theTime);

    long gmt    = timeinfo->tm_gmtoff;
    long gmtHoursOffset = gmt / 3600;

    sstr strGmtOff  = std::to_string(gmtHoursOffset);
    sstr offset;
    if (gmtHoursOffset > -1)
    {
        offset = get_Time_Part<long>(gmtHoursOffset);
        if (gmtHoursOffset == 0)
        {
            strGmtOff = "( " + offset + ")";
        } else
        {
            strGmtOff = "(+" + offset + ")";
        }
    }
    else
    {
        gmtHoursOffset *= -1;
        offset = get_Time_Part<long>(gmtHoursOffset);
        strGmtOff = "(-" + offset + ")";
    }
    return strGmtOff;
}

sstr get_Time_as_String(time_t theTime)
{
    struct tm * timeinfo;
    //if (theTime == 0)
    //{
    //    time (&theTime);
    //}
    timeinfo = localtime (&theTime);

    int hours   = timeinfo->tm_hour;
    int minutes = timeinfo->tm_min;
    int seconds = timeinfo->tm_sec;

    sstr strHours   = get_Time_Part<int>(hours);
    sstr strMinutes = get_Time_Part<int>(minutes);
    sstr strSeconds = get_Time_Part<int>(seconds);

    sstr time = strHours + ":" + strMinutes + ":" + strSeconds + " ";

    return time;
}

sstr make_fileName_dateTime(time_t theTime)
{
    sstr thefileTime = get_Time_as_String(theTime);
    j5c_Date d1{};
    sstr theDate = d1.strDate();
    theDate = theDate.replace(4,1,"_");
    theDate = theDate.replace(7,1,"_");
    theDate.append("_at_");
    theDate.append(thefileTime);
    theDate = theDate.replace(16,1,"_");
    theDate = theDate.replace(19,1,"_");
    if (theDate.length() > 22)
    {
        theDate = theDate.substr(0,22);
    }
    return theDate;
}

sstr getDuration(time_t stop, time_t start)
{
    // we are going to give a positive duration
    //   even if the parameters get switched

    long long secondsTotal = 0;
    if (start > stop)
    {
        secondsTotal = start - stop;
    }
    else
    {
        secondsTotal = stop - start;
    }

    // We are only showing 3 digits for the year,
    //    and if that is not enough we have other problems...
    int years = static_cast<int>(secondsTotal / average_seconds_in_year);
    long long remainingSeconds = static_cast<long long>(secondsTotal - (years * average_seconds_in_year));

    // We are only showing 3 digits for the year,
    //    and if that is not enough we have other problems...
    if (years > 999) years = 999;
    sstr strYears = padLeftZeros(3, years);

    // There can only be 3 digits here so we are safe with an int
    int days = static_cast<int>(remainingSeconds / seconds_in_day);
    remainingSeconds = remainingSeconds - (days * seconds_in_day);
    sstr strDays = padLeftZeros(3, days);

    // There can only be 2 digits here so we are safe with an int
    int hours = static_cast<int>(remainingSeconds / seconds_in_hour);
    remainingSeconds = remainingSeconds - (hours * seconds_in_hour);
    sstr strHours = padLeftZeros(2, hours);

    // There can only be 2 digits here so we are safe with an int
    int minutes = static_cast<int>(remainingSeconds / seconds_in_minute);
    remainingSeconds = remainingSeconds - (minutes * seconds_in_minute);
    sstr strMinutes = padLeftZeros(2, minutes);

    // There can only be 2 digits here so we are safe with an int
    int seconds = static_cast<int>(remainingSeconds);
    sstr strSeconds = padLeftZeros(2, seconds);

    sstr result = strYears;
    result.append(":");
    result.append(strDays);
    result.append(":");
    result.append(strHours);
    result.append(":");
    result.append(strMinutes);
    result.append(":");
    result.append(strSeconds);
    result.append(" ");
    return result;
}

int startNewLogSection(std::ofstream& file, sstr utc = "-7")
{
    int result = 1;  // assume failure
    if ( (file.is_open()) && (file.good()) )
    {
        j5c_Date thisDate{};
        file << std::endl << std::endl;
        file << " Status started on " << thisDate.strDate() << "  UTC = " << utc <<  std::endl;
        file << " " << std::endl;
        file << " Start    : Stop     : Duration         : Command / Comments" << std::endl;
        file << " HH:MM:SS : HH:MM:SS : YYY:DDD:HH:MM:SS :" << std::endl;
        file << " ===============================================================================================================================" << std::endl;
        result = 0; // success
    }
    else
    {
        std::cout << "!!!Error -- unable to create new log section -- " << std::endl;
    }
    return result;
}

int appendNewLogSection(sstr &fileName)
{
    j5c_Date thisDate{};
    std::ofstream file;
    int result = 0;
    file.open(fileName, std::ios::out | std::ios::app);
    if ((file.is_open()) && (file.good())) {
        sstr gmtOffset = get_GmtOffset();
        startNewLogSection(file, gmtOffset);
    }
    file.close();
    return result;
}

int create_file(const sstr& fileName)
{
    j5c_Date thisDate{};
    std::ofstream file;
    int result = 0;
    file.open(fileName, std::ios::out | std::ios::trunc );
    if ( (file.is_open()) && (file.good()) )
    {
        file << "# File Created on " << thisDate.strDate() << std::endl;
        file << "# " << std::endl;
        file << "# Copyright J5C Marketing LLC" << std::endl;
        file << "# Jay A Carlson" << std::endl;
        file << "# jay.a.carlson@gmail.com" << std::endl;
        file << "# 360-649-6218" << std::endl;
        file << "# " << std::endl;
        file << "# " << std::endl;
        file.close();
    }
    else
    {
        std::cout << "!!!Error -- unable to create file -- " << std::endl;
        result = 1;
    }

    sstr c1 = "eval \"uname -a >> ";
    sstr c2 = c1 + fileName + "\"";

    system(c2.c_str() );

    file.open(fileName, std::ios::out | std::ios::app );
    if ( (file.is_open()) && (file.good()) )
    {
        file << "# " << std::endl;
        file << "# " << std::endl;
        result += startNewLogSection(file);
    }
    else
    {
        std::cout << "!!!Error -- unable to append to file -- " << std::endl;
        result = 1;
    }

    file.close();
    return result;
}

int ensure_file(sstr &fileName)
{
    std::ofstream file;
    int result = 1;  // assume failure
    file.open(fileName, std::ios::out | std::ios::app);
    if (!(file.is_open()))
    {
        create_file(fileName);
        result = 0;
    }
    return result;
}

int write_file_entry(std::ofstream& file, const sstr& entry, time_t stop, time_t start, bool includeTime = false)
{
    int result = 0;
    if (includeTime)
    {
        sstr strStart = get_Time_as_String(start);
        sstr strStop  = get_Time_as_String(stop);
        sstr strDuration = getDuration(stop, start);
        file << " " << strStart << ": " << strStop << ": " << strDuration << ": " << entry << std::endl;
    }
    else
    {
        file << entry << std::endl;
    }
    return result;
}

int file_write_vector_to_file(sstr &fileName, std::vector <sstr> &vec_lines, bool includeTime = true)
{
    std::ofstream file;
    int result = 0;
    file.open(fileName, std::ios::out | std::ios::app );
    if ( (file.is_open()) && (file.good()) )
    {
        for (const auto& it: vec_lines)
        {
            time_t start = get_Time();
            time_t stop  = get_Time();

            if (it == "\n")
            {
                result += write_file_entry(file, "\n", stop, start, includeTime);
            }
            else
            {
                result += write_file_entry(file, it, stop, start, includeTime);
            }
        }
    }
    else
    {
        std::cout << "!!!Error -- unable to write vector contents to file -- " << std::endl;
    }
    file.close();
    return result;
}

int file_append_line(sstr &fileName, sstr &line, time_t stop, time_t start)
{
    std::ofstream file;
    int result = 1;
    bool withTime = true;
    file.open(fileName, std::ios::out | std::ios::app );
    if ( (file.is_open()) && (file.good()) )
    {
        result = write_file_entry(file, line, stop, start, withTime);
    }
    else
    {
        std::cout << "!!!Error -- unable to append line to file -- " << std::endl;
    }
    file.close();
    return result;
}

sstr getProperNameFromString(sstr& some_value)
{
    // make the first letter capital
    // make all other letters lowercase

    // in the effort of don't repeat yourself
    //   I would like to call lowerCaseString()
    //   and then uppercase the first charactor
    //   but the string to char arrays back to strings to char arrays
    //   make it sub-optimal

    auto len = some_value.length();
    char strChar[len+1];
    strcpy(strChar, some_value.c_str());
    for (auto idx = 0ul; idx < len; ++idx)
    {
        strChar[idx] =  std::tolower(strChar[idx]);
    }
    strChar[0] =  std::toupper(strChar[0]);
    sstr result {strChar};
    return result;
}

int file_append_results(sstr& fileName, an_itemValues& itemValues, int installResult, time_t stop)
{
    unsigned long width1 = 31;
    unsigned long width2 = 40;
    sstr fill1(width1,'.');
    sstr fill2(width2,'.');
    sstr ProperName = getProperNameFromString(itemValues.programName);
    std::ofstream file;
    sstr line;
    int result = 1;
    bool found = false;
    file.open(fileName, std::ios::out | std::ios::app );
    if ( (file.is_open()) && (file.good()) )
    {
        if (itemValues.programName.substr(0,12) == "Dependencies")
        {
            line = "Install " + itemValues.ProperName + fill1;
        }
        else
        {
            line = "Install " + itemValues.ProperName + "-" + itemValues.version + fill1;
        }


        if (itemValues.step < 0)
        {
            found = true;
        }

        if ((!found) && (itemValues.step < 10))
        {
            line = line.substr(0,width1) +  "step 0"  + std::to_string(itemValues.step) + fill2;
            found = true;
        }
        if ((!found) && (itemValues.step > 9))
        {
            line = line.substr(0,width1) +  "step "  + std::to_string(itemValues.step) + fill2;
            found = true;
        }
        line = line.substr(0,width2);

        sstr strResults = "......" + std::to_string(installResult);
        strResults = strResults.substr(strResults.length()-5,5);
        line += " : Result = " + strResults;
        if (installResult == -1)
        {
            if (itemValues.bDebug)
            {
                if (itemValues.debugLevel >= 5) {
                    line += " : Results Blocked by Debug.";
                } else {
                    line += " : Install Blocked by Debug.";
                }
            }
            else
            {
                line += " : Install Blocked..........";
            }
        }
        if (installResult == 0)
        {
                line += " : Good.....................";
        }
        if (installResult > 0)
        {
                line += " : What the heck?...........";
        }
        if (found) {
            result = write_file_entry(file, line, stop, itemValues.itemStartTime, true);
        }
    }
    else
    {
        std::cout << "!!!Error -- unable to append results to file -- " << std::endl;
    }
    file.close();
    return result;
}

int file_append_blank_lines(sstr& fileName, int count)
{
    std::ofstream file;
    int result = 1;
    std::vector<sstr> vec;
    file.open(fileName, std::ios::out | std::ios::app );

    if ( (file.is_open()) && (file.good()) )
    {
        while (count > 0)
        {
            count -= 1;
            file << std::endl;
        }
    }
    return result;
}

std::vector<sstr> readFile(sstr &fileName, unsigned long maxCount)
{
    unsigned long theCount = 0;
    std::ifstream file;
    std::vector<sstr> result;
    file.open(fileName, std::ios::in );
    sstr lineData;
    if ( (file.is_open()) && (file.good()) )
    {
        while (getline(file, lineData))
        {
            result.push_back(lineData + "\n");
            theCount += 1;
            if (theCount >= maxCount)
            {
                break;
            }
        }
    }
    else
    {
        std::cout << "!!!Error -- unable to read file -- " << std::endl;
    }
    file.close();
    return result;
}

bool prior_Results(sstr& fileNameResult, sstr& searchStr)
{
    auto count = 50000ul;
    bool result = false;
    sstr it_data;
    searchStr = getProperNameFromString(searchStr);
    auto max = std::numeric_limits<unsigned long>::max();

    std::vector<sstr> data = readFile(fileNameResult, count);
    for (const auto& it : data )
    {
        it_data =  it;

        auto found1 = it_data.find(searchStr);
        if ( found1 != max )
        {
            auto found2 = it_data.find("Result =");
            sstr temp1;
            if (found2 < max)
            {
                temp1 = it_data.substr(found2+8,it_data.length());
            }
            auto end = temp1.find_first_of(':');
            temp1 = temp1.substr(0, end - 1);
            unsigned long idx = 0;
            while (idx < temp1.length())
            {
                if (isdigit(temp1[idx])) break;
                if (temp1[idx] == '.') temp1[idx] = ' ';
                ++idx;
            }
            auto temp2 = std::stol(temp1, nullptr, 10);
            if (temp2 == 0)
            {
                result = true;
                break;
            }
        }
    }
    return result;
}

std::map<sstr, sstr> getProgramSettings(sstr& fileSettings)
{
    static sstr temp;
    static auto max = temp.max_size();
    auto maxLineCount = 500ul;
    std::map<sstr, sstr> result;
    sstr it_data;
    sstr delimiter;
    sstr key;
    sstr value;
    sstr ws = " \t\n\r";

    //load general default values into map

    result.emplace(std::pair<sstr , sstr >(KEY_COMPANY_NAME, VAL_COMPANY_NAME));
    result.emplace(std::pair<sstr , sstr >(KEY_DEFLT_PREFIX, VAL_DEFLT_PREFIX));
    result.emplace(std::pair<sstr , sstr >(KEY_TOUSE_PREFIX, VAL_TOUSE_PREFIX));
    result.emplace(std::pair<sstr , sstr >(KEY_PATH_VERSION, VAL_PATH_VERSION));
    result.emplace(std::pair<sstr , sstr >(KEY_AN_OS_SYSTEM, VAL_AN_OS_SYSTEM));
    result.emplace(std::pair<sstr , sstr >(KEY_RUN_DEPENDCS, VAL_RUN_DEPENDCS));
    result.emplace(std::pair<sstr , sstr >(KEY_PROTECT_MODE, VAL_PROTECT_MODE));

    int width = 32;
    std::cout << std::endl << std::endl;
    std::cout << "#    Listing Settings : Values (Defaults)" << std::endl;
    std::cout << "#=========================================================" << std::endl;
    for (auto element : result)
    {
        std::cout << ": " << std::setw(width) << element.first << " : " << element.second << std::endl;
    }

    std::cout << std::endl;
    std::vector<sstr> data = readFile(fileSettings, maxLineCount);
    for (const auto& it : data )
    {
        it_data = it;
        if (it_data.length() > 1)
        {
            delimiter = it_data.substr(0,1);
            if (delimiter != "#") {
                it_data = it_data.substr(1, it_data.length());
                auto split = it_data.find_first_of(delimiter);
                if (split < max) {
                    key = it_data.substr(0, split - 1);
                    key = trimLeftAndRight(key, ws);
                    value = it_data.substr(split+2, it_data.length());
                    value = trimLeftAndRight(value, ws);

                    std::cout << "Key     = ***" << key     << "***" << std::endl;
                    std::cout << "Value   = ***" << value   << "***" << std::endl;
                    std::cout << std::endl;

                    if (value.length() > 0) {
                        result[key] = value;
                    }
                }
            }
        }
    }

    std::cout << std::endl << std::endl;
    std::cout << "#    Listing Settings : Values (Loading Setting from File)" << std::endl;
    std::cout << "#=========================================================" << std::endl;
    for (auto element : result)
    {
        std::cout << ": " << std::setw(width) << element.first << " : " << element.second << std::endl;
    }
    std::cout << std::endl << std::endl;

    return result;
}

void section_already_loaded(sstr& programName, sstr& version)
{
    if (version.length() > 0) {
        std::cout << "This section: " << programName + "-" + version << " already loaded." << std::endl;
    }
    else
    {
        std::cout << "This section: " << programName << " already loaded." << std::endl;
    }
}

void print_blank_lines(int count)
{
    while (count > 0)
    {
        count-= 1;
        std::cout << std::endl;
    }
}

int do_command(sstr& fileName, std::vector<sstr>& vec, bool createScriptOnly = false)
{
    time_t start;
    time_t stop;
    sstr command;
    int result = 0;
    bool outputToFile = false;
    if (fileName.length()> 0)
    {
        outputToFile = true;
    }
    else
    {
        std::cout << "Doing commands will not be saved to file." << std::endl;
        std::cout << "Because no file name was given."           << std::endl;
    }
    for (const auto& it : vec )
    {
        start  = get_Time();
        sstr strStart = get_Time_as_String(start);
        result = 0;
        command = it;

        std::cout << " " << strStart << "Command: " << command << std::endl;

        if ((command.substr(0,1) != "#") && (command.substr(0,1) != ":")) {
            if (!createScriptOnly) {
                result += system(command.c_str());
            }
        }

        if ((result != 0) && (!((command.substr(0,3) == "apt") || (command.substr(0,3) == "yum"))) )
        {
            std::cout << "!!!Error -- Process terminated for safety..." << std::endl;
            break;
        }

        stop = get_Time();

        if (outputToFile) {
            //
            //  We need to position the output so that it
            //    is not under the start, stop and duration times.
            bool cont = true;
            auto startPos = command.find('\n');
            sstr temp = std::string(commandPosition + 4, ' ');
            sstr replacement = "\n";
            replacement.append(temp);
            while (cont)
            {
                if (startPos != -1) {
                    command.replace(startPos, 1, replacement, 0, replacement.length());
                }
                else
                {
                    cont = false;
                }
                startPos = command.find('\n', startPos + replacement.length() + 1);
            }
            file_append_line(fileName, command, stop, start);

            if ((result != 0) && (!((command.substr(0,3) == "apt") || (command.substr(0,3) == "yum"))) )
            {
                command = "!!!Error -- Process terminated for safety...";
                file_append_line(fileName, command, stop, start);
                break;
            }
        }
    }
    return result;
}

sstr get_sha256sum(an_itemValues& itemValues)
{
    sstr result  = "Sha256sum not found.";
    sstr positionCommand = std::string(commandPosition, ' ');
    auto length  = itemValues.sha256sum_Config.length();
    sstr sha256File = "sha256sum.txt";
    std::vector<sstr> vec;
    sstr it_data;
    sstr outPathFileName = joinPathWithFile(itemValues.stgPath, sha256File);
    sstr command = "rm -f " + itemValues.stgPath + "sha256sum.txt;";
    command.append("\n");
    command.append(positionCommand);
    command.append("sha256sum ");
    command.append(itemValues.stgPath);
    command.append( "* > ");
    command.append(outPathFileName);
    vec.emplace_back(command);
    do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    ensure_file(outPathFileName);
    vec.clear();
    // this is assuming there won't be more than 1000 different versions
    //  in the same directory -- potentially a bad decision, but unlikely
    vec = readFile(outPathFileName, 1000);
    for (const auto& it : vec)
    {
        it_data = it;
        auto find = it_data.find(itemValues.fileName_Compressed);
        if (find < it_data.length())
        {
            result = it_data.substr(0,length);
            break;
        }
    }
    return result;
}

bool check_Sha256sum(an_itemValues& itemValues)
{
    bool result = false;
    itemValues.sha256sum_Real = get_sha256sum(itemValues);
    sstr positionCommand = std::string(commandPosition, ' ');
    std::vector<sstr> vec;
    vec.clear();
    sstr command {"# Expected sha256sum_Config = "};
    command.append(itemValues.sha256sum_Config);
    vec.emplace_back(command);
    command.clear();
    command.append("# Actual   sha256sum_File   = ");
    command.append(itemValues.sha256sum_Real);
    vec.emplace_back(command);
    do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    if (itemValues.sha256sum_Real == itemValues.sha256sum_Config)
    {
        result = true;
    }
    return result;
}

int badSha256sum(an_itemValues& itemValues)
{
    std::vector<sstr> vec;
    vec.clear();
    vec.emplace_back("# ");
    vec.emplace_back("# Shaw256sums do not match: ");
    vec.emplace_back("#!!!--Warning--!!! Skipping installation due to security concerns !!! ");
    vec.emplace_back("#");
    do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    // just an arbitrary number greater than 0;
    return 2;
}


bool isFileSizeGtZero(an_itemValues itemValues, sstr &fileName, bool bShow = false)
{
    bool result = false;
    std::vector<sstr> vec;
    sstr positionCommand = std::string(commandPosition, ' ');
    sstr safeFileName = safeFilePath(fileName);
    sstr printCommand = "";
    auto startPos = fileName.find(" ");
    if (startPos == -1) {
        std::ifstream file(safeFileName, std::ios::in | std::ios::binary);
        file.seekg(0, file.end);
        auto worthless = file.tellg();
        auto real_size = static_cast<long long>(worthless);
        file.close();
        if (real_size > 0) { result = true; }
        if (real_size < 0) { real_size = 0; }
        if (bShow) {
            sstr size = std::to_string(real_size);
            if (real_size > 0) {
                vec.clear();
                printCommand = "# Found File: '";
                printCommand.append(fileName);
                printCommand.append("' \n");
                printCommand.append(positionCommand);
                printCommand.append("#   and size= ");
                printCommand.append(size);
                printCommand.append(".");
                vec.emplace_back(printCommand);
                result = true;
            } else {
                vec.clear();
                printCommand = "# Search File: '";
                printCommand.append(fileName);
                printCommand.append("' \n");
                printCommand.append(positionCommand);
                printCommand.append("#      Status= ");
                printCommand.append(" File not found or size = ");
                printCommand.append(size);
                printCommand.append(".");
                vec.emplace_back(printCommand);
            }
            if (itemValues.fileName_Build.length() > 0) {
                do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
            }
        }
    }
    else {
        vec.clear();
        printCommand = "# Search File: '";
        printCommand.append(fileName);
        printCommand.append("' \n");
        printCommand.append(positionCommand);
        printCommand.append("#      Status= ");
        printCommand.append(" is not a valid file name: not found.");
        vec.emplace_back(printCommand);
        std::cout << "File: " << fileName << " is not a valid file name: not found" << std::endl;
    }
    return result;
}


int removeDirectory(sstr& fileName, sstr& path, std::vector<sstr>& vec)
{
    if (path.length() > 2)
    {
        vec.emplace_back("eval \"rm -rf '" + path + "'\"");
    }
    return 0;
}

int install_apt_required_dependencies()
{
    std::cout << "Not Implemented Yet" << std::endl;
    return 1;
}

int install_mac_required_dependencies(Mac_PM mpm)
{
    int result = 0;
    if (mpm == Mac_PM ::Home_Brew) {
        //install_home_brew_required_dependencies();
        std::cout << "Not Implemented Yet" << std::endl;
        result = 1;
    }
    if (mpm == Mac_PM ::MacPorts) {
        //install_macPort_required_dependencies();
        std::cout << "Not Implemented Yet" << std::endl;
        result = 1;
    }
    return result;
}

int install_yum_required_dependencies(sstr& fileName, sstr& programName, bool createScriptOnly)
{
    sstr fedora_release = "8";
    sstr command;
    std::vector<sstr> vec;
    vec.emplace_back("# Install " + programName + ".");
    vec.emplace_back("yum -y install wget");
    vec.emplace_back("wget https://dl.fedoraproject.org/pub/epel/epel-release-latest-" + fedora_release + ".noarch.rpm");
    vec.emplace_back("yum -y install epel-release-latest-" + fedora_release +".noarch.rpm");
    vec.emplace_back("rm -f epel-release-latest-" + fedora_release +".noarch.rpm");
    vec.emplace_back("yum -y install autoconf");
    vec.emplace_back("yum -y install automake");
    vec.emplace_back("yum -y install bison");
    vec.emplace_back("yum -y install boost-devel");
    vec.emplace_back("yum -y install bzip2-devel");
    vec.emplace_back("yum -y install clang");
    vec.emplace_back("yum -y install cmake");
    vec.emplace_back("yum -y install cmake-gui");
    vec.emplace_back("yum -y install expat-devel");
    vec.emplace_back("yum -y install ftp");
    vec.emplace_back("yum -y install gawk");
    vec.emplace_back("yum -y install google-chrome-stable");
    vec.emplace_back("yum -y install git");
    vec.emplace_back("yum -y install gitk");
    vec.emplace_back("yum -y install gcc");
    vec.emplace_back("yum -y install gcc-c++");
    vec.emplace_back("yum -y install gcc-c++-x86_64-linux-gnu");
    vec.emplace_back("yum -y install gnutls-c++");
    vec.emplace_back("yum -y install gnutls-devel");
    vec.emplace_back("yum -y install innotop");
    vec.emplace_back("yum -y install iotop");
    vec.emplace_back("yum -y install jemalloc-devel");
    vec.emplace_back("yum -y install java-1.8.0-openjdk");
    vec.emplace_back("yum -y install Judy");
    vec.emplace_back("yum -y install libcurl-devel");
    vec.emplace_back("yum -y install libedit-devel");
    vec.emplace_back("yum -y install libffi-devel");
    vec.emplace_back("yum -y install libicu-devel");
    vec.emplace_back("yum -y install libjpeg-turbo-utils");
    vec.emplace_back("yum -y install libjpeg-turbo-devel");
    vec.emplace_back("yum -y install libpng-devel");
    vec.emplace_back("yum -y install libstdc++");
    vec.emplace_back("yum -y install libstdc++-devel");
    vec.emplace_back("yum -y install libstdc++-docs");
    vec.emplace_back("yum -y install libstdc++-static");
    vec.emplace_back("yum -y install libwebp-devel");
    vec.emplace_back("yum -y install libxml2-devel");
    vec.emplace_back("yum -y install libxslt-devel");
    vec.emplace_back("yum -y install libX11-devel");
    vec.emplace_back("yum -y install moreutils");
    vec.emplace_back("yum -y install mysqlreport");
    vec.emplace_back("yum -y install mysqltuner");
    vec.emplace_back("yum -y install mytop");
    vec.emplace_back("yum -y install ncurses-devel");
    vec.emplace_back("yum -y install openssl-devel");
    vec.emplace_back("yum -y install perl-CPAN");
    vec.emplace_back("yum -y install java-1.8.0-openjdk");
    vec.emplace_back("yum -y install re2c");
    vec.emplace_back("yum -y install sqlite-devel");
    vec.emplace_back("yum -y install sqlite-tcl");
    vec.emplace_back("yum -y install tcltls-devel");
    vec.emplace_back("yum -y install xml2");
    vec.emplace_back("yum -y xorg-x11-fonts*");
    vec.emplace_back("yum -y xorg-x11-server-Xnest");
    vec.emplace_back("yum -y install vsqlite++-devel");
    vec.emplace_back("yum -y install yum-utils");
    int result = do_command(fileName, vec, createScriptOnly);
    return result;
}


int install_apt_required_dependencies(sstr& fileName, sstr& programName, bool createScriptOnly)
{
    sstr command;
    std::vector<sstr> vec;
    vec.emplace_back("# Install " + programName + ".");
    vec.emplace_back("apt update");
    vec.emplace_back("apt upgrade -y");
    vec.emplace_back("apt install openjdk-9-jdk -y");
    vec.emplace_back("apt autoremove -y");
    vec.emplace_back("apt install autoconf -y");
    vec.emplace_back("apt install build-essential -y");
    vec.emplace_back("apt install bison -y");
    vec.emplace_back("apt install bzip2 -y");
    vec.emplace_back("apt install clang -y");
    vec.emplace_back("apt install cmake -y");
    vec.emplace_back("apt install cmake-gui -y");
    vec.emplace_back("apt install ftp -y");
    vec.emplace_back("apt install google-chrome-stable -y");
    vec.emplace_back("apt install git -y");
    vec.emplace_back("apt install gitk -y");
    vec.emplace_back("apt install git-cola -y");
    vec.emplace_back("apt install gcc -y");
    vec.emplace_back("apt install gnutls-dev -y");
    vec.emplace_back("apt install g++ -y");
    vec.emplace_back("apt install libboost-dev -y");
    vec.emplace_back("apt install libcurl4-gnutls-dev -y");
    vec.emplace_back("apt install libncurses5-dev -y");
    vec.emplace_back("apt install libedit-dev -y");
    vec.emplace_back("apt install libexpat-dev -y");
    vec.emplace_back("apt install libffi-dev -y");
    vec.emplace_back("apt install libicu-dev -y");
    vec.emplace_back("apt install libjemalloc-dev -y");
    vec.emplace_back("apt install libjpeg-dev -y");
    vec.emplace_back("apt install libjudy-dev -y");
    vec.emplace_back("apt install libncurses5-dev -y");
    vec.emplace_back("apt install libnghttp2");
    vec.emplace_back("apt install libpng-dev -y");
    vec.emplace_back("apt install libsqlite3-tcl -y");
    vec.emplace_back("apt install libssl-dev -y");
    vec.emplace_back("apt install libstdc++-5-dev -y");
    vec.emplace_back("apt install libstdc++-5-doc -y");
    vec.emplace_back("apt install libwebp-dev -y");
    vec.emplace_back("apt install libxml2-dev -y");
    vec.emplace_back("apt install libxslt-dev -y");
    vec.emplace_back("apt install libx11-dev -y");
    vec.emplace_back("apt install openssl-dev -y");
    vec.emplace_back("apt install qt4-qmake -y");
    vec.emplace_back("apt install x11-common -y");
    vec.emplace_back("apt install x11-xserver-utils  -y");
    vec.emplace_back("apt install x11-utils -y");
    vec.emplace_back("apt install re2c -y");
    vec.emplace_back("apt install sqlite3 -y");
    vec.emplace_back("apt install xml2 -y");
    vec.emplace_back("apt install wget -y");
    vec.emplace_back("apt install xxdiff -y");
    vec.emplace_back("apt update");
    vec.emplace_back("apt upgrade -y");
    int result = do_command(fileName, vec, createScriptOnly);
    return result;
}


//
// Install helper functions
//

bool getBoolFromString(sstr& some_value)
{
    bool result = false;
    sstr temp1;
    int temp2;
    for (auto idx = 0ul; idx < some_value.length(); ++idx)
    {
        if (std::isupper(some_value.at(idx)))
        {
            temp2 =  std::tolower(some_value.at(idx));
        }
        else
        {
            temp2 = some_value.at(idx);
        }
        temp1.append({static_cast<char>(temp2)});
    }
    if (temp1 == "t")    result = true;
    if (temp1 == "true") result = true;
    if (temp1 == "on")   result = true;
    return result;
}

sstr get_ProtectionFileWithPath(an_itemValues& itemValues)
{
    sstr protectionFileWithPath = itemValues.srcPath;
    protectionFileWithPath = joinPathParts(protectionFileWithPath, itemValues.programNameVersion);
    protectionFileWithPath.append("protection-");
    protectionFileWithPath.append(itemValues.programName);
    protectionFileWithPath.append(".txt");
    return protectionFileWithPath;
}

void protectProgram_IfRequired(an_itemValues& itemValues, bool show)
{
    std::vector<sstr> vec;
    sstr positionCommand = std::string(commandPosition, ' ');
    sstr command = "";
    sstr protectionFileWithPath = get_ProtectionFileWithPath(itemValues);
    if (!(isFileSizeGtZero(itemValues, itemValues.fileName_Protection, show))) {
        if (itemValues.bProtectMode) {
            vec.emplace_back("#");
            command = "# Enable protection for ";
            command.append(itemValues.ProperName);
            vec.emplace_back(command);
            command.clear();
            command = "eval \"echo 'protection = true' \\\n";
            command.append(positionCommand);
            command.append(" > '");
            command.append(protectionFileWithPath);
            command.append("'\"");
            vec.emplace_back(command);
            do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
        }
    }
    return;
}

void howToRemoveFileProtection(an_itemValues& itemValues)
{
    sstr positionCommand = std::string(commandPosition, ' ');
    sstr protectionFileWithPath = get_ProtectionFileWithPath(itemValues);
    std::vector<sstr> vec;
    vec.emplace_back("# ");
    vec.emplace_back("# This " + itemValues.ProperName + " install is currently protected:");
    vec.emplace_back("# To remove this protection, run this command:");
    vec.emplace_back("# eval \" rm -f '" + protectionFileWithPath + "' \"");
    vec.emplace_back("# ");
    do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    return;
}

bool stageSourceCodeIfNeeded(an_itemValues& itemValues)
{
    bool result = false;
    bool special = false;
    std::vector<sstr> vec;
    itemValues.getPath.append(itemValues.fileName_Compressed);

    vec.emplace_back("#");
    vec.emplace_back("# Stage source file if needed.");
    do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    vec.clear();
    if (!(isFileSizeGtZero(itemValues, itemValues.fileName_Staged, true)))
    {
        vec.emplace_back("# Attempting to download file...");
        if (itemValues.programName == "php")
        {
            vec.emplace_back("eval \"cd '" + itemValues.stgPath + "'; wget " + itemValues.getPath + "/from/this/mirror \"");
            special = true;
        }
        if (itemValues.programName == "mariadb")
        {
            vec.emplace_back("eval \"cd '" + itemValues.stgPath + "'; wget " + itemValues.getPath + "/from/http%3A//ftp.kaist.ac.kr/mariadb \"");
            special = true;
        }
        if (!special)
        {
            vec.emplace_back("eval \"cd '" + itemValues.stgPath + "'; wget " + itemValues.getPath + "\"");
        }

    }
    else
    {
        vec.emplace_back("# Source code appears to be staged.");
        result = true;
    }
    do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    return result;
}

sstr get_xxx_Path(const sstr& xxxPath, const sstr& replacement)
{
    sstr result = "";
    if (replacement.length() == 3) {
        sstr newPath = xxxPath;
        auto start = newPath.rfind("xxx");
        if (start != -1) {
            newPath.replace(start, 3, replacement, 0, 3);
        }
        else
        {
            // this will most likely cause the program
            //   to not do what is desired, but the
            //   rest of the program will handle the
            //   failure instead of crashing the program.
            //   And it won't cause deleting the entire partition.
            newPath = "Err in get_xxx_Path()...xxx was not found.";
        }
        result = newPath;
    }
    else {
        result = "Err in get_xxx_Path()...Replacement path length is != 3.";
    }
    return result;
}

int removeWorkingDirectories(an_itemValues& itemValues)
{
    std::vector<sstr> vec;
    removeDirectory(itemValues.fileName_Build, itemValues.bldPath, vec);
    removeDirectory(itemValues.fileName_Build, itemValues.etcPath, vec);
    removeDirectory(itemValues.fileName_Build, itemValues.srcPath, vec);

    if (itemValues.ProperName != "Tk") {
        removeDirectory(itemValues.fileName_Build, itemValues.usrPath, vec);
    }
    int result = do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    if (result == 0)
    {
        vec.clear();
        vec.emplace_back("# Directories were deleted successfully.");
        do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    }
    else
    {
        vec.clear();
        vec.emplace_back("# Directories were not deleted successfully.");
        vec.emplace_back("# Check for valid path and permissions.");
        do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    }
    return result;
}

int ensureWrkDirExist(an_itemValues& itemValues)
{
    int result = -1;
    std::vector<sstr> vec;
    vec.emplace_back("# ");
    vec.emplace_back("# Ensure " + itemValues.ProperName + " working directories exist. ");
    vec.emplace_back("eval \"mkdir -p '" + itemValues.bldPath + "'\"");
    vec.emplace_back("eval \"mkdir -p '" + itemValues.etcPath + "'\"");
    vec.emplace_back("eval \"mkdir -p '" + itemValues.srcPath + "'\"");
    if (itemValues.programName == "tk") {
        vec.emplace_back("#already exists '" + itemValues.usrPath + "'\"");
    }
    else
    {
        vec.emplace_back("eval \"mkdir -p '" + itemValues.usrPath + "'\"");
    }
    result = do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    if (result == 0)
    {
        vec.clear();
        vec.emplace_back("# Directories exist or were created.");
        do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    }
    else
    {
        vec.clear();
        vec.emplace_back("# Directories were not created successfully.");
        vec.emplace_back("# Check for valid path and permissions.");
        do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    }
    return result;
}

int createTargetFromStage(an_itemValues& itemValues)
{
    std::vector<sstr> vec;
    vec.emplace_back("# ");
    vec.emplace_back("# Copy, decompress, and remove compressed file.");
    vec.emplace_back("eval \"cd '" + itemValues.stgPath + "';        cp './"   + itemValues.fileName_Compressed + "' '" + itemValues.srcPath + "'\"");
    vec.emplace_back("eval \"cd '" + itemValues.srcPath + "'; tar xf '" + itemValues.fileName_Compressed + "'\"");
    vec.emplace_back("eval \"cd '" + itemValues.srcPath + "'; rm  -f '" + itemValues.fileName_Compressed + "'\"");
    int result = do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    if (result == 0)
    {
        vec.clear();
        vec.emplace_back("# Source code positioned for compiliation.");
        do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    }
    else
    {
        vec.clear();
        vec.emplace_back("# Source code NOT positioned for compiliation.");
        vec.emplace_back("# Check for valid path and permissions.");
        do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    }
    return result;
}

int ensureStageDirectoryExists(an_itemValues &itemValues)
{
    std::vector<sstr> vec;
    vec.emplace_back("# ");
    vec.emplace_back("# Ensure stage directory exists.");
    vec.emplace_back("eval \"mkdir -p '" + itemValues.stgPath + "'\"");
    int result = do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    if (result == 0)
    {
        vec.clear();
        vec.emplace_back("# Stage directory exists.");
    }
    else
    {
        vec.clear();
        vec.emplace_back("# Stage directory does NOT exists.");
        vec.emplace_back("# Check for valid path, and permissions.");
    }
    do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    return result;
}

int setupInstallDirectories(an_itemValues& itemValues)
{
    std::vector<sstr> vec;
    vec.emplace_back("# ");
    vec.emplace_back("# Remove unfinished " + itemValues.ProperName + " install (if any).");
    int result = do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);

    if ((itemValues.ProperName == "Tcl") || (itemValues.ProperName == "Tk"))
    {
        sstr tmpPath = "usr/Tcl_Tk";
        itemValues.usrPath = joinPathParts(itemValues.rtnPath, tmpPath);
    }
    result += removeWorkingDirectories(itemValues);
    result += ensureWrkDirExist       (itemValues);
    result += createTargetFromStage   (itemValues);
    return result;
}

int ensure_GroupExists(an_itemValues itemValues, sstr groupName)
{
    int result = 0;
    std::vector<sstr> vec;
    sstr groupFileName = "/etc/group";
    sstr groupTestFile = itemValues.srcPath + groupName + "-group-test.txt";

    // So to find out if a user exists already we are going to do a
    // little hacking... cat /etc/group | grep groupName will give us one line
    // if the groupName exists. So we pipe the results to a create/truncate
    // file pipe, then we check to see if the file size is greater than 0.

    // Group must exist before adding the user to the group
    // ...so we must do the group first
    vec.clear();
    vec.emplace_back("cat " + groupFileName + "  | grep " + groupName + " > '" + groupTestFile + "'");
    do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    bool GroupExists = isFileSizeGtZero(itemValues, groupTestFile, false);
    vec.clear();
    if (!GroupExists) {
        vec.emplace_back("# Adding " + groupName + " group");
        vec.emplace_back("groupadd " + groupName);
    } else {
        vec.emplace_back("# " + groupName + " group already exists.");
    }
    result += do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    // we should remove the groupTestFile now...
    vec.clear();
    vec.emplace_back("rm -f '" + groupTestFile + "'");
    do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    return result;
}

int ensure_UserExists(an_itemValues itemValues, sstr groupName, sstr userName)
{
    int result = 0;
    std::vector<sstr> vec;
    sstr userFileName = "/etc/passwd";
    sstr user_TestFile = itemValues.srcPath + "users.txt";

    // So to find out if a user exists already we are going to do a
    // little hacking... cat /etc/passwd | grep apache_ws will give us one line
    // if the user exists. So we pipe the results to a create/truncate
    // file pipe, then we check to see if the file size is greater than 0.

    vec.clear();
    vec.emplace_back("cat " + userFileName + " | grep  " + userName + "  > '" + user_TestFile + "'" );
    do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    bool userExists = isFileSizeGtZero(itemValues, user_TestFile, false);
    vec.clear();
    if (!userExists)
    {
        vec.emplace_back("# Adding " + userName + " user");
        vec.emplace_back("useradd  -g " + groupName + " --no-create-home --system " + userName);
    }
    else
    {
        vec.emplace_back("# " + userName + " user already exists.");
    }
    result += do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    // we should remove the user_TestFile now...
    vec.clear();
    vec.emplace_back("rm -f '" + user_TestFile + "'");
    do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    return result;
}

int ensureApacheUserAndGroupExists(an_itemValues& itemValues)
{
    int result = 0;
    std::vector<sstr> vec;
    vec.emplace_back("#");
    vec.emplace_back("# Check for required User and Group");
    do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);

    result += ensure_GroupExists(itemValues, APACHE_GROUP);
    result += ensure_UserExists( itemValues, APACHE_GROUP, APACHE_USER);
    return result;
}

int ensureMariaDB_UserAndGroupExist(an_itemValues& itemValues)
{
    int result = 0;
    std::vector<sstr> vec;
    vec.emplace_back("#");
    vec.emplace_back("# Check for required User and Group");
    do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);

    result += ensure_GroupExists(itemValues, MARIADB_GROUP);
    result += ensure_UserExists( itemValues, MARIADB_GROUP, MARIADB_USER);
    return result;
}


int create_apache_conf_File(an_itemValues &itemValues)
{
    std::vector<sstr> vec;
    sstr positionCommand = std::string(commandPosition, ' ');
    vec.clear();
    vec.emplace_back("#Save the original apache.conf file if any, to have as a guide.");

    //lets add the date_time to the my.cnf.old in case we run this multiple times
    //  in a day we will still have the original file somewhere.

    sstr theDate = make_fileName_dateTime(0);
    vec.emplace_back("eval \"cd /etc/httpd/conf/; cp httpd.conf \\\n"
        + positionCommand  + "  '" +  itemValues.etcPath + "extra/rpm_based_httpd.conf.old_" + theDate + "' \"");
    do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);

    //Now we have to create the new my.cnf file
    vec.clear();
    vec.emplace_back("#Create new file " + itemValues.etcPath + "apache.conf ");
    vec.emplace_back("eval \"cd /etc; echo ' ' >" + itemValues.etcPath + "apache.conf \"");
    do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);

    sstr apache_conf_File = itemValues.etcPath + "apache.conf";
    //Now we add the content to the /etc/my.cnf file
    vec.clear();
    vec.emplace_back("#");
    vec.emplace_back("# This is the main Apache HTTP server configuration file.");
    vec.emplace_back("# See <URL:http://httpd.apache.org/docs/2.4/> for detailed information.");
    vec.emplace_back("#");
    vec.emplace_back("# If you are unsure consult the online docs. ");
    vec.emplace_back("# This default configuration does not give adequate protection, You have been warned.");
    vec.emplace_back("# Additional setup is required by the system administrator");
    vec.emplace_back("");
    vec.emplace_back("ServerRoot " + itemValues.usrPath);
    vec.emplace_back("Listen 80");
    vec.emplace_back("");
    vec.emplace_back("# To check the syntax of this file run the following commands");
    vec.emplace_back("# cd " + itemValues.usrPath + "bin;");
    vec.emplace_back("# ./apachectl -f " + itemValues.etcPath + "apache.conf -t");
    vec.emplace_back("");
    vec.emplace_back("# To check the syntax of this virtual hosts file(s) run the following commands");
    vec.emplace_back("# cd " + itemValues.usrPath + "bin;");
    vec.emplace_back("# ./apachectl -f " + itemValues.etcPath + "apache.conf -S");
    vec.emplace_back("");
    vec.emplace_back("# To check the version of Apache Web server run the following commands");
    vec.emplace_back("# cd " + itemValues.usrPath + "bin;");
    vec.emplace_back("# ./apachectl -f " + itemValues.etcPath + "apache.conf -v");
    vec.emplace_back("");
    vec.emplace_back("# To start/restart Apache Web server run the following commands");
    vec.emplace_back("# cd " + itemValues.usrPath + "bin;");
    vec.emplace_back("# ./apachectl -f " + itemValues.etcPath + "apache.conf -k restart");
    vec.emplace_back("");
    vec.emplace_back("# To stop Apache Web server run the following commands");
    vec.emplace_back("# cd " + itemValues.usrPath + "bin;");
    vec.emplace_back("# ./apachectl -f " + itemValues.etcPath + "apache.conf -k graceful-stop");
    vec.emplace_back("#");
    vec.emplace_back("# Where possible the LoadModules have been put in alphabetical order.");
    vec.emplace_back("# But, order does matter in some cases, so if you change the order run:");
    vec.emplace_back("# cd " + itemValues.usrPath + "bin;");
    vec.emplace_back("# ./apachectl -f " + itemValues.etcPath + "apache.conf -k graceful-stop");
    vec.emplace_back("# ./apachectl -f " + itemValues.etcPath + "apache.conf -t");
    vec.emplace_back("# ./apachectl -f " + itemValues.etcPath + "apache.conf -k restart");
    vec.emplace_back("#  Then check the error log to see if there are any module related errors.");
    vec.emplace_back("#");
    vec.emplace_back("# Dynamic Shared Object (DSO) Support");
    vec.emplace_back("#");
    vec.emplace_back("LoadModule access_compat_module modules/mod_access_compat.so");
    vec.emplace_back("LoadModule alias_module modules/mod_alias.so");
    vec.emplace_back("LoadModule request_module modules/mod_request.so");
    vec.emplace_back("LoadModule auth_form_module modules/mod_auth_form.so");
    vec.emplace_back("LoadModule autoindex_module modules/mod_autoindex.so");
    vec.emplace_back("LoadModule authn_file_module modules/mod_authn_file.so");
    vec.emplace_back("#");
    vec.emplace_back("LoadModule authn_core_module modules/mod_authn_core.so");
    vec.emplace_back("LoadModule authz_host_module modules/mod_authz_host.so");
    vec.emplace_back("LoadModule authz_groupfile_module modules/mod_authz_groupfile.so");
    vec.emplace_back("LoadModule authz_user_module modules/mod_authz_user.so");
    vec.emplace_back("LoadModule authz_core_module modules/mod_authz_core.so");
    vec.emplace_back("LoadModule auth_basic_module modules/mod_auth_basic.so");
    vec.emplace_back("#");
    vec.emplace_back("LoadModule cache_module modules/mod_cache.so");
    vec.emplace_back("LoadModule cache_disk_module modules/mod_cache_disk.so");
    vec.emplace_back("LoadModule cache_socache_module modules/mod_cache_socache.so");
    vec.emplace_back("LoadModule dir_module modules/mod_dir.so");
    vec.emplace_back("LoadModule env_module modules/mod_env.so");
    vec.emplace_back("LoadModule file_cache_module modules/mod_file_cache.so");
    vec.emplace_back("#");
    vec.emplace_back("LoadModule filter_module modules/mod_filter.so");
    vec.emplace_back("LoadModule headers_module modules/mod_headers.so");
    vec.emplace_back("LoadModule include_module modules/mod_include.so");
    vec.emplace_back("LoadModule info_module modules/mod_info.so");
    vec.emplace_back("LoadModule lbmethod_heartbeat_module modules/mod_lbmethod_heartbeat.so");
    vec.emplace_back("LoadModule log_config_module modules/mod_log_config.so");
    vec.emplace_back("#");
    vec.emplace_back("LoadModule mime_module modules/mod_mime.so");
    vec.emplace_back("LoadModule negotiation_module modules/mod_negotiation.so");
    vec.emplace_back("LoadModule php7_module modules/mod_php7.so");
    vec.emplace_back("LoadModule reqtimeout_module modules/mod_reqtimeout.so");
    vec.emplace_back("LoadModule session_module modules/mod_session.so");
    vec.emplace_back("#");
    vec.emplace_back("LoadModule session_cookie_module modules/mod_session_cookie.so");
    vec.emplace_back("LoadModule setenvif_module modules/mod_setenvif.so");
    vec.emplace_back("LoadModule socache_shmcb_module modules/mod_socache_shmcb.so");
    vec.emplace_back("LoadModule socache_dbm_module modules/mod_socache_dbm.so");
    vec.emplace_back("LoadModule socache_memcache_module modules/mod_socache_memcache.so");
    vec.emplace_back("LoadModule slotmem_shm_module modules/mod_slotmem_shm.so");
    vec.emplace_back("#");
    vec.emplace_back("LoadModule ssl_module modules/mod_ssl.so");
    vec.emplace_back("LoadModule status_module modules/mod_status.so");
    vec.emplace_back("LoadModule substitute_module modules/mod_substitute.so");
    vec.emplace_back("LoadModule version_module modules/mod_version.so");
    vec.emplace_back("LoadModule unixd_module modules/mod_unixd.so");
    vec.emplace_back("LoadModule vhost_alias_module modules/mod_vhost_alias.so");
    vec.emplace_back("#");
    vec.emplace_back("# Unused Modules removed...get newly required from the original file...");
    vec.emplace_back("#");
    vec.emplace_back("  <IfModule unixd_module>");
    vec.emplace_back("    User  apache_ws");
    vec.emplace_back("    Group apache_ws");
    vec.emplace_back("  </IfModule>");
    vec.emplace_back("#");
    vec.emplace_back("  <Directory />");
    vec.emplace_back("    AllowOverride none");
    vec.emplace_back("    Require all denied");
    vec.emplace_back("  </Directory>");
    vec.emplace_back("#");
    vec.emplace_back("ServerAdmin yourAdminEmail@somewhere.com");
    vec.emplace_back("ServerName  127.0.1.1");
    vec.emplace_back("");
    vec.emplace_back("DocumentRoot " + itemValues.usrPath  + "htdocs");
    vec.emplace_back("<Directory   " + itemValues.usrPath  + "htdocs>");
    vec.emplace_back("");
    vec.emplace_back("  <RequireAny>");
    vec.emplace_back("    Require ip 10.0");
    vec.emplace_back("    Require ip 192.168");
    vec.emplace_back("    Require ip ::1");
    vec.emplace_back("</RequireAny>");
    vec.emplace_back("");
    vec.emplace_back("  Require all Granted");
    vec.emplace_back("");
    vec.emplace_back("  AddType application/x-httpd-php .php");
    vec.emplace_back("");
    vec.emplace_back("  <IfModule php7_module>");
    vec.emplace_back("    php_flag magic_quotes_gpc Off");
    vec.emplace_back("    php_flag short_open_tag Off");
    vec.emplace_back("    php_flag register_globals Off");
    vec.emplace_back("    php_flag register_argc_argv On");
    vec.emplace_back("    php_flag track_vars On");
    vec.emplace_back("    # this setting is necessary for some locales");
    vec.emplace_back("    php_value mbstring.func_overload 0");
    vec.emplace_back("    php_value include_path .");
    vec.emplace_back("  </IfModule>");
    vec.emplace_back("");
    vec.emplace_back("</Directory>");
    vec.emplace_back("");
    vec.emplace_back("<IfModule dir_module>");
    vec.emplace_back("  DirectoryIndex index.php index.html");
    vec.emplace_back("</IfModule>");
    vec.emplace_back("#");
    vec.emplace_back("#");
    vec.emplace_back("<Files \".ht*\">");
    vec.emplace_back("  Require all denied");
    vec.emplace_back("</Files>");
    vec.emplace_back("#");
    vec.emplace_back("ErrorLog \"logs/error_log\"");
    vec.emplace_back("LogLevel trace6");
    vec.emplace_back("#");
    vec.emplace_back("<IfModule log_config_module>");
    vec.emplace_back("  LogFormat \"%h %l %u %t \\\"%r\\\" %>s %b \\\"%{Referer}i\\\" \\\"%{User-Agent}i\\\" combined");
    vec.emplace_back("  LogFormat \"%h %l %u %t \\\"%r\\\" %>s %b\" common");
    vec.emplace_back("<IfModule logio_module>");
    vec.emplace_back("  # You need to enable mod_logio.c to use %I and %O");
    vec.emplace_back("    LogFormat \"%h %l %u %t \\\"%r\\\" %>s %b \\\"%{Referer}i\\\" \\\"%{User-Agent}i\\\" %I %O\" combinedio");
    vec.emplace_back("</IfModule>");
    vec.emplace_back("  CustomLog \"logs/access_log\" common");
    vec.emplace_back("</IfModule>");
    vec.emplace_back("#");
    vec.emplace_back("#");
    vec.emplace_back("# Customizable error responses come in three flavors:");
    vec.emplace_back("# 1) plain text 2) local redirects 3) external redirects");
    vec.emplace_back("#");
    vec.emplace_back("# Some examples:");
    vec.emplace_back("#ErrorDocument 500 \"Server Fault.\"");
    vec.emplace_back("#ErrorDocument 404 /missing.html");
    vec.emplace_back("#ErrorDocument 404 \"/cgi-bin/missing_handler.pl\"");
    vec.emplace_back("#ErrorDocument 402 http://www.example.com/subscription_info.html");
    vec.emplace_back("#");
    vec.emplace_back("#");
    vec.emplace_back("EnableMMAP off");
    vec.emplace_back("EnableSendfile on");
    vec.emplace_back("#");
    vec.emplace_back("<IfModule ssl_module>");
    vec.emplace_back("  Listen 443");
    vec.emplace_back("  SSLRandomSeed startup builtin");
    vec.emplace_back("  SSLRandomSeed connect builtin");
    vec.emplace_back("  SSLSessionCache  shmcb:" + itemValues.rtnPath +"tls/ssl_scache(512000)");
    vec.emplace_back("</IfModule>");
    vec.emplace_back("");
    vec.emplace_back("# Server-pool management (MPM specific)");
    vec.emplace_back("  Include " + itemValues.etcPath + "extra/httpd-mpm.conf");
    vec.emplace_back("");
    vec.emplace_back("# Multi-language error messages");
    vec.emplace_back("  Include " + itemValues.etcPath + "extra/httpd-multilang-errordoc.conf");
    vec.emplace_back("");
    vec.emplace_back("# Fancy directory listings");
    vec.emplace_back("  Include " + itemValues.etcPath + "extra/httpd-autoindex.conf");
    vec.emplace_back("");
    vec.emplace_back("# Language settings");
    vec.emplace_back("  Include " + itemValues.etcPath + "extra/httpd-languages.conf");
    vec.emplace_back("");
    vec.emplace_back("# Real-time info on requests and configuration");
    vec.emplace_back("  Include " + itemValues.etcPath + "extra/httpd-info.conf");
    vec.emplace_back("");
    vec.emplace_back("# Virtual hosts");
    vec.emplace_back("#  Include " + itemValues.etcPath + "sites-enabled/site-001-dev.conf");
    vec.emplace_back("#  Include " + itemValues.etcPath + "sites-enabled/site-001-tst.conf");
    vec.emplace_back("#  Include " + itemValues.etcPath + "sites-enabled/site-001-prd.conf");
    vec.emplace_back("");
    vec.emplace_back("# Local access to the Apache HTTP Server Manual");
    vec.emplace_back("  Include " + itemValues.etcPath + "extra/httpd-manual.conf");
    vec.emplace_back("");
    vec.emplace_back("## Unused Extras removed...get required from the original file...");
    vec.emplace_back("");
    vec.emplace_back("## end of configuration file");
    vec.emplace_back("");
    file_write_vector_to_file(apache_conf_File, vec, false);
    return 0;
};

int apache_notes(an_itemValues& itemValues)
{
    int result = 0;
    std::vector<sstr> vec;
    if (itemValues.ProperName == "Apache") {

        vec.clear();
        vec.emplace_back("#################################################################################");
        vec.emplace_back("# ");
        vec.emplace_back("# Apache Note Section");
        vec.emplace_back("# ");
        vec.emplace_back("#################################################################################");
        vec.emplace_back("# ");
        vec.emplace_back("#--> Commands to create the apache group and user.");
        vec.emplace_back("#-->   Should have been completed with a succesful install.");
        vec.emplace_back("groupadd   apache_ws ");
        vec.emplace_back("useradd -g apache_ws apache_ws ");
        file_write_vector_to_file(itemValues.fileName_Notes, vec, false);

        //Ensure firewalld starts on boot and more
        vec.clear();
        vec.emplace_back("# ");
        vec.emplace_back("#You can only use iptables or firewalld");
        vec.emplace_back("#We are choosing to only use firewalld");
        vec.emplace_back("#Ensure iptables will not start on boot");
        vec.emplace_back("systemctl mask   iptables");
        vec.emplace_back("#Ensure firewalld starts on boot");
        vec.emplace_back("systemctl enable firewalld");
        vec.emplace_back("#It could be a good idea to reboot your system now.");
        vec.emplace_back("#Ensure firewalld is started now");
        vec.emplace_back("systemctl status -l  firewalld");
        vec.emplace_back("#If your filewall is not running use this command.");
        vec.emplace_back("systemctl start  firewalld");
        vec.emplace_back("firewall-cmd             --list-services");
        vec.emplace_back("firewall-cmd --permanent --list-services");
        vec.emplace_back("firewall-cmd             --list-ports");
        vec.emplace_back("firewall-cmd --permanent --list-ports");
        vec.emplace_back("#");
        vec.emplace_back("#After making temp changes to reset firewall back to permanent rules");
        vec.emplace_back("systemctl reload firewalld");
        vec.emplace_back("# ");
        file_write_vector_to_file(itemValues.fileName_Notes, vec, false);

        //Remove http and https from firewall before website is ready
        vec.clear();
        vec.emplace_back("# ");
        vec.emplace_back("#Remove http and https from firewall before website is ready");
        vec.emplace_back("firewall-cmd             --list-services");
        vec.emplace_back("firewall-cmd --permanent --list-services");
        vec.emplace_back("firewall-cmd             --remove-service=http");
        vec.emplace_back("firewall-cmd --permanent --remove-service=http");
        vec.emplace_back("firewall-cmd             --remove-service=https");
        vec.emplace_back("firewall-cmd --permanent --remove-service=https");
        vec.emplace_back("firewall-cmd             --list-services");
        vec.emplace_back("firewall-cmd --permanent --list-services");
        vec.emplace_back("systemctl reload firewalld");
        vec.emplace_back("# ");
        file_write_vector_to_file(itemValues.fileName_Notes, vec, false);

        //Remove http port and https port from firewall when website is not ready
        vec.clear();
        vec.emplace_back("# ");
        vec.emplace_back("#Remove http and https from firewall before website is ready");
        vec.emplace_back("firewall-cmd             --list-ports");
        vec.emplace_back("firewall-cmd --permanent --list-ports");
        vec.emplace_back("firewall-cmd             --remove-port=80/tcp");
        vec.emplace_back("firewall-cmd --permanent --remove-port=80/tcp");
        vec.emplace_back("firewall-cmd             --remove-port=443/tcp");
        vec.emplace_back("firewall-cmd --permanent --remove-port=443/tcp");
        vec.emplace_back("firewall-cmd             --list-ports");
        vec.emplace_back("firewall-cmd --permanent --list-ports");
        vec.emplace_back("systemctl reload firewalld");
        vec.emplace_back("# ");
        file_write_vector_to_file(itemValues.fileName_Notes, vec, false);

        //Remove remote access via ssh
        vec.clear();
        vec.emplace_back("# ");
        vec.emplace_back("#Remove remote access if needed for security");
        vec.emplace_back("firewall-cmd             --list-services");
        vec.emplace_back("firewall-cmd --permanent --list-services");
        vec.emplace_back("firewall-cmd             --remove-service=ssh");
        vec.emplace_back("firewall-cmd --permanent --remove-service=ssh");
        vec.emplace_back("firewall-cmd             --list-services");
        vec.emplace_back("firewall-cmd --permanent --list-services");
        vec.emplace_back("systemctl reload firewalld");
        vec.emplace_back("# ");
        file_write_vector_to_file(itemValues.fileName_Notes, vec, false);

        //Remove remote access via ssh
        vec.clear();
        vec.emplace_back("# ");
        vec.emplace_back("#Remove remote access if needed for security");
        vec.emplace_back("firewall-cmd             --list-ports");
        vec.emplace_back("firewall-cmd --permanent --list-ports");
        vec.emplace_back("firewall-cmd             --remove-port=22/tcp");
        vec.emplace_back("firewall-cmd --permanent --remove-port=22/tcp");
        vec.emplace_back("firewall-cmd             --list-ports");
        vec.emplace_back("firewall-cmd --permanent --list-ports");
        vec.emplace_back("systemctl reload firewalld");
        vec.emplace_back("# ");
        file_write_vector_to_file(itemValues.fileName_Notes, vec, false);

        //Add remote access via ssh
        vec.clear();
        vec.emplace_back("# ");
        vec.emplace_back("#Add remote access if needed");
        vec.emplace_back("firewall-cmd             --list-services");
        vec.emplace_back("firewall-cmd --permanent --list-services");
        vec.emplace_back("firewall-cmd             --add-service=ssh");
        vec.emplace_back("firewall-cmd --permanent --add-service=ssh");
        vec.emplace_back("firewall-cmd             --list-services");
        vec.emplace_back("firewall-cmd --permanent --list-services");
        vec.emplace_back("systemctl reload firewalld");
        vec.emplace_back("# ");
        file_write_vector_to_file(itemValues.fileName_Notes, vec, false);

        //Add remote access via ssh
        vec.clear();
        vec.emplace_back("# ");
        vec.emplace_back("#Add remote access if needed for security");
        vec.emplace_back("firewall-cmd             --list-ports");
        vec.emplace_back("firewall-cmd --permanent --list-ports");
        vec.emplace_back("firewall-cmd             --add-port=22/tcp");
        vec.emplace_back("firewall-cmd --permanent --add-port=22/tcp");
        vec.emplace_back("firewall-cmd             --list-ports");
        vec.emplace_back("firewall-cmd --permanent --list-ports");
        vec.emplace_back("systemctl reload firewalld");
        vec.emplace_back("# ");
        file_write_vector_to_file(itemValues.fileName_Notes, vec, false);

        //Add http and https to firewall when website is ready
        vec.clear();
        vec.emplace_back("# ");
        vec.emplace_back("#Add http and https to firewall when website is ready");
        vec.emplace_back("firewall-cmd             --list-services");
        vec.emplace_back("firewall-cmd --permanent --list-services");
        vec.emplace_back("firewall-cmd             --add-service=http");
        vec.emplace_back("firewall-cmd --permanent --add-service=http");
        vec.emplace_back("firewall-cmd             --add-service=https");
        vec.emplace_back("firewall-cmd --permanent --add-service=https");
        vec.emplace_back("firewall-cmd             --list-services");
        vec.emplace_back("firewall-cmd --permanent --list-services");
        vec.emplace_back("systemctl reload firewalld");
        vec.emplace_back("# ");
        file_write_vector_to_file(itemValues.fileName_Notes, vec, false);

        //Add http port and https port to firewall when website is ready
        vec.clear();
        vec.emplace_back("#Add http and https to firewall when website is ready");
        vec.emplace_back("firewall-cmd             --list-ports");
        vec.emplace_back("firewall-cmd --permanent --list-ports");
        vec.emplace_back("firewall-cmd             --add-port=80/tcp");
        vec.emplace_back("firewall-cmd --permanent --add-port=80/tcp");
        vec.emplace_back("firewall-cmd             --add-port=443/tcp");
        vec.emplace_back("firewall-cmd --permanent --add-port=443/tcp");
        vec.emplace_back("firewall-cmd             --list-services");
        vec.emplace_back("firewall-cmd --permanent --list-services");
        vec.emplace_back("systemctl reload firewalld");
        file_write_vector_to_file(itemValues.fileName_Notes, vec, false);


        //Create required directories if needed
        vec.clear();
        vec.emplace_back("# ");
        vec.emplace_back("mkdir -p " + itemValues.etcPath + "extra");
        vec.emplace_back("mkdir -p " + itemValues.etcPath + "sites-available");
        vec.emplace_back("mkdir -p " + itemValues.etcPath + "sites-enabled");
        do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
        file_write_vector_to_file(itemValues.fileName_Notes, vec, false);

        vec.clear();
        vec.emplace_back("");
        vec.emplace_back("# To check the version of Apache Web server run the following commands");
        vec.emplace_back("cd " + itemValues.usrPath + "bin;");
        vec.emplace_back("./apachectl -f " + itemValues.etcPath + "apache.conf -v");
        vec.emplace_back("");
        vec.emplace_back("# To check the syntax of this file run the following commands");
        vec.emplace_back("cd " + itemValues.usrPath + "bin;");
        vec.emplace_back("./apachectl -f " + itemValues.etcPath + "apache.conf -t");
        vec.emplace_back("");
        vec.emplace_back("# To check the syntax of virtual hosts run the following commands");
        vec.emplace_back("cd " + itemValues.usrPath + "bin;");
        vec.emplace_back("./apachectl -f " + itemValues.etcPath + "apache.conf -S");
        vec.emplace_back("");
        vec.emplace_back("# To start/restart Apache Web server run the following commands");
        vec.emplace_back("cd " + itemValues.usrPath + "bin;");
        vec.emplace_back("./apachectl -f " + itemValues.etcPath + "apache.conf -k restart");
        vec.emplace_back("");
        vec.emplace_back("# To stop Apache Web server run the following commands");
        vec.emplace_back("cd " + itemValues.usrPath + "bin;");
        vec.emplace_back("./apachectl -f " + itemValues.etcPath + "apache.conf -k graceful-stop");
        vec.emplace_back("#");
        vec.emplace_back("# To set permissions run the following commands");
        vec.emplace_back("cd " + itemValues.etcPath + "../;");
        vec.emplace_back("chown -R :apache_ws apache");
        vec.emplace_back("chmod -R 750 apache");
        vec.emplace_back("# To set permissions run the following commands");
        vec.emplace_back("cd " + itemValues.usrPath + "../;");
        vec.emplace_back("chown -R :apache_ws apache");
        vec.emplace_back("chmod -R 750 apache");
        vec.emplace_back("#");

        file_write_vector_to_file(itemValues.fileName_Notes, vec, false);

        create_apache_conf_File(itemValues);

        vec.clear();
        vec.emplace_back("#");
        vec.emplace_back("# See the Installation Notes on how to");
        vec.emplace_back("#   setup and start apache web server.");
        result = do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
        return result;
    }
    return result;
}

int create_mariaDB_cnf_File(an_itemValues &itemValues)
{
    std::vector<sstr> vec;
    vec.clear();
    vec.emplace_back("#Save the original my.cnf file if any, to have as a guide.");

    //lets add the date_time to the my.cnf.old in case we run this multiple times
    //  in a day we will still have the original file somewhere.

    sstr theDate = make_fileName_dateTime(0);
    vec.emplace_back("eval \"cd /etc; cp my.cnf" + itemValues.etcPath + "my.cnf.old_" + theDate + "\"");
    do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);

    //Now we have to create the new my.cnf file
    vec.clear();
    vec.emplace_back("#Create new file " + itemValues.etcPath + "my.cnf ");
    vec.emplace_back("eval \"cd /etc; echo ' ' >" + itemValues.etcPath + "my.cnf \"");
    do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);

    sstr my_cnf_File = itemValues.etcPath + "my.cnf";
    //Now we add the content to the /etc/my.cnf file
    vec.clear();
    vec.emplace_back("[mysqld]");
    vec.emplace_back("user=mysql");
    vec.emplace_back("socket='" + itemValues.usrPath + "run/mariadb.socket'");
    vec.emplace_back("bind-address=127.0.1.2");
    vec.emplace_back("port=3306");
    vec.emplace_back("#skip-external-locking");
    vec.emplace_back(" ");
    vec.emplace_back("#Directories");
    vec.emplace_back("  basedir='" + itemValues.usrPath + "'");
    vec.emplace_back("  datadir='" + itemValues.usrPath + "data/'");
    vec.emplace_back(" ");
    vec.emplace_back("#Log File");
    vec.emplace_back("  log-basename='MariaDB_Logs'");
    vec.emplace_back(" ");
    vec.emplace_back("#Other Stuff");
    vec.emplace_back("skip-ssl");
    vec.emplace_back("key_buffer_size=32M");
    vec.emplace_back(" ");
    vec.emplace_back("# Disabling symbolic-links is recommended to prevent assorted security risks symbolic-links=0");
    vec.emplace_back("# Settings user and group are ignored when systemd is used.");
    vec.emplace_back("# If you need to run mysqld under a different user or group,");
    vec.emplace_back("# customize your systemd unit file for mariadb according to the");
    vec.emplace_back("# instructions in http://fedoraproject.org/wiki/Systemd");
    vec.emplace_back(" ");
    vec.emplace_back("[mysqld_safe]");
    vec.emplace_back("log-error='" + itemValues.usrPath + "var/log/mariadb'");
    vec.emplace_back(" pid-file='" + itemValues.usrPath + "run/mariadb_pid'");
    vec.emplace_back(" ");
    vec.emplace_back("#");
    vec.emplace_back("# include all files from the config directory");
    vec.emplace_back("#");
    vec.emplace_back("# Not using anything here....");
    vec.emplace_back("#   in fact no directory or files here");
    vec.emplace_back("#   !includedir " + itemValues.etcPath + "my.cnf.d");
    vec.emplace_back(" ");
    vec.emplace_back("## end of file");
    vec.emplace_back(" ");
    file_write_vector_to_file(my_cnf_File, vec, false);
    return 0;
};

int configure(an_itemValues& itemValues,  sstr& configureStr)
{
    sstr positionCommand = std::string(commandPosition,' ');
    sstr outFileName = "pre_make_results.txt";
    std::vector<sstr> vec;
    vec.emplace_back("# ");
    vec.emplace_back("# Pre make commands -- usually configure, but not always...");
    vec.emplace_back("# Piping results to '" + itemValues.bldPath + "'.");

    configureStr.append(positionCommand + "> '" + itemValues.bldPath + outFileName + "' 2>&1 ");
    vec.emplace_back(configureStr);
    int result = do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    vec.clear();
    if (result == 0) {
        vec.emplace_back("# Pre make commands completed successfully.");
    } else {
        vec.emplace_back("# Pre make commands had some problems.");
        vec.emplace_back("# Look through '" + itemValues.bldPath + outFileName + "' to find the issue. ");
    }
    do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    return result;
}

int make_clean(an_itemValues& itemValues)
{
    int result = 0;
    sstr positionCommand = std::string(commandPosition,' ');
    sstr clnFileName = "make_clean_results.txt";
    sstr mkeFileName = "make_results.txt";

    std::vector<sstr> vec;
    if ((itemValues.ProperName    != "Perl")
        && (itemValues.ProperName != "Perl6")
        && (itemValues.ProperName != "Libzip")
        && (itemValues.ProperName != "Cmake"))
    {
        vec.emplace_back("# ");
        vec.emplace_back("# Make clean");
        vec.emplace_back(          "eval \"    cd '" + itemValues.srcPathPNV + "';\n"
               + positionCommand + "make clean > '" + itemValues.bldPath + clnFileName + "' 2>&1 \"");
    }
    result = do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    vec.clear();
    if (result == 0) {
        vec.emplace_back("# Make clean commands completed successfully.");
    } else {
        vec.emplace_back("# Make clean commands NOT completed successfully.");
    }
    do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    return result;
}

int make(an_itemValues& itemValues)
{
    sstr positionCommand = std::string(commandPosition,' ');
    sstr clnFileName = "make_clean_results.txt";
    sstr mkeFileName = "make_results.txt";

    std::vector<sstr> vec;
    vec.emplace_back("# ");
    sstr command = "make ";
    int result = 0;
    if (
        (itemValues.ProperName == "Cmake")
        && (
                (itemValues.thisOSType == OS_type::RedHat)
             || (itemValues.thisOSType == OS_type::Fedora)
             || (itemValues.thisOSType == OS_type::CentOS)
           )
       )
    {
        command = "gmake";
    }
    sstr printCommand = getProperNameFromString(command);
    vec.emplace_back("# " + printCommand);
    vec.emplace_back("eval \"cd '" + itemValues.srcPathPNV + "';\n"
                + positionCommand + command + "  > '" + itemValues.bldPath + mkeFileName + "' 2>&1 \"");
    result = do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    vec.clear();
    if (result == 0) {
        vec.emplace_back("# " + printCommand + " completed successfully.");
    } else {
        vec.emplace_back("# " + printCommand + " NOT completed successfully.");
        vec.emplace_back("# Look through '" + itemValues.bldPath + mkeFileName + "' to find the issue. ");
    }
    do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    return result;
}

int test_php(an_itemValues& itemValues)
{
    std::vector<sstr> vec1;
    if (itemValues.ProperName == "Php") {
        // do nothing..
        // until I can get "expect" to read the input and "send" some data
        // I don't want the tests to hold up the script.
        vec1.emplace_back("# ");
        vec1.emplace_back("# Make test");
        vec1.emplace_back("# The tests must be run manually.");
        vec1.emplace_back("#   So you can answer the questions at the end of the tests.");
        do_command(itemValues.fileName_Build, vec1, itemValues.bScriptOnly);
    }
    return 0;
}

int test_perl6(an_itemValues& itemValues)
{
    int result = 0;
    sstr positionCommand = std::string(commandPosition,' ');
    sstr suffix1 = "make_test_results.txt";
    sstr suffix2 = "rakudo_test_results.txt";
    sstr suffix3 = "rakudo_specTest_results.txt";
    sstr suffix4 = "modules_test_results.txt";

    sstr testPathAndFileName1 = joinPathWithFile(itemValues.bldPath, suffix1);
    sstr testPathAndFileName2 = joinPathWithFile(itemValues.bldPath, suffix2);
    sstr testPathAndFileName3 = joinPathWithFile(itemValues.bldPath, suffix3);
    sstr testPathAndFileName4 = joinPathWithFile(itemValues.bldPath, suffix4);

    std::vector<sstr> vec1;
    vec1.clear();
    vec1.emplace_back("# ");
    vec1.emplace_back("# make test...");
    vec1.emplace_back("# !!! Warning this may take a while...");
    vec1.emplace_back("eval \"cd '" + itemValues.srcPathPNV + "';\n"
                 + positionCommand + "make test > '" + testPathAndFileName1 + "' 2>&1 \"");
    do_command(itemValues.fileName_Build, vec1, itemValues.bScriptOnly);
    vec1.clear();
    vec1.emplace_back("eval \"cd '" + itemValues.srcPathPNV + "';\n"
                 + positionCommand + "make rakudo-test > '" + testPathAndFileName2 + "' 2>&1 \"");
    do_command(itemValues.fileName_Build, vec1, itemValues.bScriptOnly);
    vec1.clear();
    vec1.emplace_back("eval \"cd '" + itemValues.srcPathPNV + "';\n"
                 + positionCommand + "make rakudo-spectest > '" + testPathAndFileName3 + "' 2>&1 \"");
    do_command(itemValues.fileName_Build, vec1, itemValues.bScriptOnly);
    vec1.clear();
    vec1.emplace_back("eval \"cd '" + itemValues.srcPathPNV + "';\n"
                 + positionCommand + " make modules-test > '"    + testPathAndFileName4 + "' 2>&1 \"");
    do_command(itemValues.fileName_Build, vec1, itemValues.bScriptOnly);
    // We are assuming the tests will always fail to some degree,
    //    and we are ok with that, just report conditions normal...
    return result;
}

int make_tests(an_itemValues& itemValues)
{
    int result = 0;
    bool cont = true;
    sstr positionCommand = std::string(commandPosition,' ');
    std::vector<sstr> vec;
    if (itemValues.bDoTests) {
        if (itemValues.ProperName == "Php")   {
            result += test_php(itemValues);
            cont = false;
        }
        if (itemValues.ProperName == "Perl6") {
            test_perl6(itemValues);
            cont = false;
        }

        if (cont) {
            sstr testPathAndFileName = itemValues.bldPath;
            sstr suffix = "test_results.txt";
            testPathAndFileName = joinPathWithFile(testPathAndFileName, suffix);

            vec.emplace_back("# ");
            vec.emplace_back("# Make test(s)...");
            vec.emplace_back("# !!! Warning this may take a while...");
            vec.emplace_back(
                                   "eval \"cd    '" + itemValues.srcPathPNV + "';\n"
               + positionCommand + "make test > '" + testPathAndFileName + "' 2>&1 \"");

            //Most tests have some failures,
            //  so we don't want to fail the install because of a test failure.
            //  so we don't record the result here.
            do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
        }

        // so technically we never fail the tests
        vec.clear();
        vec.emplace_back("# Make test(s) have completed");
        vec.emplace_back("# See results in " + itemValues.bldPath +".");
        do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    }
    return 0;
}

int mariadb_notes(an_itemValues& itemValues)
{
    int result = 0;
    std::vector<sstr> vec;
    if (itemValues.ProperName == "Mariadb") {

        sstr testPathAndFileName = itemValues.bldPath;
        sstr suffix = "test_results_02.txt";

        auto len = itemValues.usrPath.find_first_of("usr");
        sstr perlPath;
        perlPath.append(itemValues.usrPath.substr(0, len - 1));
        perlPath.append("/usr/perl/bin/perl");

        vec.clear();
        testPathAndFileName = joinPathWithFile(testPathAndFileName, suffix);
        vec.emplace_back("# ");
        vec.emplace_back("# optional testing...(after installation and starting)...");
        vec.emplace_back(
                "# eval \"cd '" + itemValues.srcPathPNV + "mysql-test'; '" + perlPath + "' mysql-test-run.pl > " +
                testPathAndFileName + " 2>&1 \"");
        do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);

        vec.clear();
        vec.emplace_back("#################################################################################");
        vec.emplace_back("# ");
        vec.emplace_back("# MariaDB Note Section");
        vec.emplace_back("# ");
        vec.emplace_back("#################################################################################");
        vec.emplace_back("# ");
        vec.emplace_back("#--> Edit the /etc/hosts file to contain the following line:");
        vec.emplace_back("     127.0.1.2   mariadb");
        vec.emplace_back("#");
        vec.emplace_back("#--> Run these commands to secure and start mariadb.");
        vec.emplace_back("#--> Commands run by install (no need to run again)");
        vec.emplace_back("#-->    This information is provided only for your knowledge");
        vec.emplace_back("groupadd   mysql ");
        vec.emplace_back("useradd -g mysql mysql ");
        //Create required directories if needed
        vec.emplace_back("# ");
        vec.emplace_back("mkdir -p '/auth_pam_tool_dir/auth_pam_tool'");
        vec.emplace_back("mkdir -p '" + itemValues.usrPath + "data/temp'");
        vec.emplace_back("mkdir -p '" + itemValues.usrPath + "data/source'");
        vec.emplace_back("mkdir -p '" + itemValues.usrPath + "run'");
        vec.emplace_back("mkdir -p '" + itemValues.usrPath + "var/log'");
        // Add required run files
        vec.emplace_back("# ");
        vec.emplace_back("cd '"       + itemValues.usrPath + "run'");
        vec.emplace_back("touch mariadb.socket ");
        vec.emplace_back("touch mariadb_pid ");
        //set permissions for mariadb directory recursively
        vec.emplace_back("# ");
        vec.emplace_back("cd '" + itemValues.rtnPath + "usr' ");
        vec.emplace_back("chown -R root:mysql mariadb ");
        vec.emplace_back("chmod -R 770        mariadb ");
        //Over ride permissions as required
        vec.emplace_back("# ");
        vec.emplace_back("cd '" + itemValues.usrPath + "'");
        vec.emplace_back("chown -R mysql:mysql data ");
        vec.emplace_back("chmod -R 770         data ");
        vec.emplace_back("chown -R mysql:mysql run  ");
        vec.emplace_back("chmod -R 770         run  ");
        vec.emplace_back("chown -R mysql:mysql var  ");
        vec.emplace_back("chmod -R 770         var  ");
        vec.emplace_back("#--> End of Commands run by install (no need to run again)");
        file_write_vector_to_file(itemValues.fileName_Notes, vec, false);

        create_mariaDB_cnf_File(itemValues);

        vec.clear();
        vec.emplace_back("#--> #");
        vec.emplace_back("#--> Starting here you must run the commands by hand.");
        vec.emplace_back("#-->     Start the server");
        vec.emplace_back("#-->     Set the initial security");
        vec.emplace_back("#-->     Start the client");
        vec.emplace_back("#-->     Run a few test commands");
        vec.emplace_back("cd '" + itemValues.usrPath + "'");
        vec.emplace_back("#");
        vec.emplace_back("# Script the initial database");
        vec.emplace_back("#   Note 1: If you see a lot of permission errors and the script fails...");
        vec.emplace_back("#             It probably means you need to run chmod o+x on all the directories");
        vec.emplace_back("#             from root down to the " + itemValues.usrPath + " directory.");
        vec.emplace_back("#             Once permissions are set up to the mariadb directory, the rest of ");
        vec.emplace_back("#             the permissions should be ok. ");
        vec.emplace_back("#   Note 2: Don't use chmod -R o+x because that would set all the files as well");
        vec.emplace_back("#              and we only need the directories.");
        vec.emplace_back("#   Note 3: Note 1 may not be secure enough for you, In that case you must use");
        vec.emplace_back("#               Access Control Lists, and there are too many different user options ");
        vec.emplace_back("#               to create a detailed list here.");
        vec.emplace_back("# ");
        sstr command = "./scripts/mysql_install_db ";
        command.append(" --defaults-file='");
        command.append(itemValues.etcPath);
        command.append("my.cnf' ");
        command.append(" --basedir='");
        command.append(itemValues.usrPath.substr(0,itemValues.usrPath.length()-1));
        command.append("' ");
        command.append(" --datadir='");
        command.append(itemValues.usrPath);
        command.append("data' ");
        command.append(" --tmpdir='");
        command.append(itemValues.usrPath);
        command.append("data/temp' ");
        command.append(" --socket='");
        command.append(itemValues.usrPath);
        command.append("run/mariadb.socket' ");
        command.append(" --user=mysql");
        vec.emplace_back(command);
        vec.emplace_back("#");
        vec.emplace_back("# start the database ");
        vec.emplace_back("cd '" + itemValues.usrPath + "'");
        command.clear();
        command.append("./bin/mysqld_safe ");
        command.append(" --defaults-file='");
        command.append(itemValues.etcPath);
        command.append("my.cnf' ");
        command.append(" --socket='");
        command.append(itemValues.usrPath);
        command.append("run/mariadb.socket' ");
        command.append(" --user=mysql &");
        vec.emplace_back(command);
        vec.emplace_back("#");
        vec.emplace_back("#Secure the installation after starting server by running command:");
        vec.emplace_back("cd " + itemValues.usrPath);
        command.clear();
        command = "./bin/mysql_secure_installation ";
        command.append(" --socket='");
        command.append(itemValues.usrPath);
        command.append("run/mariadb.socket' ");
        vec.emplace_back(command);
        vec.emplace_back("#");
        vec.emplace_back("#After securing mariadb start the client console:");
        command.clear();
        command = "./bin/mysql ";
        command.append(" --defaults-file='");
        command.append(itemValues.etcPath);
        command.append("my.cnf' ");
        command.append(" --socket='");
        command.append(itemValues.usrPath);
        command.append("run/mariadb.socket' ");
        command.append(" -u root -p ");
        vec.emplace_back(command);
        vec.emplace_back("# ");
        vec.emplace_back("# ");
        vec.emplace_back("# When you want to shutdown run this:");
        vec.emplace_back("cd '" + itemValues.usrPath + "'");
        command.clear();
        command ="./bin/mysqladmin ";
        command.append(" --socket='");
        command.append(itemValues.usrPath);
        command.append("run/mariadb.socket'  ");
        command.append(" -u root -p shutdown ");
        vec.emplace_back(command);
        vec.emplace_back("# ");
        vec.emplace_back("# ");
        vec.emplace_back("# ");
        file_write_vector_to_file(itemValues.fileName_Notes, vec, false);

        vec.clear();
        vec.emplace_back("#");
        vec.emplace_back("# See the Installation_Notes on how to setup and start mariadb.");
        vec.emplace_back("eval \"cd '" + itemValues.rtnPath + "' \"");
        result = do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
        return result;
    }
    return result;
}

int make_install(an_itemValues& itemValues)
{
    sstr positionCommand = std::string(commandPosition,' ');
    std::vector<sstr> vec;
    sstr makePathAndFileName = itemValues.bldPath;
    sstr suffix = "make_install_results.txt";
    makePathAndFileName = joinPathWithFile(makePathAndFileName, suffix);
    vec.emplace_back("# ");
    vec.emplace_back("# Propername....." + itemValues.ProperName);
    vec.emplace_back("# Version........" + itemValues.version);
    vec.emplace_back("# Make install...");
    vec.emplace_back("eval \"      cd '" + itemValues.srcPathPNV + "';\n"
            + positionCommand + "make install > '" + makePathAndFileName + "' 2>&1 \"");

    int result = do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    if (result == 0) {
        vec.clear();
        vec.emplace_back("# Make install completed successfully.");
    } else {
        vec.clear();
        vec.emplace_back("# Make install had some problems.");
        vec.emplace_back("# Look through '" + itemValues.bldPath + "make_install_results.txt' to find the issue. ");
    }
    do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    return result;
}

int decrementResultIfTesting(an_itemValues& itemValues, int in_result)
{
    int result = in_result;
    if (itemValues.bDebug)
    {
        result-=1;
    }
    return result;
}

int do_configure(an_itemValues& itemValues, sstr& configureStr)
{
    int result = 0;
    if (configureStr.length() > 0) {
        if (    (!itemValues.bDebug) ||
                ((itemValues.bDebug) && (itemValues.debugLevel > 0)))
        {
            result = configure(itemValues, configureStr);
            if (itemValues.debugLevel == 1) {
                result = decrementResultIfTesting(itemValues, result);
            }
        }
    }
    return result;
}

int do_make_clean(an_itemValues& itemValues)
{
    int result = 0;
    if (    (!itemValues.bDebug) ||
            ((itemValues.bDebug) && (itemValues.debugLevel > 1)))
    {
        result = make_clean(itemValues);
        if (itemValues.debugLevel == 2) {
            result = decrementResultIfTesting(itemValues, result);
        }
    }
    return result;
}

int do_make(an_itemValues& itemValues)
{
    int result = 0;
    if (    (!itemValues.bDebug) ||
            ((itemValues.bDebug) && (itemValues.debugLevel > 2)))
    {
        result = make(itemValues);
        if (itemValues.debugLevel == 3) {
            result = decrementResultIfTesting(itemValues, result);
        }
    }
    return result;
}

int do_make_tests(an_itemValues& itemValues)
{
    int result = 0;
    if (    (!itemValues.bDebug) ||
            ((itemValues.bDebug) && (itemValues.debugLevel > 3)))
    {
        result = make_tests(itemValues);
        if (itemValues.debugLevel == 4) {
            result = decrementResultIfTesting(itemValues, result);
        }
    }
    return result;
}

int do_make_install(an_itemValues& itemValues, int results)
{
    if (    (!itemValues.bDebug) ||
            ((itemValues.bDebug) && (itemValues.debugLevel > 4))) {
        if (results == 0) {
            results = make_install(itemValues);
        }
        if ((results == 0) && (itemValues.ProperName == "Apache")) {
            results += apache_notes(itemValues);
        }
        if ((results == 0) && (itemValues.ProperName == "Mariadb")) {
            results += mariadb_notes(itemValues);
        }
        if (itemValues.debugLevel == 5) {
            results = decrementResultIfTesting(itemValues, results);
        }
    }
    return results;
}

int postInstall_Apache(an_itemValues& itemValues)
{
    int result = 0;
    std::vector<sstr> vec;
    sstr positionCommand = std::string(commandPosition, ' ');
    vec.clear();
    vec.emplace_back("# ");
    vec.emplace_back("# Copy apache configuration files to '" + itemValues.etcPath + "extra'");
    vec.emplace_back("mkdir -p '" + itemValues.etcPath + "extra' ");
    vec.emplace_back("cp   -rf '" + itemValues.usrPath + "conf/extra/'* '" + itemValues.etcPath  + "extra/.' ");
    int temp = do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    std::cout << "cp results = " << temp << std::endl;
    vec.clear();
    if (temp == 0) {
        vec.emplace_back("# Copy apache configuration files was successful.");
    } else {
        vec.emplace_back("# Copy apache configuration files was NOT successful.");
    }
    result += temp;

    //set permissions for apache directory recursively
    vec.clear();
    vec.emplace_back("# ");
    vec.emplace_back("# Change Apache permissions.");
    vec.emplace_back("chown -R root:" + APACHE_GROUP + " '" + itemValues.usrPath + "'");
    vec.emplace_back("chmod -R 770  '"                      + itemValues.usrPath + "'");
    temp = do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);

    vec.clear();
    vec.emplace_back("# ");
    vec.emplace_back("chown -R root:" + APACHE_GROUP + " '" + itemValues.etcPath + "'");
    vec.emplace_back("chmod -R 770 '" + itemValues.etcPath + "'");
    temp += do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);

    vec.clear();
    if (temp == 0) {
        vec.emplace_back("# Change Apache file permissions were successful.");
    } else {
        vec.emplace_back("# Change Apache file permissions were NOT successful.");
    }
    do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    result += temp;

    return result;
}

int postInstall_MariaDB(std::map<sstr, sstr>& settings, an_itemValues& itemValues)
{
    int result = 0;
    sstr positionCommand = std::string(commandPosition, ' ');
    std::vector<sstr> vec;

    //Create required directories if needed
    vec.clear();
    vec.emplace_back("# ");
    vec.emplace_back("mkdir -p '" + itemValues.usrPath + "data/temp'");
    vec.emplace_back("mkdir -p '" + itemValues.usrPath + "data/source'");
    vec.emplace_back("mkdir -p '" + itemValues.usrPath + "run'");
    vec.emplace_back("mkdir -p '" + itemValues.usrPath + "var/log'");

    // Add required run files
    vec.emplace_back("# ");
    vec.emplace_back("cd '"       + itemValues.usrPath + "run'");
    vec.emplace_back("touch mariadb.socket ");
    vec.emplace_back("touch mariadb_pid ");
    //set permissions for mariadb directory recursively
    vec.emplace_back("# ");
    vec.emplace_back("eval \"cd '" + itemValues.rtnPath + "usr' \";\n ");
    vec.emplace_back("chown -R root:" + MARIADB_GROUP + " " + itemValues.programName );
    vec.emplace_back("chmod -R 770  '" + itemValues.programName + "'");
    //Over ride permissions as required
    vec.emplace_back("# ");
    vec.emplace_back("eval \"cd '" + itemValues.usrPath + "'\";\n");
    vec.emplace_back("chown -R " + MARIADB_OWNER + ":" + MARIADB_GROUP  + " data ");
    vec.emplace_back("chmod -R 770         data ");
    vec.emplace_back("chown -R " + MARIADB_OWNER + ":" + MARIADB_GROUP  + " run ");
    vec.emplace_back("chmod -R 770         run  ");
    vec.emplace_back("chown -R " + MARIADB_OWNER + ":" + MARIADB_GROUP  + " var ");
    vec.emplace_back("chmod -R 770         var  ");
    result += do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    return result;
}

int postInstall_PHP(std::map<sstr, sstr>& settings, an_itemValues& itemValues)
{
    int result = 0;
    sstr positionCommand = std::string(commandPosition, ' ');

    sstr compileForDebug    = settings[itemValues.programName + "->Compile_For_Debug"];
    sstr xdebug_install     = settings[itemValues.programName + "->Xdebug_Install"];
    sstr xdebug_name        = settings[itemValues.programName + "->Xdebug_Name"];
    sstr xdebug_version     = settings[itemValues.programName + "->Xdebug_Version"];
    sstr xdebug_compression = settings[itemValues.programName + "->Xdebug_Compression"];
    sstr xdebug_wget        = settings[itemValues.programName + "->Xdebug_WGET"];
    sstr xdebug_tar_options = settings[itemValues.programName + "->Xdebug_Tar_Options"];
    sstr zts_version        = settings[itemValues.programName + "->zts_version"];
    bool bCompileForDebug   = getBoolFromString(compileForDebug);
    bool bInstall_Xdebug    = getBoolFromString(xdebug_install);

    sstr xDebugProgVersion        =  xdebug_name + "-" + xdebug_version;
    sstr xDebugCompressedFileName =  xDebugProgVersion + xdebug_compression;

    sstr tmpPath = "usr/apache/bin";
    sstr apePath = joinPathParts(itemValues.rtnPath, tmpPath);

    sstr tmpFile = "apachectl";
    sstr apacheController  = joinPathWithFile(apePath, tmpFile);

    tmpFile = "mysqld";
    sstr mariadbPath = itemValues.rtnPath + "usr/mariadb/bin";
    sstr mariadbController = joinPathWithFile(mariadbPath, tmpFile);

    std::vector<sstr> vec;
    vec.clear();
    vec.emplace_back("eval \"cd '" + itemValues.usrPath + "'; mkdir -p libs \"");
    result += do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);

    vec.clear();
    vec.emplace_back("# ");
    vec.emplace_back("# Copy Php.ini files to '" + itemValues.etcPath + "'");
    vec.emplace_back("eval \"cd '" + itemValues.srcPathPNV + "'; cp *.ini* '" + itemValues.etcPath  + ".' \"");
    vec.emplace_back("# ");
    vec.emplace_back("# libtool --finish");
    vec.emplace_back("eval \"cd '" + itemValues.srcPathPNV + "'; cp libs/* '" + itemValues.usrPath + "libs/.' \"");
    vec.emplace_back("eval \"cd '" + itemValues.srcPathPNV + "'; ./libtool --finish '" + itemValues.usrPath + "libs' \"");
    vec.emplace_back("# ");
    vec.emplace_back("# Copy library to apache web server");
    vec.emplace_back("eval \"cp '" + itemValues.usrPath + "libs/libphp7.so' '" + itemValues.rtnPath + "usr/apache/modules/libphp7.so' \"");
    vec.emplace_back("eval \"cp '" + itemValues.usrPath + "libs/libphp7.so' '" + itemValues.rtnPath + "usr/apache/modules/mod_php7.so' \"");
    int temp = do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    vec.clear();
    if (temp == 0) {
        vec.emplace_back("# Copy library file operations were successful.");
    } else {
        vec.emplace_back("# Copy library file operations were NOT successful.");
    }
    result += temp;
    do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);

    vec.clear();
    vec.emplace_back("# ");
    vec.emplace_back("# Ensure Apache user / group exists");
    do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);

    vec.clear();
    vec.emplace_back("# ");
    vec.emplace_back("# Set apache ownership");
    vec.emplace_back("eval \"chown root:" + APACHE_GROUP + " '" + itemValues.rtnPath + "usr/apache/modules/libphp7.so' \"");
    vec.emplace_back("eval \"chown root:" + APACHE_GROUP + " '" + itemValues.rtnPath + "usr/apache/modules/mod_php7.so' \"");
    vec.emplace_back("# ");
    vec.emplace_back("# Set apache permissions");
    vec.emplace_back("eval \"chmod 755 '" + itemValues.rtnPath + "usr/apache/modules/libphp7.so' \"");
    vec.emplace_back("eval \"chmod 755 '" + itemValues.rtnPath + "usr/apache/modules/mod_php7.so' \"");
    temp = do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    vec.clear();
    if (temp == 0) {
        vec.emplace_back("# Change ownership and permission file operations were successful.");
    } else {
        vec.emplace_back("# Change ownership and permission file operations NOT were successful.");
    }
    result += temp;
    do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);

    sstr xdebugProgVersionCompression = xdebug_version + xdebug_compression;

    // When analyzing code, view this whole block together...
    //    if we install Xdebug, are we installing for PHP (debug mode) or PHP (non debug mode)?
    //    depending on the mode of PHP we need to change some text below...
    if (bInstall_Xdebug) {
        vec.clear();
        vec.emplace_back("# ");
        vec.emplace_back("# wget xdebug");
        vec.emplace_back("eval \"cd '" + itemValues.usrPath + "';\nwget '" + xdebug_wget + xDebugCompressedFileName + "' \"");
        vec.emplace_back(
                "eval \"cd '" + itemValues.usrPath + "'; tar '" + xdebug_tar_options + "' '" + xDebugCompressedFileName +
                "' \"");

        vec.emplace_back("# ");
        vec.emplace_back("# phpize");
        vec.emplace_back("eval \"cd '" + itemValues.usrPath + xDebugProgVersion + "';\n ../bin/phpize > '" + itemValues.bldPath + "phpize.txt' \"");
        vec.emplace_back("# ");
        vec.emplace_back("# config");
        vec.emplace_back("eval \"cd '" + itemValues.usrPath + xDebugProgVersion + "'; ./configure --with-php-config='"
                         + itemValues.usrPath + "bin/php-config' > '" + itemValues.bldPath + "xdebug-configure.txt' \"");

        temp = do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
        vec.clear();
        if (temp == 0) {
            vec.emplace_back("# Wget, phpize, and configure commands were successful.");
        } else {
            vec.emplace_back("# Wget, phpize, and configure commands were NOT successful.");
        }
        result += temp;
        do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);

        vec.clear();
        vec.emplace_back("# ");
        vec.emplace_back("# make");
        vec.emplace_back("eval \"cd '" + itemValues.usrPath + xDebugProgVersion + "'; make > '"
                         + itemValues.bldPath + "xdebug-make.txt' \"");
        temp = do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
        vec.clear();
        if (temp == 0) {
            vec.emplace_back("# Make command was successful.");
        } else {
            vec.emplace_back("# Make command was NOT successful.");
        }
        result += temp;
        do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);

        vec.clear();
        vec.emplace_back("# ");
        vec.emplace_back("# cp modules/xdebug.so");

        // checking for the mode of PHP and adjusting accordingly
        if (bCompileForDebug) {
            vec.emplace_back("eval \"cd '" + itemValues.usrPath + xDebugProgVersion + "'; cp modules/xdebug.so '"
                             + itemValues.usrPath + "lib/php/extensions/debug-zts-" + zts_version + "' \"");

        } else {
            vec.emplace_back("eval \"cd '" + itemValues.usrPath + xDebugProgVersion + "'; cp modules/xdebug.so '"
                             + itemValues.usrPath + "lib/php/extensions/no-debug-zts-" + zts_version + "' \"");

        }
        result += do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);

        // This small section is the same for PHP
        //   regardless of the debug / non-debug mode.
        vec.clear();
        vec.emplace_back("# ");
        vec.emplace_back("# Create: '" + itemValues.etcPath + "lib'");
        vec.emplace_back("eval \"mkdir -p '" + itemValues.etcPath + "lib' \"");
        // end of small section

        // checking for the mode of PHP and adjusting accordingly
        if (bCompileForDebug) {
            vec.emplace_back("# ");
            vec.emplace_back("# zend_extension = '" + itemValues.usrPath + "lib/php/extensions/debug-zts-" + zts_version +
                             "/xdebug.so'");
            vec.emplace_back("eval \"cd '" + itemValues.etcPath + "lib/';\necho zend_extension = '"
                             + itemValues.usrPath + "lib/php/extensions/debug-zts-" + zts_version +
                             "/xdebug.so' > php_ext.ini \"");
        } else {
            vec.emplace_back("# ");
            vec.emplace_back("# zend_extension = '" + itemValues.usrPath + "lib/php/extensions/debug-zts-" + zts_version +
                             "/xdebug.so'");
            vec.emplace_back("eval \"cd '" + itemValues.etcPath + "lib/';\necho zend_extension = '"
                             + itemValues.usrPath + "lib/php/extensions/no-debug-zts-" + zts_version +
                             "/xdebug.so' > php_ext.ini \"");
        }
    } else {
        vec.emplace_back("# Xdebug not installed.");
    }
    temp = do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    vec.clear();
    if (temp == 0) {
        vec.emplace_back("# Copy libraries and modules was successful.");
    } else {
        vec.emplace_back("# Copy libraries and modules was not successful.");
    }
    result += temp;
    do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    return result;
}

int do_post_install(std::map<sstr, sstr>& settings, an_itemValues& itemValues, int results)
{
    int newResults = 0;
    if (    (!itemValues.bDebug) ||
            ((itemValues.bDebug) && (itemValues.debugLevel > 5))) {
        if (results == 0)
        {
            if (itemValues.ProperName == "Php")
            {
                newResults = postInstall_PHP(settings, itemValues);
            }
            if (itemValues.ProperName == "Apache")
            {
                newResults = postInstall_Apache(itemValues);
            }
            if (itemValues.ProperName == "MariaDB")
            {
                newResults = postInstall_MariaDB(settings, itemValues);
            }
        }
        if (itemValues.debugLevel == 6) {
            newResults = decrementResultIfTesting(itemValues, results);
        }
    }
    return newResults;
}

int basicInstall(an_itemValues& itemValues, sstr& configureStr)
{
    int result = 0;
    std::vector<sstr> vec;

    if (    (!itemValues.bDebug) ||
            ((itemValues.bDebug) && (itemValues.debugLevel > -1))) {
        if (itemValues.debugLevel == 0){
            result = -1;
        }
        if (result == 0) {
            result = do_configure(itemValues, configureStr);
        }
        if (result == 0) {
            result += do_make_clean(itemValues);
        }
        if (result == 0) {
            result += do_make(itemValues);
        }
        if (result == 0) {
            result += do_make_tests(itemValues);
        }
        if (result == 0) {
            result += do_make_install(itemValues, result);
        }
    }
    if (result == 0 )
    {
        vec.clear();
        vec.emplace_back("# ");
        vec.emplace_back("# Summary Status");
        vec.emplace_back("# " + itemValues.ProperName + " install was successful.");
    }
    else
    {
        vec.clear();
        if (itemValues.bDebug)
        {
            sstr temp1 = "# Install for " + itemValues.ProperName + " blocked because Debug_Only  = True";
            sstr temp2 = "# Install for " + itemValues.ProperName + " blocked because Debug_Level = " + std::to_string(itemValues.debugLevel);
            vec.emplace_back(temp1);
            vec.emplace_back(temp2);
        }
        else
        {
            vec.emplace_back("# " + itemValues.ProperName + " Install has some issues.");
            vec.emplace_back("# Look through the build logs in the '" + itemValues.bldPath + "' directory.");
        }
    }
    do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    return result;
}

int basicInstall_tcl(an_itemValues& itemValues, sstr& configureStr)
{
    int result = -1;
    bool skipTests = false;

    sstr tmpPath = "usr/Tcl_Tk";
    sstr usrPath = joinPathParts(itemValues.rtnPath, tmpPath);

    std::vector<sstr> vec1;
    std::vector<sstr> vec2;

    if (!itemValues.bDebug) {

        vec1.clear();
        vec1.emplace_back("# ");
        vec1.emplace_back("# Configure...");
        vec1.emplace_back("# Piping results to the \"'" + itemValues.bldPath + "'\" directory.");

        configureStr.append(" > '" + itemValues.bldPath + "configure_results.txt' 2>&1 ");
        vec1.emplace_back(configureStr);
        vec1.emplace_back("# ");
        vec1.emplace_back("# make");

        sstr makePathAndFileName = itemValues.bldPath;
        sstr suffix = "make_results.txt";
        makePathAndFileName = joinPathWithFile(makePathAndFileName, suffix);
        vec1.emplace_back("eval \"cd '" + itemValues.srcPathInstallOS + "'; make -j  > '" + makePathAndFileName + "' 2>&1 \"");
        result = do_command(itemValues.fileName_Build, vec1, itemValues.bScriptOnly);

        if (itemValues.bDoTests) {

            if (!skipTests) {
                sstr testPathAndFileName = itemValues.bldPath;
                suffix = "test_results.txt";
                testPathAndFileName = joinPathWithFile(testPathAndFileName, suffix);

                vec2.emplace_back("# ");
                vec2.emplace_back("# make test");
                vec2.emplace_back("# !!! Warning this may take a while...");
                vec2.emplace_back("eval \"cd '" + itemValues.srcPathInstallOS + "'; make test > '" + testPathAndFileName + "' 2>&1 \"");
                do_command(itemValues.fileName_Build, vec2, itemValues.bScriptOnly);
            }
        }
        vec1.clear();
        vec1.emplace_back("# ");
        vec1.emplace_back("# make install");
        vec1.emplace_back("eval \"cd '" + itemValues.srcPathInstallOS + "'; make install > '" + itemValues.bldPath + "install_results.txt' 2>&1 \"");
        vec1.emplace_back("# ");
        result += do_command(itemValues.fileName_Build, vec1, itemValues.bScriptOnly);

        if (itemValues.programName == "tcl") {
            vec2.clear();
            vec2.emplace_back("# ");
            vec2.emplace_back("# ");
            vec2.emplace_back("#################################################################################");
            vec2.emplace_back("# ");
            vec2.emplace_back("# Tcl/Tk Note Section");
            vec2.emplace_back("# ");
            vec2.emplace_back("#################################################################################");
            vec2.emplace_back("# ");
            vec2.emplace_back("# ");
            vec2.emplace_back("# To use tcl cd to this directory: ");

            sstr command = "cd '";
            command.append(usrPath);
            command.append("bin/'");
            vec2.emplace_back(command);
            vec2.emplace_back("ls -al");
            command.clear();
            command = "# Then run tcl with ./tcl [tab complete] then [enter]";
            vec2.emplace_back(command);
            vec2.emplace_back("# At this point you should see a prompt like % ");
            vec2.emplace_back("# type after prompt--> % info tclversion ");
            vec2.emplace_back("#   and you should see the version you installed. ");
            vec2.emplace_back("#   add the path to your PATH variable to run this version from any directory.");
            vec2.emplace_back("# type after prompt--> % wish ");
            vec2.emplace_back("#   and you should see a small new window open up. ");
            vec2.emplace_back("#   depending on how you installed this you may have change permissions.");
            vec2.emplace_back("#   chmod o+rx in the directory tree as appropriate.");
            vec2.emplace_back("#  Some reference material ");
            vec2.emplace_back("#    1. The Tcl and TK programming for the Absolute Beginner by Kurt Wall");
            vec2.emplace_back("#    2. The TCL Programming Language by Ashok P. Nadkarni");
            vec2.emplace_back("#    3. The TCL and the Tk Toolkit by John K. Outsterhout and Ken Jones");
            vec2.emplace_back("#    4. The TCL/Tk A Developers Guide by Clif Flynt");
            vec2.emplace_back("# ");
            file_write_vector_to_file(itemValues.fileName_Notes, vec2, false);
        }
    }
    if (result == 0 )
    {
        vec1.clear();
        vec1.emplace_back("# Install was successful.");
    }
    else
    {
        vec1.clear();
        vec1.emplace_back("# Install had some issues.");
        vec1.emplace_back("# Look through the build logs in the '" + itemValues.bldPath + "' directory.");
    }
    do_command(itemValues.fileName_Build, vec1, itemValues.bScriptOnly);
    return result;
}

void set_bInstall(an_itemValues &itemValues)
{
    std::vector<sstr> vec;
    sstr protectionFileWithPath = get_ProtectionFileWithPath(itemValues);

    bool bProtected = false;

    vec.emplace_back("#");
    vec.emplace_back("# Check for " + itemValues.ProperName + " protection file.");
    do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    if (isFileSizeGtZero(itemValues, protectionFileWithPath, true)) {
        bProtected = true;
        howToRemoveFileProtection(itemValues);
    }
    if ((bProtected) && (directoryExists(itemValues.usrPath)))
    {
        bProtected = true;
        vec.emplace_back("# The " + itemValues.ProperName + " usr directory (" + itemValues.usrPath + ") exists already.");
        vec.emplace_back("# This will prevent the installation out of caution. ");
        vec.emplace_back("# Remove this directory to install this program.");
        vec.emplace_back("# you can use the command below to do it: (without the '#')");
        vec.emplace_back("# rm -rf '" + itemValues.usrPath + "'");
        do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
    }
    if (bProtected)
    {
        itemValues.bProtectMode = true;
        itemValues.bInstall = false;
    }
}

void createProtectionWhenRequired(int result, an_itemValues& itemValues, bool show)
{
    std::vector<sstr> vec;
    if ((itemValues.bProtectMode) && (result == 0))
    {
        protectProgram_IfRequired(itemValues, show);
        howToRemoveFileProtection(itemValues);
    }
}

sstr get_InstallOS(OS_type thisOS_Type)
{
    // from above...
    // enum class OS_type
    // { Selection_Not_Available = -1,
    //   No_Selection_Made = 0,
    //   CentOS = 1,
    //   Fedora = 2,
    //   Linux_Mint = 3,
    //   RedHat = 4,
    //   MaxOSX = 5};
    sstr installOS = "unknown";
    if (thisOS_Type == OS_type::CentOS)     { installOS = "unix";  }
    if (thisOS_Type == OS_type::Fedora)     { installOS = "unix";  }
    if (thisOS_Type == OS_type::RedHat)     { installOS = "unix";  }
    if (thisOS_Type == OS_type::Linux_Mint) { installOS = "unix";  }
    if (thisOS_Type == OS_type::MaxOSX)     { installOS = "macosx";  }

    return  installOS;
}

sstr getProtectedFileName(sstr& programName)
{
    sstr protectedFileName = "protection";
    protectedFileName.append("-");
    protectedFileName.append(programName);
    protectedFileName.append(".txt");
    return protectedFileName;
}

sstr create_php_configuration(std::map<sstr, sstr>& settings, an_itemValues& itemValues)
{
    sstr positionCommand = std::string(commandPosition, ' ');
    sstr  compileForDebug   = settings[itemValues.programName + "->Compile_For_Debug"];
    bool bCompileForDebug   = getBoolFromString(compileForDebug);
    sstr sslVersion     = settings["openssl->Version"];
    sstr openSSLVersion = "openssl";
    openSSLVersion.append("-");
    openSSLVersion.append(sslVersion);
    //
    // Note: Don't end the command with \" to close the command here.
    //   We are going to append more to the command in the function
    //     and end the command with \" there.

    sstr tmpPath = "usr/apache/bin";
    sstr apePath = joinPathParts(itemValues.rtnPath, tmpPath);

    sstr tmpFile = "apachectl";
    sstr apacheController  = joinPathWithFile(apePath, tmpFile);

    tmpFile = "mysqld";
    sstr mariadbPath = itemValues.rtnPath + "usr/mariadb/bin";
    sstr mariadbController = joinPathWithFile(mariadbPath, tmpFile);

    std::vector<sstr> commands;
    sstr configureStr = "eval \"set PKG_CONFIG_PATH /usr/lib64/pkgconfig;\"";
    configureStr.append("\n");

    sstr temp = "";
    temp.append(positionCommand);
    temp.append("cd '");
    temp.append(itemValues.srcPathPNV);
    temp.append("';\n");
    configureStr.append(temp);

    temp.clear();
    temp.append(positionCommand);
    temp.append("./configure");
    temp.append("  --prefix='");
    temp.append(itemValues.usrPath);
    temp.append("'");
    commands.emplace_back(temp);

    temp.clear();
    temp.append(positionCommand);
    temp.append("  --exec-prefix='");
    temp.append(itemValues.usrPath);
    temp.append("'");
    commands.emplace_back(temp);

    temp.clear();
    temp.append(positionCommand);
    temp.append("  --srcdir='");
    temp.append(itemValues.srcPathPNV);
    temp.append("'");
    commands.emplace_back(temp);

    temp.clear();
    temp.append(positionCommand);
    temp.append("  --with-openssl='" + itemValues.rtnPath + "usr/openssl/'");
    commands.emplace_back(temp);

    temp.clear();
    tmpPath = "usr/apache/bin/";
    sstr apxPathFile = joinPathParts(itemValues.rtnPath, tmpPath);
    tmpFile = "apxs";
    apxPathFile = joinPathWithFile(apePath, tmpFile);
    temp.append(positionCommand);
    temp.append("  --with-apxs2='");
    temp.append(apxPathFile);
    temp.append("'");
    commands.emplace_back(temp);

    temp.clear();
    temp.append(positionCommand);
    temp.append("  --enable-mysqlnd ");
    commands.emplace_back(temp);

    temp.clear();
    temp.append(positionCommand);
    sstr pcrePath = "/usr/pcre";
    pcrePath = joinPathParts(itemValues.rtnPath, pcrePath);
    temp.append("  --with-pcre-regex='");
    temp.append(pcrePath);
    temp.append("'");
    commands.emplace_back(temp);

    temp.clear();
    temp.append(positionCommand);
    temp.append("  --with-config-file-path='");
    tmpPath = "lib";
    sstr libPath = joinPathParts(itemValues.usrPath, tmpPath);
    temp.append(libPath);
    temp.append("'");
    commands.emplace_back(temp);

    temp.clear();
    temp.append(positionCommand);
    temp.append("  --with-config-file-scan-dir='");
    temp.append(itemValues.etcPath);
    sstr crlPath = "/usr/bin";
    temp.append("'");
    commands.emplace_back(temp);

    if (itemValues.thisOSType != OS_type::Fedora)
    {
        //I tried everything I could think of
        // and I couldn't get --with-curl to work on Fedora
        // even though the files were there the ./config command
        // couldn't find them.  I also tried tweaking the permissions
        // on the files, and still nothing.

        //I will probably have to work on this later...

        //The problem is with defaults, where some code is looking in lib and other code is looking in lib64
        //I still have to deffer this till later.

        temp.clear();
        temp.append(positionCommand);
        temp.append("  --with-curl='");
        temp.append(crlPath);
        temp.append("'");
        commands.emplace_back(temp);
    }

    temp.clear();
    temp.append(positionCommand);
    temp.append("  --with-mysql-sock='");
    tmpPath = "usr/mariadb/run/";
    sstr sckPathFile = joinPathParts(itemValues.rtnPath, tmpPath);
    tmpFile = "mariadb.socket";
    sckPathFile = joinPathWithFile(sckPathFile, tmpFile);
    temp.append(sckPathFile);
    temp.append("'");
    commands.emplace_back(temp);

    temp.clear();
    temp.append(positionCommand);
    temp.append("  --with-libzip='");
    tmpPath = "/usr/libzip";
    sstr libZipPath =  joinPathParts(itemValues.rtnPath, tmpPath);
    temp.append(libZipPath);
    temp.append("' ");
    commands.emplace_back(temp);

    temp.clear();
    temp.append(positionCommand);
    temp.append("  --with-zlib-dir='");
    tmpPath = "usr/mariadb/";
    sstr zlibPath = joinPathParts(itemValues.rtnPath, tmpPath);
    temp.append(zlibPath);
    temp.append("'");
    commands.emplace_back(temp);

    commands.emplace_back(positionCommand + "  --with-pdo-mysql=mysqlnd");
    commands.emplace_back(positionCommand + "  --enable-embedded-mysqli");
    commands.emplace_back(positionCommand + "  --disable-cgi");
    commands.emplace_back(positionCommand + "  --disable-short-tags");
    commands.emplace_back(positionCommand + "  --enable-bcmath");
    commands.emplace_back(positionCommand + "  --with-pcre-jit");
    commands.emplace_back(positionCommand + "  --enable-sigchild");
    commands.emplace_back(positionCommand + "  --enable-libgcc");
    commands.emplace_back(positionCommand + "  --enable-calendar");
    commands.emplace_back(positionCommand + "  --enable-dba=shared");
    commands.emplace_back(positionCommand + "  --enable-ftp");
    commands.emplace_back(positionCommand + "  --enable-intl");
    commands.emplace_back(positionCommand + "  --enable-mbstring");
    commands.emplace_back(positionCommand + "  --enable-zend-test");

    if ( bCompileForDebug ){
        commands.emplace_back(positionCommand + "  --enable-debug");
    }

    configureStr.append(multilineCommand(commands, false));
    return  configureStr;
}

//
// install functions
//

int install_cmake(std::map<sstr, sstr>& settings, an_itemValues& itemValues) {
    int result = -1;
    sstr positionCommand = std::string(commandPosition, ' ');
    sstr command;
    std::vector<sstr> vec;

    set_bInstall(itemValues);
    if (itemValues.bInstall) {
        appendNewLogSection(itemValues.fileName_Build);
        ensureStageDirectoryExists(itemValues);
        stageSourceCodeIfNeeded(itemValues);
        itemValues.srcPathPNV = joinPathParts(itemValues.srcPath, itemValues.programNameVersion);

        bool securityCheck = check_Sha256sum(itemValues);
        if (securityCheck) {

            result = setupInstallDirectories(itemValues);
            // Unlike all the other install_xxx functions this time we have to call configure
            //   two times, so the first time we call it individually with the bootstrap command.
            //   The second time we pass gmake to the basicInstall function
            //   But that happens at a sub function level.

            sstr configureStr = "eval \"cd '" + itemValues.srcPathPNV + "'\"\n";
            std::vector<sstr> commands;
            commands.emplace_back(positionCommand + " ./bootstrap --prefix='" + itemValues.usrPath + "' ");
            configureStr.append(multilineCommand(commands, false));

            result += basicInstall(itemValues, configureStr);
            createProtectionWhenRequired(result, itemValues, false);
        } else {
            result = badSha256sum(itemValues);
        }
    }
    return result;
}

int install_libzip(std::map<sstr, sstr>& settings, an_itemValues& itemValues)
{
    int result = -1;
    sstr positionCommand = std::string(commandPosition, ' ');
    sstr command;
    std::vector<sstr> vec;

    set_bInstall(itemValues);
    if (itemValues.bInstall)
    {
        appendNewLogSection(itemValues.fileName_Build);
        ensureStageDirectoryExists(itemValues);
        stageSourceCodeIfNeeded(itemValues);
        itemValues.srcPathPNV = joinPathParts(itemValues.srcPath, itemValues.programNameVersion);
        sstr tmpPath = "build";
        itemValues.srcPathPNV = joinPathParts(itemValues.srcPathPNV, tmpPath);

        bool securityCheck = check_Sha256sum(itemValues);
        if (securityCheck)
        {
            result = setupInstallDirectories(itemValues);
            vec.clear();
            vec.emplace_back("#");
            vec.emplace_back("# Special Instructions for libzip");
            vec.emplace_back("eval \"mkdir -p '" + itemValues.srcPathPNV + "' \"");
            vec.emplace_back("eval \"cd       '" + itemValues.srcPathPNV + "';\n"
                             + positionCommand + " '" + itemValues.rtnPath + "usr/cmake/bin/cmake' ..\\\n"
                             + positionCommand + " -DCMAKE_INSTALL_PREFIX='" + itemValues.usrPath + "' \\\n"
                             + positionCommand +"> '" + itemValues.bldPath + "custom_cmake.txt'\"");
            result += do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);

            if (result == 0) {
                sstr configureStr = "# No configuration command required. \\\n";
                result += basicInstall(itemValues, configureStr);
                createProtectionWhenRequired(result, itemValues, false);
            }
            else
            {
                vec.clear();
                vec.emplace_back("#");
                vec.emplace_back("# non-system cmake failed");
                do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
            }
        } else {
            result = badSha256sum(itemValues);
        }
    }
    return result;
}

int install_perl5(std::map<sstr, sstr>& settings, an_itemValues& itemValues)
{
    int result = -1;
    sstr positionCommand = std::string(commandPosition, ' ');
    sstr command;
    std::vector<sstr> vec;

    set_bInstall(itemValues);
    if (itemValues.bInstall)
    {
        appendNewLogSection(itemValues.fileName_Build);
        ensureStageDirectoryExists(itemValues);
        stageSourceCodeIfNeeded(itemValues);
        itemValues.srcPathPNV = joinPathParts(itemValues.srcPath, itemValues.programNameVersion);

        bool securityCheck = check_Sha256sum(itemValues);
        if (securityCheck)
        {
            result = setupInstallDirectories(itemValues);
            if (result == 0) {
                sstr configureStr   = "eval \"cd   '" + itemValues.srcPathPNV + "'\";\n "
                  + positionCommand + "./Configure  -Dprefix='" + itemValues.usrPath + "' -d -e \\\n ";
                result += basicInstall(itemValues, configureStr);
            }
            if (result == 0) {
                createProtectionWhenRequired(result, itemValues, false);
            }
        } else {
            result = badSha256sum(itemValues);
        }
    }
    settings["perl5RunPath"] = itemValues.usrPath + "bin";
    return result;
}

int install_openssl(std::map<sstr, sstr>& settings, an_itemValues& itemValues)
{
    int result = -1;
    sstr positionCommand = std::string(commandPosition, ' ');
    sstr command;
    std::vector<sstr> vec;

    set_bInstall(itemValues);
    if (itemValues.bInstall)
    {
        appendNewLogSection(itemValues.fileName_Build);
        ensureStageDirectoryExists(itemValues);
        stageSourceCodeIfNeeded(itemValues);
        itemValues.srcPathPNV = joinPathParts(itemValues.srcPath, itemValues.programNameVersion);

        bool securityCheck = check_Sha256sum(itemValues);
        if (securityCheck)
        {
            result = setupInstallDirectories(itemValues);
            sstr configureStr = "eval \"cd '" + itemValues.srcPathPNV + "' \"; \n"
                    + positionCommand + " ./config --prefix='" + itemValues.usrPath + "' \\\n";
            result += basicInstall(itemValues, configureStr);
            createProtectionWhenRequired(result, itemValues, false);
        } else {
            result = badSha256sum(itemValues);
        }
    }
    return result;
}

int install_mariadb(std::map<sstr, sstr>& settings, an_itemValues& itemValues)
{
    int result = -1;
    sstr positionCommand = std::string(commandPosition, ' ');
    sstr command;
    std::vector<sstr> vec;

    set_bInstall(itemValues);
    if (itemValues.bInstall)
    {
        appendNewLogSection(itemValues.fileName_Build);
        ensureStageDirectoryExists(itemValues);
        stageSourceCodeIfNeeded(itemValues);
        itemValues.srcPathPNV = joinPathParts(itemValues.srcPath, itemValues.programNameVersion);



        bool securityCheck = check_Sha256sum(itemValues);
        if (securityCheck)
        {
            result = setupInstallDirectories(itemValues);

            ensureMariaDB_UserAndGroupExist(itemValues);
            sstr configureStr = "eval \"cd " + itemValues.srcPathPNV + "\";\n "
                       + positionCommand + "./BUILD/autorun.sh;\n "
                       + positionCommand + " cd '" + itemValues.srcPathPNV + "';\n";

            std::vector<sstr> commands;
            commands.emplace_back(positionCommand +  "./configure --prefix='" + itemValues.usrPath + "'");
            commands.emplace_back(positionCommand +  "  --enable-assembler");
            commands.emplace_back(positionCommand +  "  --jemalloc_static_library='/usr/lib64'");
            commands.emplace_back(positionCommand +  "  --with-extra-charsets=complex");
            commands.emplace_back(positionCommand +  "  --enable-thread-safe-client");
            commands.emplace_back(positionCommand +  "  --with-big-tables");
            commands.emplace_back(positionCommand +  "  --with-plugin-maria");
            commands.emplace_back(positionCommand +  "  --with-aria-tmp-tables");
            commands.emplace_back(positionCommand +  "  --without-plugin-innodb_plugin");
            commands.emplace_back(positionCommand +  "  --with-mysqld-ldflags=-static");
            commands.emplace_back(positionCommand +  "  --with-client-ldflags=-static");
            commands.emplace_back(positionCommand +  "  --with-readline");
            commands.emplace_back(positionCommand +  "  --with-ssl");
            commands.emplace_back(positionCommand +  "  --with-embedded-server");
            commands.emplace_back(positionCommand +  "  --with-libevent");
            commands.emplace_back(positionCommand +  "  --with-mysqld-ldflags=-all-static");
            commands.emplace_back(positionCommand +  "  --with-client-ldflags=-all-static");
            commands.emplace_back(positionCommand +  "  --with-zlib-dir=bundled");
            commands.emplace_back(positionCommand +  "  --enable-local-infile");
            configureStr.append(multilineCommand(commands, false));

            result += basicInstall(itemValues, configureStr);
            result += do_post_install(settings, itemValues, result);

            createProtectionWhenRequired(result, itemValues, false);
        } else {
            result = badSha256sum(itemValues);
        }
    }
    return result;
}

int install_perl6(std::map<sstr, sstr>& settings, an_itemValues& itemValues)
{
    // we need this path for running perl5 as part of the configure statement
    itemValues.perl5RunPath = settings["perl5RunPath"];
    if (itemValues.perl5RunPath.length()< 1)
    {
        sstr temp = "usr/perl/bin/";
        itemValues.perl5RunPath = joinPathParts(itemValues.rtnPath,temp);
    }

    int result = -1;
    sstr positionCommand = std::string(commandPosition, ' ');
    sstr command;
    std::vector<sstr> vec;

    set_bInstall(itemValues);
    if (itemValues.bInstall)
    {
        appendNewLogSection(itemValues.fileName_Build);
        ensureStageDirectoryExists(itemValues);
        stageSourceCodeIfNeeded(itemValues);
        itemValues.srcPathPNV = joinPathParts(itemValues.srcPath, itemValues.programNameVersion);

        bool securityCheck = check_Sha256sum(itemValues);
        if (securityCheck)
        {
            result = setupInstallDirectories(itemValues);
            sstr configureStr = "eval \"cd '" + itemValues.srcPathPNV + "'\";\n";
            std::vector<sstr> commands;
            commands.emplace_back(positionCommand + "'" + itemValues.perl5RunPath + "/perl' Configure.pl");
            commands.emplace_back(positionCommand + "--backend=moar --gen-moar --prefix='" + itemValues.usrPath + "'");
            configureStr.append(multilineCommand(commands, false));

            result += basicInstall(itemValues, configureStr);
            createProtectionWhenRequired(result, itemValues, false);
        } else {
            result = badSha256sum(itemValues);
        }
    }
    return result;
}

int install_ruby(std::map<sstr, sstr>& settings, an_itemValues& itemValues)
{
    int result = -1;
    sstr positionCommand = std::string(commandPosition, ' ');
    sstr command;
    std::vector<sstr> vec;

    set_bInstall(itemValues);
    if (itemValues.bInstall)
    {
        appendNewLogSection(itemValues.fileName_Build);
        ensureStageDirectoryExists(itemValues);
        stageSourceCodeIfNeeded(itemValues);
        itemValues.srcPathPNV = joinPathParts(itemValues.srcPath, itemValues.programNameVersion);

        bool securityCheck = check_Sha256sum(itemValues);
        if (securityCheck)
        {
            result = setupInstallDirectories(itemValues);
            sstr configureStr = "eval \"cd '" + itemValues.srcPathPNV + "'\";\n"
                       + positionCommand + "./configure --prefix='" + itemValues.usrPath + "' \\\n";
            result += basicInstall(itemValues, configureStr);
            createProtectionWhenRequired(result, itemValues, false);
        } else {
            result = badSha256sum(itemValues);
        }
    }
    return result;
}

int install_apache_step_01(std::map<sstr, sstr>& settings, an_itemValues& itemValues)
{
    int result = -1;
    sstr positionCommand = std::string(commandPosition, ' ');
    sstr command;
    std::vector<sstr> vec;

    set_bInstall(itemValues);
    if (itemValues.bInstall)
    {
        appendNewLogSection(itemValues.fileName_Build);
        ensureStageDirectoryExists(itemValues);
        stageSourceCodeIfNeeded(itemValues);
        itemValues.srcPathPNV = joinPathParts(itemValues.srcPath, itemValues.programNameVersion);

        bool securityCheck = check_Sha256sum(itemValues);
        if (securityCheck)
        {
            result = setupInstallDirectories(itemValues);

            sstr configureStr = "eval \"cd '" + itemValues.srcPathPNV + "' \";\n"
               + positionCommand + " ./configure --prefix='" + itemValues.usrPath + "' \\\n";
            result += basicInstall(itemValues, configureStr);
            createProtectionWhenRequired(result, itemValues, false);
        } else {
            result = badSha256sum(itemValues);
        }
    }
    return result;
}

int install_apache_step_02(std::map<sstr, sstr>& settings, an_itemValues& itemValues)
{
    int result = -1;
    sstr positionCommand = std::string(commandPosition, ' ');
    sstr command;
    std::vector<sstr> vec;

    set_bInstall(itemValues);

    if (itemValues.bInstall)
    {
        appendNewLogSection(itemValues.fileName_Build);
        ensureStageDirectoryExists(itemValues);
        stageSourceCodeIfNeeded(itemValues);
        itemValues.srcPathPNV = joinPathParts(itemValues.srcPath, itemValues.programNameVersion);

        bool securityCheck = check_Sha256sum(itemValues);
        if (securityCheck)
        {
            result = setupInstallDirectories(itemValues);
            sstr aprBasePath = itemValues.rtnPath + "usr/apr/bin";
            sstr configureStr = "eval \"cd '" + itemValues.srcPathPNV + "' \";\n";

            std::vector<sstr> commands;
            commands.emplace_back(positionCommand + "./configure");
            commands.emplace_back(positionCommand + "  --prefix='" + itemValues.usrPath + "'");
            commands.emplace_back(positionCommand + "  --with-apr='" + aprBasePath + "'");
            configureStr.append(multilineCommand(commands, false));

            result += basicInstall(itemValues, configureStr);

            createProtectionWhenRequired(result, itemValues, false);
        } else {
            result = badSha256sum(itemValues);
        }

    }
    return result;
}

int install_apache_step_03(std::map<sstr, sstr>& settings, an_itemValues& itemValues)
{
    int result = -1;
    sstr positionCommand = std::string(commandPosition, ' ');
    sstr command;
    std::vector<sstr> vec;

    set_bInstall(itemValues);
    if (itemValues.bInstall)
    {
        appendNewLogSection(itemValues.fileName_Build);
        ensureStageDirectoryExists(itemValues);
        stageSourceCodeIfNeeded(itemValues);
        itemValues.srcPathPNV = joinPathParts(itemValues.srcPath, itemValues.programNameVersion);

        bool securityCheck = check_Sha256sum(itemValues);
        if (securityCheck)
        {
            result = setupInstallDirectories(itemValues);
            sstr aprBasePath = itemValues.rtnPath + "usr/apr/bin";
            sstr configureStr = "eval \"cd '" + itemValues.srcPathPNV + "' \";\n";

            std::vector<sstr> commands;
            commands.emplace_back(positionCommand + "./configure");
            commands.emplace_back(positionCommand + "  --prefix='" + itemValues.usrPath + "'");
            commands.emplace_back(positionCommand + "  --with-apr='" + aprBasePath + "'");
            configureStr.append(multilineCommand(commands, false));

            result += basicInstall(itemValues, configureStr);
            createProtectionWhenRequired(result, itemValues, false);
        } else {
            result = badSha256sum(itemValues);
        }
    }
    return result;
}

int install_apache_step_04(std::map<sstr, sstr>& settings, an_itemValues& itemValues)
{
    int result = -1;
    sstr positionCommand = std::string(commandPosition, ' ');
    sstr command;
    std::vector<sstr> vec;

    set_bInstall(itemValues);
    if (itemValues.bInstall)
    {
        appendNewLogSection(itemValues.fileName_Build);
        ensureStageDirectoryExists(itemValues);
        stageSourceCodeIfNeeded(itemValues);
        itemValues.srcPathPNV = joinPathParts(itemValues.srcPath, itemValues.programNameVersion);

        bool securityCheck = check_Sha256sum(itemValues);
        if (securityCheck)
        {
            result = setupInstallDirectories(itemValues);
            sstr configureStr = "eval \"cd '" + itemValues.srcPathPNV + "' \";\n";

            std::vector<sstr> commands;
            commands.emplace_back(positionCommand + "./configure");
            commands.emplace_back(positionCommand + "  --prefix='" + itemValues.usrPath + "'");
            configureStr.append(multilineCommand(commands, false));

            result += basicInstall(itemValues, configureStr);
            createProtectionWhenRequired(result, itemValues, false);
        } else {
            result = badSha256sum(itemValues);
        }
    }
    return result;
}

int install_apache_step_05(std::map<sstr, sstr>& settings, an_itemValues& itemValues)
{
    int result = -1;
    sstr positionCommand = std::string(commandPosition, ' ');
    sstr command;
    std::vector<sstr> vec;

    set_bInstall(itemValues);
    if (itemValues.bInstall)
    {
        appendNewLogSection(itemValues.fileName_Build);
        ensureStageDirectoryExists(itemValues);
        stageSourceCodeIfNeeded(itemValues);
        itemValues.srcPathPNV = joinPathParts(itemValues.srcPath, itemValues.programNameVersion);

        bool securityCheck = check_Sha256sum(itemValues);
        if (securityCheck)
        {
            result = setupInstallDirectories(itemValues);
            sstr configureStr =    "eval \"    cd '" + itemValues.srcPathPNV + "' \";\n";
            std::vector<sstr> commands;
            commands.emplace_back(positionCommand + "./configure");
            commands.emplace_back(positionCommand + "  --prefix='" + itemValues.usrPath + "'");
            configureStr.append(multilineCommand(commands, false));

            result += basicInstall(itemValues, configureStr);
            createProtectionWhenRequired(result, itemValues, false);
        } else {
            result = badSha256sum(itemValues);
        }
    }
    return result;
}

int install_apache(std::map<sstr, sstr>& settings, an_itemValues& itemValues)
{
    int result = -1;
    sstr positionCommand = std::string(commandPosition, ' ');
    sstr command;
    std::vector<sstr> vec;

    set_bInstall(itemValues);
    if (itemValues.bInstall)
    {
        appendNewLogSection(itemValues.fileName_Build);
        ensureStageDirectoryExists(itemValues);
        stageSourceCodeIfNeeded(itemValues);
        itemValues.srcPathPNV = joinPathParts(itemValues.srcPath, itemValues.programNameVersion);

        bool securityCheck = check_Sha256sum(itemValues);
        if (securityCheck)
        {
            result = setupInstallDirectories(itemValues);
            sstr usrBasePath = itemValues.rtnPath + "usr/";
            sstr configureStr = "eval \"cd '" + itemValues.srcPathPNV + "' \";\n";

            std::vector<sstr> commands;
            commands.emplace_back(positionCommand + "./configure");
            commands.emplace_back(positionCommand + "  --prefix='" + itemValues.usrPath + "'");
            commands.emplace_back(positionCommand + "  --with-apr='" + usrBasePath + "apr'");
            commands.emplace_back(positionCommand + "  --with-apr-util='" + usrBasePath + "apr-util'");
            commands.emplace_back(positionCommand + "  --with-apr-iconv='" + usrBasePath + "apr-iconv'");
            commands.emplace_back(positionCommand + "  --with-pcre='" + usrBasePath + "pcre'");
            //
            // Note:
            //  We are NOT enabling http2 in the apache setup
            //     because of the following error
            //
            //    checking for nghttp2... checking for user-provided nghttp2 base directory... none
            //    checking for pkg-config along ... checking for nghttp2 version >= 1.2.1... FAILED
            //    configure: WARNING: nghttp2 version is too old
            //
            //  even though
            //
            //  yum install libnghttp2
            //  Loaded plugins: fastestmirror, langpacks
            //  Loading mirror speeds from cached hostfile
            //     * base: mirror.web-ster.com
            //     * epel: mirror.prgmr.com
            //     * extras: mirror.web-ster.com
            //     * updates: mirror.web-ster.com
            //  Package libnghttp2-1.31.1-1.el7.x86_64 already installed and latest version
            //  Nothing to do
            //
            //  so we have to comment out the next command
            //
            //  commands.emplace_back(positionCommand + "  --enable-http2");
            //
            commands.emplace_back(positionCommand + "  --enable-so");
            configureStr.append(multilineCommand(commands, false));

            result += basicInstall(itemValues, configureStr);
            result += do_post_install(settings, itemValues, result);

            createProtectionWhenRequired(result, itemValues, false);
        } else {
            result = badSha256sum(itemValues);
        }
    }
    return result;
}

int install_php(std::map<sstr, sstr>& settings, an_itemValues& itemValues)
{
    int result    = -1;

    sstr positionCommand = std::string(commandPosition, ' ');

    sstr compileForDebug    = settings[itemValues.programName + "->Compile_For_Debug"];
    sstr xdebug_install     = settings[itemValues.programName + "->Xdebug_Install"];
    sstr xdebug_name        = settings[itemValues.programName + "->Xdebug_Name"];
    sstr xdebug_version     = settings[itemValues.programName + "->Xdebug_Version"];
    sstr xdebug_compression = settings[itemValues.programName + "->Xdebug_Compression"];
    sstr xdebug_wget        = settings[itemValues.programName + "->Xdebug_WGET"];
    sstr xdebug_tar_options = settings[itemValues.programName + "->Xdebug_Tar_Options"];
    sstr zts_version        = settings[itemValues.programName + "->zts_version"];
    bool bCompileForDebug   = getBoolFromString(compileForDebug);
    bool bInstall_Xdebug    = getBoolFromString(xdebug_install);

    sstr command;
    std::vector<sstr> vec;
    appendNewLogSection(itemValues.fileName_Build);

    sstr xDebugProgVersion        =  xdebug_name + "-" + xdebug_version;
    sstr xDebugCompressedFileName =  xDebugProgVersion + xdebug_compression;

    sstr tmpPath = "usr/apache/bin";
    sstr apePath = joinPathParts(itemValues.rtnPath, tmpPath);

    sstr tmpFile = "apachectl";
    sstr apacheController  = joinPathWithFile(apePath, tmpFile);

         tmpFile = "mysqld";
    sstr mariadbPath = itemValues.rtnPath + "usr/mariadb/bin";
    sstr mariadbController = joinPathWithFile(mariadbPath, tmpFile);

    if ((isFileSizeGtZero(itemValues,   apacheController))
        && isFileSizeGtZero(itemValues, mariadbController)) {

        vec.clear();
        vec.emplace_back("# ");
        vec.emplace_back("# Attempt to Start/Restart Apache if possible.");
        vec.emplace_back("#   If you see an error: could not bind to address [::]:80");
        vec.emplace_back("#   It most likely means Apache is already online.");
        // ./apachectl -f /wd3/j5c/p002/etc/apache/apache.conf -k start
        vec.emplace_back("eval \"cd " + apePath + "; ./apachectl -k restart \"");
        int temp = do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
        vec.emplace_back("# ");
        if (temp == 0)
        {
            vec.emplace_back("# Restarting Apache Web Server was successful.");
        }
        else
        {
            // The first attempt failed because the server is already running,
            //   But it is probably running under this config file name, so lets try again.
            //   Even though we know it is running somewhere.
            vec.clear();
            vec.emplace_back("# ");
            vec.emplace_back("# The server appears to be online, lets see if we can restart.");
            vec.emplace_back("#     using the most likely configuration file...");
            vec.emplace_back("eval \"cd '" + apePath + "'; ./apachectl -f "
                             + itemValues.etcPath.substr(0,itemValues.etcPath.length()-4)
                             + "apache/apache.conf -k restart \"");
            temp = do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
            vec.emplace_back("# ");
            if (temp == 0) {
                vec.emplace_back("# Restarting Apache Web Server was successful.");
            } else {
                vec.emplace_back("# Skipping restart of Apache Web Server due to failure of commands.");
            }
        }

        vec.clear();
        vec.emplace_back("# ");
        vec.emplace_back("# MariaDB Database Server files appear to be installed.");
        vec.emplace_back("#   Not attempting to start mariadb. ");
        vec.emplace_back("#   MariaDB requires more setup. Follow the instructions");
        vec.emplace_back("#   in '" + itemValues.rtnPath + "Installation_Notes_" + itemValues.pathVersion + ".txt' " );
        vec.emplace_back("#   To start MariaDB." );
        do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);

        set_bInstall(itemValues);
        if (itemValues.bInstall)
        {
            appendNewLogSection(itemValues.fileName_Build);
            ensureStageDirectoryExists(itemValues);
            bool staged = stageSourceCodeIfNeeded(itemValues);

            if (!staged) {
                // If the source code was just downloaded the file name is mirror
                // instead of anything useful, so we need to rename the rename the file
                vec.clear();
                vec.emplace_back("# ");
                vec.emplace_back("# Change downloaded file name if needed.");
                vec.emplace_back(
                        "eval \"cd " + itemValues.stgPath + "; mv -f mirror '" + itemValues.fileName_Compressed + "' \"");
                do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
            }
            itemValues.srcPathPNV = joinPathParts(itemValues.srcPath, itemValues.programNameVersion);

            bool securityCheck = check_Sha256sum(itemValues);
            if (securityCheck)
            {
                result = setupInstallDirectories(itemValues);

                sstr configureStr = create_php_configuration(settings, itemValues);
                result += basicInstall(itemValues, configureStr);
                result += do_post_install(settings, itemValues, result);

                // end of code block
                createProtectionWhenRequired(result, itemValues, false);
            } else {
                result = badSha256sum(itemValues);
            }
        }
    }
    else {
        vec.clear();
        vec.emplace_back("# ");
        vec.emplace_back("# Apache Web Server and MariaDB are required ");
        vec.emplace_back("# to be installed before PHP can be installed.");
        do_command(itemValues.fileName_Build, vec, itemValues.bScriptOnly);
        result = 1;
    }
    return  result;
}

int install_poco(std::map<sstr, sstr>& settings, an_itemValues& itemValues)
{
    int result = -1;
    sstr positionCommand = std::string(commandPosition, ' ');
    sstr command;
    std::vector<sstr> vec;

    set_bInstall(itemValues);
    if (itemValues.bInstall)
    {
        appendNewLogSection(itemValues.fileName_Build);
        ensureStageDirectoryExists(itemValues);
        stageSourceCodeIfNeeded(itemValues);
        itemValues.srcPathPNV = joinPathParts(itemValues.srcPath, itemValues.programNameVersion);

        bool securityCheck = check_Sha256sum(itemValues);
        if (securityCheck)
        {
            result = setupInstallDirectories(itemValues);
            sstr configureStr = "eval \"cd '" + itemValues.srcPathPNV + "' \";\n"
                           + positionCommand + " ./configure --prefix='" + itemValues.usrPath + "' \\\n";
            result += basicInstall(itemValues, configureStr);
            createProtectionWhenRequired(result, itemValues, false);
        } else {
            result = badSha256sum(itemValues);
        }
    }
    return result;
}

int install_python(std::map<sstr, sstr>& settings, an_itemValues& itemValues)
{
    int result = -1;
    sstr positionCommand = std::string(commandPosition, ' ');
    sstr command;
    std::vector<sstr> vec;

    set_bInstall(itemValues);
    if (itemValues.bInstall)
    {
        appendNewLogSection(itemValues.fileName_Build);
        ensureStageDirectoryExists(itemValues);
        stageSourceCodeIfNeeded(itemValues);
        itemValues.srcPathPNV = joinPathParts(itemValues.srcPath, itemValues.programNameVersion);

        bool securityCheck = check_Sha256sum(itemValues);
        if (securityCheck)
        {
            result = setupInstallDirectories(itemValues);
            sstr configureStr = "eval \"cd '" + itemValues.srcPathPNV + "' \";\n"
                           + positionCommand + " ./configure --prefix='" + itemValues.usrPath + "' \\\n";
            result += basicInstall(itemValues, configureStr);
            createProtectionWhenRequired(result, itemValues, false);
        } else {
            result = badSha256sum(itemValues);
        }
    }
    return result;
}

int install_postfix(std::map<sstr, sstr>& settings, an_itemValues& itemValues)
{
    int result = -1;
    sstr positionCommand = std::string(commandPosition, ' ');
    sstr command;
    std::vector<sstr> vec;

    set_bInstall(itemValues);
    if (itemValues.bInstall)
    {
        appendNewLogSection(itemValues.fileName_Build);
        ensureStageDirectoryExists(itemValues);
        stageSourceCodeIfNeeded(itemValues);
        itemValues.srcPathPNV = joinPathParts(itemValues.srcPath, itemValues.programNameVersion);

        bool securityCheck = check_Sha256sum(itemValues);
        if (securityCheck)
        {
            result = setupInstallDirectories(itemValues);


            /*
             * Currently Installation of this is not supported...
             *   Maybe later when I know more about it.
             */
        } else {
            result = badSha256sum(itemValues);
        }
    }
    return result;
}

 int install_tcl(std::map<sstr, sstr>& settings, an_itemValues& itemValues)
{
    int result = -1;
    sstr positionCommand = std::string(commandPosition, ' ');
    sstr command;
    std::vector<sstr> vec;

    set_bInstall(itemValues);
    if (itemValues.bInstall)
    {
        appendNewLogSection(itemValues.fileName_Build);
        ensureStageDirectoryExists(itemValues);
        stageSourceCodeIfNeeded(itemValues);
        itemValues.srcPathPNV = joinPathParts(itemValues.srcPath, itemValues.programNameVersion);
        sstr installOS = get_InstallOS(itemValues.thisOSType);
        itemValues.srcPathInstallOS = joinPathParts(itemValues.srcPathPNV, installOS);

        bool securityCheck = check_Sha256sum(itemValues);
        if (securityCheck)
        {
            result = setupInstallDirectories(itemValues);
            sstr configureStr = "eval \"cd '" + itemValues.srcPathInstallOS + "' \"\n";

            std::vector<sstr> commands;
            commands.emplace_back(positionCommand + "./configure --prefix='" + itemValues.usrPath + "'");
            commands.emplace_back(positionCommand + "  --enable-threads");
            commands.emplace_back(positionCommand + "  --enable-shared");
            commands.emplace_back(positionCommand + "  --enable-symbols");
            commands.emplace_back(positionCommand + "  --enable-64bit");
            configureStr.append(multilineCommand(commands,false));
            result += basicInstall_tcl(itemValues, configureStr);
            createProtectionWhenRequired(result, itemValues, false);
        } else {
            result = badSha256sum(itemValues);
        }
    }
    return result;
}

int install_tk(std::map<sstr, sstr>& settings, an_itemValues& itemValues)
{
    sstr positionCommand = std::string(commandPosition, ' ');
    sstr tclProgramName = "tcl";
    sstr tclVersion     = settings["tcl->Version"];
    sstr tclConfigurePath = itemValues.rtnPath;
    sstr tclSrcPath = "src";
    tclConfigurePath = joinPathParts(tclConfigurePath, tclSrcPath);
    tclConfigurePath = joinPathParts(tclConfigurePath, tclProgramName);
    sstr tclProgramNameVersion = tclProgramName.append(tclVersion);
    tclConfigurePath = joinPathParts(tclConfigurePath, tclProgramNameVersion);
    sstr installOS = get_InstallOS(itemValues.thisOSType);
    tclConfigurePath = joinPathParts(tclConfigurePath, installOS);

    int result = -1;
    sstr command;
    std::vector<sstr> vec;

    set_bInstall(itemValues);
    if (itemValues.bInstall)
    {
        appendNewLogSection(itemValues.fileName_Build);
        ensureStageDirectoryExists(itemValues);
        stageSourceCodeIfNeeded(itemValues);
        itemValues.srcPathPNV       = joinPathParts(itemValues.srcPath, itemValues.programNameVersion);
        itemValues.srcPathInstallOS = joinPathParts(itemValues.srcPathPNV, installOS);

        bool securityCheck = check_Sha256sum(itemValues);
        if (securityCheck)
        {
            result = setupInstallDirectories(itemValues);
            sstr configureStr = "eval \"cd '" + itemValues.srcPathInstallOS + "' \";\n";
            std::vector<sstr> commands;
            commands.emplace_back(positionCommand + "./configure --prefix='" + itemValues.usrPath + "'");
            commands.emplace_back(positionCommand + "  --with-tcl='" + tclConfigurePath + "'");
            commands.emplace_back(positionCommand + "  --enable-threads");
            commands.emplace_back(positionCommand + "  --enable-shared");
            commands.emplace_back(positionCommand + "  --enable-symbols");
            commands.emplace_back(positionCommand + "  --enable-64bit");
            configureStr.append(multilineCommand(commands,false));
            result += basicInstall_tcl(itemValues, configureStr);
            createProtectionWhenRequired(result, itemValues, false);
        } else {
            result = badSha256sum(itemValues);
        }
    }
    return result;
}

int reportResults(an_itemValues& itemValues, int installResult)
{
    int result = 0;
    sstr positionCommand = std::string(commandPosition, ' ');
    time_t  now = get_Time();
    sstr strNow = get_Time_as_String(now);

    std::cout << " " << strNow << "Summary: # " << "Install " << itemValues.ProperName
              << "-" << itemValues.version;
    if (itemValues.step < 0)
    {
        std::cout << " : Result = " << installResult << "." << std::endl;
    }
    else
    {
        std::cout << " step " << std::setw(2) << std::setfill('0') << itemValues.step
                  << " : Result = " << installResult << std::endl;
    }
    time_t stop = get_Time();

    result += file_append_results(itemValues.fileName_Build,   itemValues, installResult, stop);
    result += file_append_results(itemValues.fileName_Results, itemValues, installResult, stop);

    print_blank_lines(2);
    return result;
};

int logFinalSettings(sstr& fileNameBuilds, std::map<sstr, sstr>& settings, sstr& programName )
{
    sstr tempProgramName = "";
    int max_set_width = 32;
    if (programName == "perl")
    {
        tempProgramName = programName + "5->";
    }
    else
    {
        tempProgramName = programName + "->";
    }
    sstr pad_string;
    sstr str_buffer;
    std::vector<sstr> generated_settings;
    std::cout << std::endl << std::endl;
    generated_settings.emplace_back("#                 Listing Settings : Values ");
    generated_settings.emplace_back("#============================================================================");
    for (auto element : settings)
    {

        if (element.first.substr(0, tempProgramName.length()) == tempProgramName) {
            auto pad_length = max_set_width - element.first.length();
            pad_string = sstr(pad_length, ' ');
            str_buffer = pad_string + element.first + " : " + element.second;
            generated_settings.push_back(str_buffer);
        }
    }
    file_write_vector_to_file(fileNameBuilds, generated_settings);
    return 0;
}

int process_section(std::map<sstr, sstr>& settings,
                    int (*pfunc)(std::map<sstr, sstr>& settings, an_itemValues& itemValues),
                    an_itemValues& itemValues )
{
    int result = -1;
    sstr searchStr = lowerCaseString(itemValues.programName);
    itemValues.programName = searchStr;

    if (itemValues.programName != "dependencies") {
        searchStr = itemValues.programNameVersion;
    } else {
        searchStr = "Dependencies";
    }

    bool sectionLoaded = prior_Results(itemValues.fileName_Results, searchStr);
    if (!sectionLoaded)
    {
        appendNewLogSection(itemValues.fileName_Build);
        logFinalSettings(itemValues.fileName_Build, settings, itemValues.programName);
        result = pfunc(settings, itemValues);
        reportResults( itemValues, result);
    }
    else
    {
        section_already_loaded(itemValues.programName, itemValues.version);
    }
    return result;
}

bool set_settings(std::map<sstr,sstr>& settings, an_itemValues& itemValues )
{
    // In order for future knowledge of using if statements with these variables
    //    I am providing these guaranties
    // programName --> will be guarantied to be: lowercase
    // ProperName -->  will be guarantied to be: the first letter is capital, all others lowercase
       itemValues.programName = lowerCaseString(itemValues.programName);
       itemValues.ProperName  = getProperNameFromString(itemValues.programName);
    //
    //

    itemValues.fileName_Protection = getProtectedFileName(itemValues.programName);

    sstr skip  = settings[itemValues.programName + "->Skip"];
    if (itemValues.programName == "perl")
    {
        skip  = settings[itemValues.programName + "5->Skip"];
    }
    bool bSkip = getBoolFromString(skip);
    if (!bSkip) {
        itemValues.bSkip = bSkip;

        sstr scriptOnly = "";
        sstr doTests    = "";
        sstr debugOnly  = "";
        sstr thisOS     = "";
        sstr version    = "";



        scriptOnly    = settings[itemValues.programName + "->Script_Only"];
        doTests       = settings[itemValues.programName + "->Do_Tests"];
        debugOnly     = settings[itemValues.programName + "->Debug_Only"];

        sstr stgPath = joinPathParts(itemValues.cpyStgPath, itemValues.programName);
        sstr temp    = settings[itemValues.programName + "->Debug_Level"];

        // we don't want to throw any exceptions,
        //    so make it safe to convert to int
        sstr debugLevel = getDigitsInStringAsString(temp);
        if (debugLevel.length() == 0) {

            // this means run everything
            itemValues.debugLevel = 7;

        } else {

            // else accept user input
            itemValues.debugLevel = std::stoi(debugLevel);
        }
        itemValues.compression  = settings[itemValues.programName + "->Compression"];
        itemValues.version      = settings[itemValues.programName + "->Version"];
        itemValues.getPath      = settings[itemValues.programName + "->WGET"];
        itemValues.sha256sum_Config = settings[itemValues.programName + "->Sha256sum"];


        if (itemValues.programName == "perl")
        {
            // We have to override these values because perl  and perl5 are the same thing
            //   But we don't want to confuse it with   perl6 because that is a totally different by similar language
            //   We are adding 5 or 6 to match the perl version
            skip                    = settings[itemValues.programName + "5->Skip"];
            itemValues.compression  = settings[itemValues.programName + "5->Compression"];
            debugOnly               = settings[itemValues.programName + "5->Debug_Only"];
            doTests                 = settings[itemValues.programName + "5->Do_Tests"];
            scriptOnly              = settings[itemValues.programName + "5->Script_Only"];
            itemValues.version      = settings[itemValues.programName + "5->Version"];
            itemValues.getPath      = settings[itemValues.programName + "5->WGET"];
            itemValues.sha256sum_Config = settings[itemValues.programName + "5->Sha256sum"];
        }

        itemValues.bSkip         = getBoolFromString(skip);
        itemValues.bScriptOnly   = getBoolFromString(scriptOnly);
        itemValues.bDoTests      = getBoolFromString(doTests);
        itemValues.bDebug        = getBoolFromString(debugOnly);
        itemValues.bInstall      = true;

        itemValues.programNameVersion = itemValues.programName;
        itemValues.programNameVersion.append("-");
        itemValues.programNameVersion.append(itemValues.version);


        //This section is to re-write the program name
        //     for programNameVersion strings
        if (itemValues.programName == "apache")
        {
            itemValues.programNameVersion = "httpd-";
            itemValues.programNameVersion.append(itemValues.version);
        }

        if (itemValues.programName == "perl6")
        {
            itemValues.programNameVersion = "rakudo-star-";
            itemValues.programNameVersion.append(itemValues.version);
        }

        if (itemValues.programName == "python")
        {
            itemValues.programNameVersion = "Python-";
            itemValues.programNameVersion.append(itemValues.version);
        }

        if ((itemValues.programName == "tcl") || (itemValues.programName == "tk"))
        {
            itemValues.programNameVersion = itemValues.programName;
            itemValues.programNameVersion.append(itemValues.version);
        }

        itemValues.stgPath             = joinPathParts(itemValues.cpyStgPath,itemValues.programName);
        itemValues.fileName_Compressed =  itemValues.programNameVersion + itemValues.compression;

        if ((itemValues.programName == "tcl") || (itemValues.programName == "tk"))
        {
            itemValues.fileName_Compressed =  itemValues.programNameVersion;
            itemValues.fileName_Compressed.append("-src");
            itemValues.fileName_Compressed.append(itemValues.compression);
        }

        itemValues.sha256sum_FileName = joinPathWithFile(itemValues.stgPath, itemValues.fileName_Compressed);
        //end of section

        itemValues.fileName_Staged     =  joinPathWithFile(itemValues.stgPath, itemValues.fileName_Compressed);

        sstr tmpPath1 = "src";
        sstr tmpPath2 = joinPathParts(itemValues.rtnPath, tmpPath1);
             tmpPath1 = itemValues.programName;
        itemValues.srcPath = joinPathParts(tmpPath2, tmpPath1);

             tmpPath1 = "xxx";
        sstr xxxPath = joinPathParts(itemValues.rtnPath, tmpPath1);
        itemValues.bldPath = get_xxx_Path(xxxPath, "bld");
        itemValues.bldPath.append(itemValues.programName);
        itemValues.bldPath.append("/");
        itemValues.etcPath = get_xxx_Path(xxxPath, "etc");
        itemValues.etcPath.append(itemValues.programName);
        itemValues.etcPath.append("/");

        itemValues.usrPath = get_xxx_Path(xxxPath, "usr");
        itemValues.usrPath.append(itemValues.programName);
        itemValues.usrPath.append("/");
    }
    return bSkip;
}

/*
 * Main Starts Here
 *
 */

int main() {


    struct programs
    {
        an_itemValues itemValues;
        int  step;
        int (*funptr)(std::map<sstr, sstr> &settings, an_itemValues& itemValues);
    } program;

    OS_type thisOSType;
    Mac_PM mpm;
    sstr prefix;
    sstr protectModeText;
    time_t programStart;
    time_t programStop;
    programStart = get_Time();

    // get settings from file
    std::map<sstr, sstr> settings;
    sstr fileSettings = "./Install_Settings.cfg";
    settings = getProgramSettings(fileSettings);
    std::vector<sstr> generated_settings;

    // put settings into program variables
    std::cout << "Putting settings into program variables..." << std::endl;
    sstr pricomy = "/usr/local/j5c";
    sstr company    = settings[KEY_COMPANY_NAME];

    sstr usePrefix  = settings[KEY_TOUSE_PREFIX];
    bool bUsePrefix = getBoolFromString(usePrefix);
    if (bUsePrefix) {
        prefix = settings[KEY_DEFLT_PREFIX];
        program.itemValues.company = joinPathParts(prefix, company);
    } else {
        sstr beginPath = "/";
        program.itemValues.company = joinPathParts(beginPath, company);
    }

    sstr pVersion = settings[KEY_PATH_VERSION];
    pVersion = getValid_pVersion(pVersion);
    program.itemValues.pathVersion = "p" + pVersion;

    // assign a default; and change if not correct...
    thisOSType = OS_type::RedHat;
    mpm = Mac_PM::No_Selection_Made;
    sstr theOStext = settings[KEY_AN_OS_SYSTEM];
    theOStext = lowerCaseString(theOStext);
    if (theOStext == "cent os")    { thisOSType = OS_type::CentOS;      }
    if (theOStext == "cent_os")    { thisOSType = OS_type::CentOS;      }
    if (theOStext == "centos")     { thisOSType = OS_type::CentOS;      }
    if (theOStext == "fedora")     { thisOSType = OS_type::Fedora;      }
    if (theOStext == "linux mint") { thisOSType = OS_type::Linux_Mint;  }
    if (theOStext == "linux_mint") { thisOSType = OS_type::Linux_Mint;  }
    if (theOStext == "linuxmint")  { thisOSType = OS_type::Linux_Mint;  }
    if (theOStext == "red hat")    { thisOSType = OS_type::RedHat;      }
    if (theOStext == "red_hat")    { thisOSType = OS_type::RedHat;      }
    if (theOStext == "redhat")     { thisOSType = OS_type::RedHat;      }
    if (theOStext == "OSX") {
        thisOSType = OS_type::MaxOSX;
        // TODO add a setting for Homebrew or Mac Ports
        // there currently is no setting for this yet so we will pick the default for now.
        mpm = Mac_PM::MacPorts;
    }

    program.itemValues.thisOSType = thisOSType;

    protectModeText = settings[KEY_PROTECT_MODE];
    program.itemValues.bProtectMode = getBoolFromString(protectModeText);

    sstr runDepds = settings[KEY_RUN_DEPENDCS];
    bool runDependencies = getBoolFromString(runDepds);

    bool sectionLoaded;

    sstr temp = "p" + pVersion;
    program.itemValues.rtnPath = joinPathParts(program.itemValues.company, temp);
    temp = STG_NAME;
    program.itemValues.cpyStgPath = joinPathParts(program.itemValues.company, temp);
    temp = "xxx";
    sstr xxxPath  = joinPathParts(program.itemValues.rtnPath, temp);
    sstr programName = "Dependencies";
    program.itemValues.programName = programName;
    program.itemValues.ProperName  = getProperNameFromString(program.itemValues.programName);
    program.itemValues.tlsPath = get_xxx_Path(xxxPath, "tls");
    program.itemValues.wwwPath = get_xxx_Path(xxxPath, "www");

    ensure_Directory_exists1(program.itemValues.rtnPath);
    ensure_Directory_exists1(program.itemValues.cpyStgPath);
    ensure_Directory_exists1(program.itemValues.tlsPath);
    ensure_Directory_exists1(program.itemValues.wwwPath);

    sstr getPath = "xxx";
    sstr buildVersion;

    temp = "Installation_Builds_p" + pVersion + ".txt";
    program.itemValues.fileName_Build = joinPathWithFile(program.itemValues.rtnPath, temp);

    temp = "Installation_Notes_p" + pVersion + ".txt";
    program.itemValues.fileName_Notes = joinPathWithFile(program.itemValues.rtnPath, temp);

    temp = "Installation_Results_p" + pVersion + ".txt";
    program.itemValues.fileName_Results = joinPathWithFile(program.itemValues.rtnPath, temp);

    std::vector<sstr> vec;
    program.itemValues.step             = -1;
    program.itemValues.itemStartTime    = get_Time();


    create_file(program.itemValues.fileName_Build);
    do_command(program.itemValues.fileName_Build, vec, false);

    if (!(isFileSizeGtZero(program.itemValues, program.itemValues.fileName_Notes))) {
        create_file(program.itemValues.fileName_Notes);
    } else {
        ensure_file(program.itemValues.fileName_Notes);
    }

    if (!(isFileSizeGtZero(program.itemValues, program.itemValues.fileName_Results))) {
        create_file(program.itemValues.fileName_Results);
    } else {
        ensure_file(program.itemValues.fileName_Results);
    }

    int result = 0;
    sectionLoaded = prior_Results(program.itemValues.fileName_Results, programName);
    if (!sectionLoaded) {
        bool bScriptOnly = !runDependencies;
        if ((thisOSType == OS_type::RedHat) || (thisOSType == OS_type::CentOS) || (thisOSType == OS_type::Fedora)) {
            install_yum_required_dependencies(program.itemValues.fileName_Build, program.itemValues.programName, bScriptOnly);
            print_blank_lines(2);
        }

        if (thisOSType == OS_type::Linux_Mint) {
            install_apt_required_dependencies(program.itemValues.fileName_Build, programName, bScriptOnly);
        }

        if (thisOSType == OS_type::MaxOSX) {
            install_mac_required_dependencies(mpm);
        }
        reportResults(program.itemValues, result);
    } else {
        sstr version = "";
        section_already_loaded(programName, version);
    }

    std::vector<programs> progVector;
    program.itemValues.programName = "cmake";
    program.itemValues.step        = -1;
    program.funptr = &install_cmake;
    progVector.emplace_back(program);

    program.itemValues.programName = "libzip";
    program.itemValues.step        = -1;
    program.funptr = &install_libzip;
    progVector.emplace_back(program);

    program.itemValues.programName = "perl";
    program.itemValues.step        = -1;
    program.funptr = &install_perl5;
    progVector.emplace_back(program);

    program.itemValues.programName = "openssl";
    program.itemValues.step        = -1;
    program.funptr = &install_openssl;
    progVector.emplace_back(program);

    program.itemValues.programName = "mariadb";
    program.itemValues.step        = -1;
    program.funptr = &install_mariadb;
    progVector.emplace_back(program);

    program.itemValues.programName = "apr";
    program.itemValues.step        = 1;
    program.funptr = &install_apache_step_01;
    progVector.emplace_back(program);

    program.itemValues.programName = "apr-util";
    program.itemValues.step        = 2;
    program.funptr = &install_apache_step_02;
    progVector.emplace_back(program);

    program.itemValues.programName = "apr-iconv";
    program.itemValues.step        = 3;
    program.funptr = &install_apache_step_03;
    progVector.emplace_back(program);

    program.itemValues.programName = "pcre";
    program.itemValues.step        = 4;
    program.funptr = &install_apache_step_04;
    progVector.emplace_back(program);

    program.itemValues.programName = "pcre2";
    program.itemValues.step        = 5;
    program.funptr = &install_apache_step_05;
    progVector.emplace_back(program);

    program.itemValues.programName = "apache";
    program.itemValues.step        = -1;
    program.funptr = &install_apache;
    progVector.emplace_back(program);

    program.itemValues.programName = "perl6";
    program.itemValues.step        = -1;
    program.funptr = &install_perl6;
    progVector.emplace_back(program);

    program.itemValues.programName = "php";
    program.itemValues.step        = -1;
    program.funptr = &install_php;
    progVector.emplace_back(program);

    program.itemValues.programName = "poco";
    program.itemValues.step        = -1;
    program.funptr = &install_poco;
    progVector.emplace_back(program);

    program.itemValues.programName = "postfix";
    program.itemValues.step        = -1;
    program.funptr = &install_postfix;
    progVector.emplace_back(program);

    program.itemValues.programName = "python";
    program.itemValues.step        = -1;
    program.funptr = &install_python;
    progVector.emplace_back(program);

    program.itemValues.programName = "ruby";
    program.itemValues.step        = -1;
    program.funptr = &install_ruby;
    progVector.emplace_back(program);

    program.itemValues.programName = "tcl";
    program.itemValues.step        =  1;
    program.funptr = &install_tcl;
    progVector.emplace_back(program);

    program.itemValues.programName = "tk";
    program.itemValues.step        =  2;
    program.funptr = &install_tk;
    progVector.emplace_back(program);
    program.itemValues.thisOSType   = thisOSType;

    //function pointer declaration
    int (*funptr)(std::map<sstr, sstr> &settings, an_itemValues& itemValues);

    bool anyInstalled = false;

    ensureMariaDB_UserAndGroupExist(program.itemValues);
    ensureApacheUserAndGroupExists(program.itemValues);

    for( auto& it: progVector )
    {
        result = -1;
        it.itemValues.itemStartTime = get_Time();
        bool skip = set_settings(settings, it.itemValues);

        if (!skip) {
            funptr = (it.funptr);
            result = process_section(settings, funptr, it.itemValues);
            if (result > -1) { anyInstalled = true; }
        }
    }

    if (anyInstalled) {
        programStop = get_Time();
        sstr end = "End of Program";
        file_append_line(program.itemValues.fileName_Build,   end, programStop, programStart);
        file_append_line(program.itemValues.fileName_Results, end, programStop, programStart);
    }
    return 0;
}
