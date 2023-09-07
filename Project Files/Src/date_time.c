/**
 * @file date_time.c
 * @brief Date and time management
 *
 * @section License
 *
 * Copyright (C) 2010-2017 Oryx Embedded SARL. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 1.7.6
 **/

//Dependencies
#include <stdio.h>
#include <string.h>
#include "date_time.h"

#if defined(_WIN32)
   #include <time.h>
#endif


/**
 * @brief Format date
 * @param[in] date Pointer to a structure representing the date
 * @param[out] str NULL-terminated string representing the specified date
 * @return Pointer to the formatted string
 **/
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif


/**
 * @brief Convert Unix timestamp to date
 * @param[in] t Unix timestamp
 * @param[out] date Pointer to a structure representing the date and time
 **/

void convertUnixTimeToDate(systime_t t, DateTime_t *date)
{
   uint32_t a;
   uint32_t b;
   uint32_t c;
   uint32_t d;
   uint32_t e;
   uint32_t f;

   //Negative Unix time values are not supported
   if(t < 1) t = 0;

   //Clear milliseconds
   date->milliseconds = 0;

   //Retrieve hours, minutes and seconds
   date->seconds = t % 60;
   t /= 60;
   date->minutes = t % 60;
   t /= 60;
   date->hours = t % 24;
   t /= 24;

   //Convert Unix time to date
   a = (uint32_t) ((4 * t + 102032) / 146097 + 15);
   b = (uint32_t) (t + 2442113 + a - (a / 4));
   c = (20 * b - 2442) / 7305;
   d = b - 365 * c - (c / 4);
   e = d * 1000 / 30601;
   f = d - e * 30 - e * 601 / 1000;

   //January and February are counted as months 13 and 14 of the previous year
   if(e <= 13)
   {
      c -= 4716;
      e -= 1;
   }
   else
   {
      c -= 4715;
      e -= 13;
   }

   //Retrieve year, month and day
   date->year   = c;
   date->month = e;
   date->day   = f;
}


/**
 * @brief Convert date to Unix timestamp
 * @param[in] date Pointer to a structure representing the date and time
 * @return Unix timestamp
 **/

systime_t convertDateToUnixTime(const DateTime_t *date)
{
   uint32_t		y;
   uint32_t 	m;
   uint32_t 	d;
   uint32_t 	t;

   //Year
   y = date->year;
   //Month of year
   m = date->month;
   //Day of month
   d = date->day;

   //January and February are counted as months 13 and 14 of the previous year
   if(m <= 2)
   {
      m += 12;
      y -= 1;
   }

   //Convert years to days
   t = (365 * y) + (y / 4) - (y / 100) + (y / 400);
   //Convert months to days
   t += (30 * m) + (3 * (m + 1) / 5) + d;
   //Unix time starts on January 1st, 1970
   t -= 719561;
   //Convert days to seconds
   t *= 86400;
   //Add hours, minutes and seconds
   t += (3600 * date->hours) + (60 * date->minutes) + date->seconds;

   //Return Unix time
   return t;
}
