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

#ifndef LOG_H
#define LOG_H

#define LOG_ERROR 1
#define LOG_INFO  2
#define LOG_DEBUG 3

void logline(int loglevel, const char* format, ...);
void set_loglevel(int loglevel);

#endif /* LOG_H */
