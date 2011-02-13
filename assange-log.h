/*  Assange Webserver: The webserver for sysadmins that are having a really, really bad day.
    Copyright (c) 2011 William Demchick

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ASSANGE_LOG_H

#  define ASSANGE_LOG_H 1

#  include <stdio.h>

#  define assange_log(s) fputs(s, stdout)

#  define assange_log_error(m) do { assange_log("[!] "); assange_log(m); assange_log("\n"); } while(0)
#  define assange_log_info(m)  do { assange_log("[i] "); assange_log(m); assange_log("\n"); } while(0)
#  define assange_log_warn(m)  do { assange_log("[w] "); assange_log(m); assange_log("\n"); } while(0)

#endif
