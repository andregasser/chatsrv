/******************************************************************************
 *    Copyright 2012 André Gasser
 *
 *    This file is part of Dnsmap.
 *
 *    Dnsmap is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    Dnsmap is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Dnsmap.  If not, see <http://www.gnu.org/licenses/>.
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include "log.h"

/* static reduces this variables scope to file scope.
 * This variable is global, but only throughout this file.
 */
static int global_loglevel = LOG_INFO;

void logline(int loglevel, const char* format, ...) 
{
	va_list args;
		
	if (loglevel <= global_loglevel)
	{
		char timestr[20];
		char *loginfo;
		struct tm *ptr;
		time_t lt;

		lt = time(NULL);
		ptr = localtime(&lt);
		strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", ptr);

		switch (loglevel)
		{
			case LOG_ERROR: loginfo = "E"; break;
			case LOG_INFO: loginfo = "I"; break;
			case LOG_DEBUG: loginfo = "D"; break;
			default: loginfo = "I"; 
		}

		printf("[%s %s] ", timestr, loginfo);
		va_start(args, format);
		vfprintf(stdout, format, args);
		va_end(args);
		printf("\n");
	}
}

/*
 * This function sets the loglevel of the logger.
 */
void set_loglevel(int loglevel)
{
	if ((loglevel == LOG_ERROR) || (loglevel == LOG_INFO) || (loglevel == LOG_DEBUG))
	{
		global_loglevel = loglevel;
	}
}

