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

#include "assange-common.h"
#include "assange-log.h"
#include "assange-jessica.h"

#include <unistd.h>
#include <stdio.h>
#include <netinet/ip.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>

int assange_occ = 0;
int assange_port = 8080;
int assange_socket = -1;
char * assange_image = 0;
int assange_image_size = 0;
int assange_image_hashc = 0;
struct assange_session * assange_sessions;

void assange_sig(int);
void assange_pump(void);
void assange_scan(void);

int main(int argc, char ** argv) {
    assange_log_info("Welcome to ASSANGE!");
    assange_log_info("The webserver for sysadmins that are having a really, really bad day.");
    assange_log_info("Copyright (c) 2011 William Demchick");
    assange_log_info("");
    assange_log_info("Version: 0.4 BETA");
    assange_log_info("Modules: <core>, jessica");
    assange_log("\n\n");
    
    if(signal(SIGTERM, assange_sig) == SIG_ERR || signal(SIGINT, assange_sig) == SIG_ERR) {
        return 8;
    }

    if(argc < 2) {
        assange_log_error("Image not provided.");
        return 7;
    }
    else if(argc > 2) {
        assange_log_warn("Extra arguments ignored.");
    }

    {
        FILE * file;
        struct stat img_stat;

        assange_log_info("Attempting to retrieve image from file...");

        if(stat(argv[1], &img_stat) != 0) {
            assange_log_error("Cannot load image: file props cannot be obtained.");
            return 6;
        }
        
        if(img_stat.st_size > ASSANGE_IMGSHARE) {
            assange_log_error("Cannot load image: image too large.");
            return 6;
        }

        assange_image = (char *)malloc(img_stat.st_size);
        
        if(assange_image == 0) {
            assange_log_error("Cannot load image: memory cannot be allocated.");
            return 6;
        }
        
        file = fopen(argv[1], "rb");
        
        if(file == 0) {
            assange_log_error("Cannot load image: file cannot be opened.");
            free(assange_image);
            return 6;
        }
        
        assange_image_size = fread((void *)assange_image, 1, img_stat.st_size, file);
        
        if(ferror(file) != 0) {
            fclose(file);
            free(assange_image);
            assange_log_error("Cannot load image: file could not be read.");
            return 6;
        }
        
        fclose(file);
        
        assange_log_info("Image retrieved from file.  Checking...");
    }
    
    if(assange_image_size < 16) {
        assange_log_error("Image invalid: too small.");
        return 16;
    }
    
    if(memcmp(assange_image, "DNTPNCXX", 8) != 0) {
        assange_log_error("Image invalid: magic bytes do not match.");
        return 16;
    }
    
    if(memcmp(assange_image + 9, "AAR", 3) != 0) {
        assange_log_error("Image cannot be used: this program only can handle image version 0.");
        return 15;
    }
    
    if(assange_image[8] != 'j') {
        assange_log_error("Image cannot be used: this program only can generate jessica hashes.");
        return 15;
    }
    
    if(assange_image[12] != 0) {
        assange_log_error("Image cannot be used: this program only handles mode 0.");
        return 15;
    }
    
    if(assange_image_size < 1040) {
       assange_log_error("Image invalid: jessica pointer table partial.");
       return 16;
    }
    
    {
        int x;
        int current = 0, last = 0;
        for(x = 16; x < 1040; x += 4) {
            current = ASSANGE_READ_U3(assange_image + x);
            
            if(current < last) {
                assange_log_error("Image invalid: jessica pointer table corrupt.");
                return 16;
            }
            
            last = current;
        }
        
        assange_image_hashc = last;
    }

    if(assange_image_size < 1040 + (assange_image_hashc * 20)) {
        assange_log_error("Image invalid: primary jessica table not large enough.");
        return 16;
    }
    
    {
        int x;
        int data_boundary = 1040 + (assange_image_hashc * 20);
        int advertised_data_size = 0;
        
        for(x = 1040; x < data_boundary; x += 20) {
            int len, pos, new;
            
            len = ASSANGE_READ_U4(assange_image + x + 8);
            pos = ASSANGE_READ_U4(assange_image + x + 12);
            
            new = len + pos;
            
            if(new > advertised_data_size) {
                advertised_data_size = new;
            }
        }
        
        if(advertised_data_size + data_boundary > assange_image_size) {
            assange_log_error("Image invalid: primary jessica table advertises content which does not exist.");
            return 16;
        }
    }
    
    // ... and we're done!
    
    assange_pump();

    return 0;
}

#define ASSANGE_CLOSESESS(s) do { s.as_state = ASSANGE_STATE_FREE; close(s.as_socket); assange_occ--; } while(0)


void assange_sig(int signum) {
    switch(signum) {
        case SIGTERM:
        case SIGINT:
            assange_log_info("Closing up shop...");
            {
                int x;
                
                for(x = 0; x < ASSANGE_SESSC; x++) {
                    if(assange_sessions[x].as_state != ASSANGE_STATE_FREE) {
                        ASSANGE_CLOSESESS(assange_sessions[x]);
                    }
                }
            }
            
            assange_log_info("Closing root socket...");
            close(assange_socket);
            
            assange_log_info("Bye!");
            
            _exit(0);
    }
}

void assange_pump(void) {
    struct sockaddr_in address;
    int sess_first = -1, sess_count = 0;
    
    address.sin_family = AF_INET;
    address.sin_port = htons(assange_port);
    address.sin_addr.s_addr = INADDR_ANY;

    assange_socket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0); // TCP socket

    if(assange_socket == -1) {
        assange_log_error("Could not create root socket.");
        return;
    }
    
    if(bind(assange_socket, (struct sockaddr *)&address, sizeof(struct sockaddr_in)) != 0) {
        assange_log_error("Could not bind root socket.");
        close(assange_socket);
        return;
    }
    
    assange_log_info("Attempting to map session memory...");
    
    assange_sessions = malloc(ASSANGE_SESSC * sizeof(struct assange_session));
    
    if(assange_sessions == 0) {
        assange_log_error("Could not map session memory: stage 1 failed.");
        close(assange_socket);
        return;
    }
    
    {
        int x;
        
        for(x = 0; x < ASSANGE_SESSC; x++) {
            assange_sessions[x].as_state = ASSANGE_STATE_FREE;
            assange_sessions[x].as_in = malloc(ASSANGE_MEMSHARE);
            if(assange_sessions[x].as_in == 0) {
                assange_log_error("Could not map session memory: stage 2 failed.");
                close(assange_socket);
                return;
            }
        }
    }
    
    assange_log_info("Session memory ready.");
    
    if(listen(assange_socket, 240) != 0) { // 240 is arbitrary, but purposefully large
        assange_log_error("Could not listen for connections.");
        free(assange_sessions);
        close(assange_socket);
        return;
    }

    assange_log_info("Ready.");
    
    while(1) {
        {
            // idle process
            int usecs = ASSANGE_SESSC - assange_occ;
            if(assange_occ > 0)
                usecs >>= 2;
                
            usleep(usecs);
        }
        assange_scan();
    }
    
    close(assange_socket);
}

#define ASSANGE_DNLSTATE_LOOP(s) do { \
                                     int y; \
                                     for(y = 0; y < (s).as_in_index; y++) { \
                                         if((s).as_state_dnl == ASSANGE_DNLSTATE_BEG) { \
                                             if((s).as_in[y] == '\r') \
                                                 (s).as_state_dnl = ASSANGE_DNLSTATE_R; \
                                         } \
                                         else if((s).as_state_dnl == ASSANGE_DNLSTATE_R) { \
                                             if((s).as_in[y] == '\n') \
                                                 (s).as_state_dnl = ASSANGE_DNLSTATE_RN; \
                                             else if((s).as_in[y] != '\r') \
                                                 (s).as_state_dnl = ASSANGE_DNLSTATE_BEG; \
                                         } \
                                         else if((s).as_state_dnl == ASSANGE_DNLSTATE_RN) { \
                                             if((s).as_in[y] == '\r') \
                                                 (s).as_state_dnl = ASSANGE_DNLSTATE_RNR; \
                                             else \
                                                 (s).as_state_dnl = ASSANGE_DNLSTATE_BEG; \
                                         } \
                                     } \
                                 } while(0)

void assange_scan(void) {
    int x;
    ASSANGE_BOOL accept_failed = ASSANGE_FALSE;
    for(x = 0; x < ASSANGE_SESSC; x++) {
        switch(assange_sessions[x].as_state) {
            case ASSANGE_STATE_FREE:
                if(accept_failed) {
                    break;
                }
                
                {
                    int socket;
                    
                    socket = accept4(assange_socket, NULL, NULL, SOCK_NONBLOCK);
                    
                    if(socket == -1) {
                        accept_failed = ASSANGE_TRUE;
                        break;
                    }
                    
                    assange_sessions[x].as_state = ASSANGE_STATE_RECV_PATH;
                    assange_sessions[x].as_socket = socket;
                    assange_sessions[x].as_out = 0;
                    assange_sessions[x].as_in_index = 0;
                    
                    assange_occ++;
                }
                
                break;
                
            case ASSANGE_STATE_RECV_PATH:
                {
                    ssize_t bytes_read;
                    char * i, i2;

                    bytes_read = read(assange_sessions[x].as_socket,
                                      assange_sessions[x].as_in + assange_sessions[x].as_in_index,
                                      ASSANGE_MEMSHARE - assange_sessions[x].as_in_index);
                    
                    if(bytes_read == -1) {
                        if(errno != EAGAIN && errno != EWOULDBLOCK) { // inexcusable
                            // we don't have time for this bull-crap :)
                            // so drop 'em
                            ASSANGE_CLOSESESS(assange_sessions[x]);
                            break;
                        }
                        else { // excusable
                            break;
                        }
                    }
                    
                    i = (char *)memmem(assange_sessions[x].as_in + assange_sessions[x].as_in_index, bytes_read, "\r\n", 2);

                    assange_sessions[x].as_in_index += bytes_read;
                    
                    if(i == 0) {
                        if(assange_sessions[x].as_in_index == ASSANGE_MEMSHARE) {
                            // we are busy... cannot wait for this bulls***
                            ASSANGE_CLOSESESS(assange_sessions[x]);
                            break;
                        }
                    }
                    else {
                    
                        {
                            int y;
                            ASSANGE_BOOL space_found = ASSANGE_FALSE;
                            
                            if(assange_sessions[x].as_in_index < 16) {
                                write(assange_sessions[x].as_socket, "HTTP/1.1 400 Bad\r\n\r\n", 20);
                                ASSANGE_CLOSESESS(assange_sessions[x]);
                                break;
                            }
                            
                            if(assange_sessions[x].as_in[0] != 'G' ||
                               assange_sessions[x].as_in[1] != 'E' ||
                               assange_sessions[x].as_in[2] != 'T' ||
                               assange_sessions[x].as_in[3] != ' ') {
                                write(assange_sessions[x].as_socket, "HTTP/1.1 400 Method Unsupported\r\n\r\n", 35);
                                ASSANGE_CLOSESESS(assange_sessions[x]);
                                break;
                            }
                            
                            for(y = 4; y < assange_sessions[x].as_in_index; y++) {
                                if(assange_sessions[x].as_in[y] == ' ') {
                                    space_found = ASSANGE_TRUE;
                                    break;
                                }
                            }
                            
                            if(space_found) {
                                ASSANGE_JESSICA aj = assange_jessica();
                                
                                assange_jessica_calc(assange_sessions[x].as_in + 4, y - 4, aj);

                                {
                                    int lowerinc, upperex;
                                    int z;
                                    ASSANGE_BOOL e404 = ASSANGE_TRUE;
                                    char * imgp;
                                    
                                    if(aj[0] == 0) {
                                        lowerinc = 0;
                                    }
                                    else {
                                        lowerinc = ASSANGE_READ_U3(assange_image + 16 + (((int)aj[0] - 1) << 2));
                                    }
                                    
                                    upperex = ASSANGE_READ_U3(assange_image + 16 + (((int)aj[0]) << 2));
                                    
                                    imgp = assange_image + 1040 + lowerinc * 20;

                                    for(z = lowerinc; z < upperex; z++) {
                                        if(assange_jessica_equal(aj, assange_jessica_from_mem(imgp))) {
                                            assange_sessions[x].as_out_size = ASSANGE_READ_U4(imgp + 8);
                                            assange_sessions[x].as_out_index = ASSANGE_READ_U4(imgp + 12) + 1040 + assange_image_hashc * 20;
                                            e404 = ASSANGE_FALSE; // we are good! no need to panic...
                                            break;
                                        }
                                        imgp += 20;
                                    }
                                    
                                    if(e404) {
                                        write(assange_sessions[x].as_socket, "HTTP/1.1 404 CNL\r\n\r\n", 20);
                                        ASSANGE_CLOSESESS(assange_sessions[x]);
                                        break;
                                    }
                                }
                                
                                assange_jessica_destroy(aj);
                                
                            }
                            else {
                                write(assange_sessions[x].as_socket, "HTTP/1.1 400 Bad\r\n\r\n", 20);
                                ASSANGE_CLOSESESS(assange_sessions[x]);
                                break;
                            }
                        }
                        
                        assange_sessions[x].as_out = assange_image;
                        assange_sessions[x].as_state = ASSANGE_STATE_RECV_HDRS;
                        assange_sessions[x].as_state_dnl = ASSANGE_DNLSTATE_BEG;
                        ASSANGE_DNLSTATE_LOOP(assange_sessions[x]);

                        if(assange_sessions[x].as_state_dnl == ASSANGE_DNLSTATE_RNR) {
                            assange_sessions[x].as_state = ASSANGE_STATE_SEND;
                        }
                        
                        assange_sessions[x].as_in_index = 0;
                    }
                }
                break;
                
            case ASSANGE_STATE_RECV_HDRS:
                {
                    ssize_t bytes_read;
                    
                    bytes_read = read(assange_sessions[x].as_socket,
                                      assange_sessions[x].as_in,
                                      ASSANGE_MEMSHARE);
                    
                    if(bytes_read == -1) {
                        if(errno != EAGAIN && errno != EWOULDBLOCK) {
                            ASSANGE_CLOSESESS(assange_sessions[x]);
                            break;
                        }
                        else {
                            break;
                        }
                    }
                    
                    assange_sessions[x].as_in_index = bytes_read;
                    ASSANGE_DNLSTATE_LOOP(assange_sessions[x]);

                    if(assange_sessions[x].as_state_dnl == ASSANGE_DNLSTATE_RNR) {
                        assange_sessions[x].as_state = ASSANGE_STATE_SEND;
                    }
                }
                break;
                
            case ASSANGE_STATE_SEND:
				{
				    int bytes_to_send;
				    			    
				    if(assange_sessions[x].as_out_size >= ASSANGE_SENDSHARE) {
				        bytes_to_send = ASSANGE_SENDSHARE;
				    }
				    else {
				        bytes_to_send = assange_sessions[x].as_out_size;
				    }
				    
				    if(assange_sessions[x].as_state_dnl == ASSANGE_DNLSTATE_RNR) {
				        assange_sessions[x].as_state_dnl = ASSANGE_DNLSTATE_BEG;
				        
				        if(write(assange_sessions[x].as_socket, "HTTP/1.1 200 Ok\r\n", 17) != 17) {
				            ASSANGE_CLOSESESS(assange_sessions[x]);
				            break;
				        }
				    }
				    
                    if(write(assange_sessions[x].as_socket,
                             assange_sessions[x].as_out + assange_sessions[x].as_out_index,
                             bytes_to_send) != bytes_to_send) {
                        ASSANGE_CLOSESESS(assange_sessions[x]);
                        break;
                    }
                    
                    assange_sessions[x].as_out_index += bytes_to_send;
                    assange_sessions[x].as_out_size -= bytes_to_send;

                    if(assange_sessions[x].as_out_size < 1) {
                        ASSANGE_CLOSESESS(assange_sessions[x]);
                        break;
                    }
				}
                break;
        }
    }
}

