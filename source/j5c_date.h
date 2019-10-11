//
// j5c_Date Class Header File
//
// Copyright (C) 2017  Jay A. Carlson of J5C Marketing LLC.
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


#ifndef DATE_DATE_H
#define DATE_DATE_H

#include <limits>
#include <iosfwd>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <ctime>
#include <chrono>
#include <ratio>

namespace J5C_DSL_Code {

    // leap getYear is adjusted in code      Not Used  Jan  Feb  Mar  Apr  May  Jun  Jul  Aug  Sep  Oct  Nov  Dec
    static const int numberOfDaysInMonth[13]     = {0,  31,  28,  31,  30,  31,  30,  31,  31,  30,  31,  30,  31};

    //                                       Not Used  Jan  Feb  Mar  Apr  May  Jun  Jul  Aug  Sep  Oct  Nov  Dec
    static const int numberOfDaysBeforeMonth[13] = {0,   0,  31,  59,  90, 120, 151, 181, 212, 243, 273, 304, 334};


    static const int numberOfDaysBefore_forDayOfQuarter[13] = {0, 0, 31, 59, 0, 30, 61, 0, 31, 62, 0, 31, 61};


    static const int firstDayOfYear[400] =
            { -1,
              1, 2, 3, 4, 6, 0, 1, 2, 4, 5, 6, 0, 2, 3,
              4, 5, 0, 1, 2, 3, 5, 6, 0, 1, 3, 4, 5, 6,
              1, 2, 3, 4, 6, 0, 1, 2, 4, 5, 6, 0, 2, 3,
              4, 5, 0, 1, 2, 3, 5, 6, 0, 1, 3, 4, 5, 6,
              1, 2, 3, 4, 6, 0, 1, 2, 4, 5, 6, 0, 2, 3,
              4, 5, 0, 1, 2, 3, 5, 6, 0, 1, 3, 4, 5, 6,
              1, 2, 3, 4, 6, 0, 1, 2, 4, 5, 6, 0, 2, 3,
              4, 5, 6, 0, 1, 2, 4, 5, 6, 0, 2, 3, 4, 5,
              0, 1, 2, 3, 5, 6, 0, 1, 3, 4, 5, 6, 1, 2,
              3, 4, 6, 0, 1, 2, 4, 5, 6, 0, 2, 3, 4, 5,
              0, 1, 2, 3, 5, 6, 0, 1, 3, 4, 5, 6, 1, 2,
              3, 4, 6, 0, 1, 2, 4, 5, 6, 0, 2, 3, 4, 5,
              0, 1, 2, 3, 5, 6, 0, 1, 3, 4, 5, 6, 1, 2,
              3, 4, 6, 0, 1, 2, 4, 5, 6, 0, 2, 3, 4, 5,
              0, 1, 2, 3, 4, 5, 6, 0, 2, 3, 4, 5, 0, 1,
              2, 3, 5, 6, 0, 1, 3, 4, 5, 6, 1, 2, 3, 4,
              6, 0, 1, 2, 4, 5, 6, 0, 2, 3, 4, 5, 0, 1,
              2, 3, 5, 6, 0, 1, 3, 4, 5, 6, 1, 2, 3, 4,
              6, 0, 1, 2, 4, 5, 6, 0, 2, 3, 4, 5, 0, 1,
              2, 3, 5, 6, 0, 1, 3, 4, 5, 6, 1, 2, 3, 4,
              6, 0, 1, 2, 4, 5, 6, 0, 2, 3, 4, 5, 0, 1,
              2, 3, 5, 6, 0, 1, 2, 3, 4, 5, 0, 1, 2, 3,
              5, 6, 0, 1, 3, 4, 5, 6, 1, 2, 3, 4, 6, 0,
              1, 2, 4, 5, 6, 0, 2, 3, 4, 5, 0, 1, 2, 3,
              5, 6, 0, 1, 3, 4, 5, 6, 1, 2, 3, 4, 6, 0,
              1, 2, 4, 5, 6, 0, 2, 3, 4, 5, 0, 1, 2, 3,
              5, 6, 0, 1, 3, 4, 5, 6, 1, 2, 3, 4, 6, 0,
              1, 2, 4, 5, 6, 0, 2, 3, 4, 5, 0, 1, 2, 3,
              5, 6, 0, 1, 3, 4, 5
            };



    class j5c_Date {

        friend std::ostream& operator<<(std::ostream& out, const j5c_Date& d);
        friend std::istream& operator>>(std::istream& ins,       j5c_Date& d);

    private:

        //
        // DOW definitions
        //
        // 0 = Sunday (The Lord's Day)
        // 1 = Monday
        // . . .
        //
        // Month Definitions
        //  01 - Jan
        //  02 - Feb
        //  . . .


        // firstDayOfYear[0] is not used ie -1
        // Note: valid years in this class are limited to 1 - 9999 AD (CE)

        // Time Line Abbreviations in order...
        // Past Infinity -> 1 BC (BCE) -> (no zero getYear) -> 1 AD (CE) -> (Present Date) -> Future Infinity


    private:
        int LeapYearsSinceYear0001(int year, int month) const noexcept;
        static const int MIN_YEAR = 1;
        static const int MAX_YEAR = 9999;
        //std::string DOWT = "Invalid DOW";
        void cout_InvalidDate() const noexcept;
        int daysSinceYear0001Day001(int year, int month, int day) const noexcept;
        bool isLeapYear(int year) const noexcept;
        j5c_Date internal_addDays(int days)         const noexcept;
        j5c_Date internal_subDays(int days)         const noexcept;
        std::string padright(int width, int value)  const noexcept;

    protected:
        int m_year;
        int m_month;
        int m_day;


    public:
        //constructors
        explicit j5c_Date()                                       noexcept;
        explicit j5c_Date(int year, int month, int day)           noexcept
                : m_year(year) , m_month(month), m_day(day) { };

        explicit j5c_Date(int year, int dayOfTheYear)             noexcept;
        //constructor helpers / methods
        void set_y_d(int year, int dayOfTheYear)                  noexcept;

        j5c_Date& operator=(const j5c_Date& date);

        //virtual destructor
        virtual ~j5c_Date() = default;


        bool isValid()                              const noexcept;
        bool isLeapYear()                           const noexcept;
        int getAge()                                const noexcept;
        int getFirstDayOfYear()                     const noexcept;
        int getDayOfWeek()                          const noexcept;
        int getDayOfTheYear()                       const noexcept;
        int getDayOfTheQuarter()                    const noexcept;
        int getQuarter()                            const noexcept;
        int getDaysDiff(const j5c_Date& dt2)        const noexcept;
        std::string getDayText(uint forcedLength)   const noexcept;
        std::string strDate()                       const noexcept;

        j5c_Date getNext_Date()                     const noexcept;
        j5c_Date getPriorDate()                     const noexcept;
        // add days can accept negative numbers
        j5c_Date add_Days(int days)                 const noexcept;

        const bool operator==(const j5c_Date &)     const noexcept;
        const bool operator!=(const j5c_Date &)     const noexcept;
        const bool operator<( const j5c_Date &)     const noexcept;
        const bool operator>( const j5c_Date &)     const noexcept;
        const bool operator<=(const j5c_Date &)     const noexcept;
        const bool operator>=(const j5c_Date &)     const noexcept;

        const j5c_Date& operator++()        noexcept;   // prefix
        const j5c_Date& operator--()        noexcept;   // prefix
        const j5c_Date  operator++(int)     noexcept;   // postfix
        const j5c_Date  operator--(int)     noexcept;   // postfix


        //too simple for tests...
        int  getDay()   const noexcept { return m_day;   }
        int  getMonth() const noexcept { return m_month; }
        int  getYear()  const noexcept { return m_year;  }
        void setDay  (int day)   noexcept  { m_day   = day;   }
        void setMonth(int month) noexcept  { m_month = month; }
        void setYear (int year)  noexcept  { m_year  = year;  }

    };

}


#endif //DATE_DATE_H
