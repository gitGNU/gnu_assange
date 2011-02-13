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

#ifndef ASSANGE_COMMON_H

#  define ASSANGE_COMMON_H 1

#  define ASSANGE_BOOL int
#  define ASSANGE_TRUE 1
#  define ASSANGE_FALSE 0

#  define ASSANGE_BYTE unsigned char

#  define ASSANGE_SESSC 16384

#  define ASSANGE_STATE int
#  define ASSANGE_STATE_FREE       -1
#  define ASSANGE_STATE_RECV_PATH  2
#  define ASSANGE_STATE_RECV_HDRS  3
#  define ASSANGE_STATE_SEND       16

#  define ASSANGE_DNLSTATE     int
#  define ASSANGE_DNLSTATE_BEG 0x00
#  define ASSANGE_DNLSTATE_R   0x01
#  define ASSANGE_DNLSTATE_RN  0x02
#  define ASSANGE_DNLSTATE_RNR 0x03

struct assange_session {
    ASSANGE_STATE as_state;
    int as_state_dnl;
    int as_socket;
    
    char * as_in;
    int as_in_index;
    
    char * as_out;
    int as_out_size;
    int as_out_index;
};

#  define ASSANGE_SENDSHARE 1024
#  define ASSANGE_RECVSHARE 512
#  define ASSANGE_MEMSHARE  1024
#  define ASSANGE_IMGSHARE  262144000

#  define ASSANGE_READ_U4(mp) ((((*((ASSANGE_BYTE *)mp)) & 0x7F) << 24) | ((*((ASSANGE_BYTE *)(mp + 1))) << 16) | ((*((ASSANGE_BYTE *)(mp + 2))) << 8) | ((*((ASSANGE_BYTE *)(mp + 3)))))
#  define ASSANGE_READ_U3(mp) (((*((ASSANGE_BYTE *)(mp + 1))) << 16) | ((*((ASSANGE_BYTE *)(mp + 2))) << 8) | ((*((ASSANGE_BYTE *)(mp + 3)))))
#  define ASSANGE_READ_U2(mp) (((*((ASSANGE_BYTE *)(mp + 2))) << 8) | ((*((ASSANGE_BYTE *)(mp + 3)))))
#  define ASSANGE_READ_B10(mp) ((((*((ASSANGE_BYTE *)(mp + 2))) << 8) | ((*((ASSANGE_BYTE *)(mp + 3))))) & 0x3F)

#endif
