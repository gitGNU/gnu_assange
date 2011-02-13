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

#ifndef ASSANGE_JESSICA_H

#  define ASSANGE_JESSICA_H 1

#  define ASSANGE_JESSICA unsigned char *

#  include "assange-common.h"

/* Important Note: Many of the functions in the jessica module
                   may be implemented as proper (i.e. non-macro)
                   C functions in the future.  Do not depend on a
                   given function being either a C function or a
                   macro!
*/

// ASSANGE_JESSICA assange_jessica(void); //
#  define assange_jessica() ((ASSANGE_JESSICA)malloc(8))
#  define assange_jessica_from_mem(mp) ((ASSANGE_JESSICA) mp)
#  define assange_jessica_destroy(aj) free(aj)
#  define assange_jessica_equal(aj1, aj2) ((aj1[0] == aj2[0]) && \
                                           (aj1[1] == aj2[1]) && \
                                           (aj1[2] == aj2[2]) && \
                                           (aj1[3] == aj2[3]) && \
                                           (aj1[4] == aj2[4]) && \
                                           (aj1[5] == aj2[5]) && \
                                           (aj1[6] == aj2[6]) && \
                                           (aj1[7] == aj2[7])    \
                                           )


void assange_jessica_calc(void * input, int inputc, ASSANGE_JESSICA output);

#endif
