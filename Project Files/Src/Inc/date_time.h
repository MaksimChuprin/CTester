/**
 * @file date_time.h
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

#ifndef _DATE_TIME_H
#define _DATE_TIME_H

//Dependencies


/**
 * @brief Date and time representation
 **/

typedef struct
{
   uint16_t year;
   uint16_t month;
   uint16_t day;
   uint16_t dayOfWeek;
   uint16_t hours;
   uint16_t minutes;
   uint16_t seconds;
   uint16_t milliseconds;
} DateTime_t;

typedef uint32_t	systime_t;


void 		convertUnixTimeToDate(uint32_t t, DateTime_t *date);
uint32_t 	convertDateToUnixTime(const DateTime_t *date);
uint8_t 	computeDayOfWeek(uint16_t y, uint8_t m, uint8_t d);

#endif
