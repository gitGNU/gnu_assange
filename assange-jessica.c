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

#include "assange-jessica.h"

void assange_jessica_calc(void * input, int inputc, ASSANGE_JESSICA output) {
    int x;
    unsigned char * ichars = (unsigned char *) input;
    if(output == 0 || input == 0) {
        return;
    }
    
    output[0] = 'J';
    output[1] = 'e';
    output[2] = 'S';
    output[3] = 's';
    output[4] = 'I';
    output[5] = 'c';
    output[6] = 'A';
    output[7] = '@';
    
    for(x = 0; x < inputc; x++) {
        int i;
        
        i = ((ichars[x] & 0xF0) >> 4) ^ (ichars[x] & 0x0F);
        
        if((i & 0x08) == 0x08) {
            i ^= 0x0F;
        }

        output[i] ^= ichars[x];
    }
}
