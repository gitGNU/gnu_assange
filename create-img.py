#! /usr/bin/python

#   Assange Webserver: The webserver for sysadmins that are having a really, really bad day.
#   Copyright (c) 2011 William Demchick
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.


import sys

def panic(msg):
    print "[!] Uh oh :(...", msg
    sys.exit(10)

import optparse

optengine = optparse.OptionParser()

optengine.add_option("-i", "--input-file", dest="input_file_name",
                     default=None,
                     help="The input (root source) file")

optengine.add_option("-o", "--output-file", dest="output_file_name",
                     default=None,
                     help="The output (image) file")

(opts, args) = optengine.parse_args()

if opts.input_file_name == None:
    panic("you must supply a root source file name")

if opts.output_file_name == None:
    panic("you must supply a image file name")

try:
    ifile = open(opts.input_file_name, "rb")
except IOError:
    panic("could not open source file")
    
try:
    raw_source = ifile.read()
except IOError:
    ifile.close()
    panic("could not read the source file")
    
ifile.close()

source = raw_source.replace("\r", "").split("\n")

x = 0
while x < len(source):
    source[x] = source[x].strip(" \t")
    x += 1

x = 0
while x < len(source):
    if source[x] == "":
        del source[x]
    else:
        x += 1

if len(source) < 1:
    panic("source file empty")
    
if source[0][0] != "$":
    panic("syntax error: first line declaration missing")
    
raw_mode = source[0][1:].strip(" \t")

ASSANGE_MODE_NORMAL = 0
ASSANGE_MODE_MOVE   = 1

if raw_mode == "dict":
    mode = ASSANGE_MODE_NORMAL
elif raw_mode == "move":
    mode = ASSANGE_MODE_MOVE
    panic("mass move mode not supported yet")
else:
    panic("mode not supported")
    
del source[0]

binary = "DNTPNCXXjAAR"

def jessica(ins):
    outi = []
    outi.append(ord("J"))
    outi.append(ord("e"))
    outi.append(ord("S"))
    outi.append(ord("s"))
    outi.append(ord("I"))
    outi.append(ord("c"))
    outi.append(ord("A"))
    outi.append(ord("@"))
    
    for c in ins:
        cc = ord(c)
        
        i = ((cc & 0xF0) >> 4) ^ (cc & 0x0F)
        
        if i > 7:
            i ^= 0x0F
            
        outi[i] ^= cc
    
    return chr(outi[0]) + chr(outi[1]) + chr(outi[2]) + chr(outi[3]) + chr(outi[4]) + chr(outi[5]) + chr(outi[6]) + chr(outi[7])

def compile_u4(value):
    value_r = int(value)
    value_r = min(value_r, 0x7FFFFFFF)
    value_r = max(value_r, 0x00000000)
    
    return chr((value_r >> 24) & 0xFF) + chr((value_r >> 16) & 0xFF) + chr((value_r >> 8) & 0xFF) + chr(value_r & 0xFF)

def compile_u3(value):
    value_r = int(value)
    value_r = min(value_r, 0x00FFFFFF)
    value_r = max(value_r, 0x00000000)
    
    return chr(0) + chr((value_r >> 16) & 0xFF) + chr((value_r >> 8) & 0xFF) + chr(value_r & 0xFF)

def compile_b10(value):
    value_r = int(value)
    value_r = min(value_r, 0x000003FF)
    value_r = max(value_r, 0x00000000)
    
    return chr(0) + chr(0) + chr((value_r >> 8) & 0x03) + chr(value_r & 0xFF)

import os.path

def guess_mime(file_name):
    file_name = file_name.lower()
    
    retvalue = "text/html"
    
    if file_name == "readme":
        retvalue = "text/plain"
    elif file_name.endswith(".txt"):
        retvalue = "text/plain"
    elif file_name.endswith(".log"):
        retvalue = "text/plain"
    elif file_name.endswith(".png"):
        retvalue = "image/png"
    elif file_name.endswith(".jpeg"):
        retvalue = "image/jpeg"
    elif file_name.endswith(".jpg"):
        retvalue = "image/jpeg"
    elif file_name.endswith(".gif"):
        retvalue = "image/gif"
    elif file_name.endswith(".css"):
        retvalue = "text/css"
        
    return retvalue

def compile_dict(srcs):
    binary = ""
    
    htable = {}
    btable = {}
    hprefix = {}
    hlist = []
    
    x = 0
    while x < 256:
        hprefix[chr(x)] = []
        x += 1
    
    for s in srcs:
        (path, sep, blob_file_name) = s.replace("\t", " ").partition(" ")
        path = path.strip(" \t")
        blob_file_name = blob_file_name.strip(" ")
        
        hsh = jessica(path)
        
        if hsh in htable:
            panic("hash collision")
            
        htable[hsh] = os.path.join(os.path.dirname(opts.input_file_name), blob_file_name)
        
        hprefix[hsh[0]].append(hsh)
        
        
    for hsh in htable:
        try:
            ifile = open(htable[hsh], "rb")
        except IOError:
            panic("could not open blob file")
        
        try:
            btable[hsh] = "Content-Type: " + guess_mime(htable[hsh]) + "\r\nCache-Control: no-cache\r\n\r\n" + ifile.read()
        except IOError:
            ifile.close()
            panic("could not read blob file")
            
        ifile.close()
    
    x = 0
    y = 0
    while y < 256:
        p = chr(y)
        x += len(hprefix[p])
        
        if x > 0x00FFFFFF:
            panic("(way) too many entries; you CRAZY")
            
        binary += compile_u3(x)
    
        for h in hprefix[p]:
            hlist.append(h)
            
        y += 1
    
    x = 0
    for h in hlist:
        binary += h
        binary += compile_u4(len(btable[h]))
        binary += compile_u4(x)
        binary += "\x00\x00\x00\x00"
        x += len(btable[h])
    
    for h in hlist:
        binary += btable[h]
    
    return binary

if mode == ASSANGE_MODE_MOVE:
    binary += "\x01\x00\x00\x00"
    binary += compile_move(source)
elif mode == ASSANGE_MODE_NORMAL:
    binary += "\x00\x00\x00\x00"
    binary += compile_dict(source)

try:
    ofile = open(opts.output_file_name, "wb")
except IOError:
    panic("could not open image file")
    
try:
    ofile.write(binary)
except IOError:
    ofile.close()
    panic("could not write image file")
    
ofile.close()

print "[i] Done."
