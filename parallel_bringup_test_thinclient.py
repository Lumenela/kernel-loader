#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
 * parallel_bringup.py
 * 
 * Created on: 24 June 2011
 * Author:     Viktar Palstsiuk <viktar.palstsiuk@promwad.com>
 * Modifed by: 	Alexey Karpovich <alexey.karpovich@promwad.com>
 * 
 *      Copyright (C) 2011 Promwad
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 * Requires pySerial to run.
 * http://pyserial.sourceforge.net
"""

import ConfigParser
import ftplib
import hashlib
import os
import time
import serial
import sys
import datetime
from threading import Thread
from uuid import getnode as get_mac

print "ThinClient fw update & test script version 1.2a"

class xmodem_load(Thread):
    def __init__ (self,tty, ipaddr, ethaddr, eth1addr):
        Thread.__init__(self)
        self.tty = tty
        if (hw == "thinclient"):
	    helper = "loader.kwb"
	elif (hw == "thinclient_hynix"):
	    helper = "u-boot_TC_uart.kwb"
#	elif (hw == "wtplug512mb"):
#	    helper = "u-boot.512mb.kwb"	    
#	elif (hw == "wtplug_hynix"):
#	    helper = "u-boot.hynix.kwb"	 
#	elif (hw == "wtplug512mb_hynix"):
#	    helper = "u-boot.512mb.hynix.kwb"	
	else:
	    self.report("Wrong hw in config file!");
	    return False

        if os.name == 'posix':
	    # Running on Linux
	    self.devtty = "/dev/"+tty
	    self.file = "./"+helper
	else:
	    # Running on Windows
	    self.devtty = "\\\\.\\"+tty
	    self.file = helper
        self.timestamp = time.strftime("%Y-%m-%d_%H-%M-%S_")
        self.ipaddr = ipaddr
        self.serverip = config.get('Network', 'serverip')
        self.ethaddr = ethaddr
        self.eth1addr = eth1addr
        self.status = False
        self.lastLog = ''
#-----------------------------------------------------------------
    def run(self):
	try:
	    self.ser = serial.Serial(port=self.devtty, baudrate=115200, timeout=5)
	    self.status = False
	    self.ser.flushInput()
	    self.report("Starting")
	    if(self.start_xmodem()):
		self.report("XMODEM session started")
		self.report("Sending "+self.file)
		if (self.xmodem_send()):
		    self.report("XMODEM transfer\t\t\tPASS")
		else:
		    self.report("XMODEM transfer\t\t\tFAIL")
		    return
	    else:
		self.report("XMODEM session start failed due to NAK timeout")
		return

	    self.ser.close()
	except:  
	    self.report("Failed to connect on "+self.devtty)
#-----------------------------------------------------------------
    def start_xmodem(self):
	ch = 0
	self.report("Starting XMODEM session...")
	for i in range(50):
	    self.ser.flushInput()
	    self.ser.write("\xBB\x11\x22\x33\x44\x55\x66\x77")
	    time.sleep(0.2)
	    if (self.ser.inWaiting() > 0):
		ch = self.ser.read(1)
		if ch == chr(0x15):
		    break;
	
	if ch == chr(0x15):
	    return True
	else:
	    return False

#-----------------------------------------------------------------
    def falsh_uboot(self):
	self.report("Uboot Flashing started")

	self.sendcmd("\r")
	self.waitfor("U-Boot")
	self.sendcmd("\r")
	
	if not self.waitfor("Marvell>>"):
	    self.report("U-boot UART start\t\tFAIL")
	    return False
	    
	self.sendcmd("setenv serverip "+self.serverip+"\n")
	self.sendcmd("setenv ipaddr "+self.ipaddr+"\n")
	self.sendcmd("setenv ethaddr "+self.ethaddr+"\n")
	self.sendcmd("setenv eth1addr "+self.eth1addr+"\n")

	self.sendcmd("nand erase.chip\n")
	if not self.waitfor("OK"):
	    self.report("NAND erase\t\t\tFAIL")
	    return False
	else:
	    self.report("NAND erase\t\t\tPASS")	       

#	self.sendcmd("saveenv\n")
#	if not self.waitfor("done"):
#	    self.report("Save Environment\t\tFAIL")
#	    return False	    
#	else:
#	    self.report("Save Environment\t\tPASS")

	self.sendcmd("tftp 0x06400000 "+hw+"-u-boot.kwb\n")
	if not self.waitfor("done"):
	    self.report("TFTP U-boot\t\t\tFAIL ("+hw+")")
	    return False	    
	else:
	    self.report("TFTP U-boot\t\t\tPASS")	    

	if "hynix" in hw:
	    self.sendcmd("nand write 0x06400000 0x0 0x200000\n")
	else:
	    self.sendcmd("nand write 0x06400000 0x0 0xa0000\n")
	if not self.waitfor("OK"):
	    self.report("U-boot update\t\t\tFAIL")
	    return False
	else:
	    self.report("U-boot update\t\t\tPASS")  

	self.report("Uboot Flashing finished")	    
	return True
	    
#-----------------------------------------------------------------
    def upgrade_fw(self):
	self.report("Firmware upgrade started")

	self.sendcmd("setenv serverip "+self.serverip+"\n")
	self.sendcmd("setenv ipaddr "+self.ipaddr+"\n")
	self.sendcmd("setenv ethaddr "+self.ethaddr+"\n")
	self.sendcmd("setenv eth1addr "+self.eth1addr+"\n")

	self.sendcmd("tftp 0x06400000 "+hw+"_uImage\n")
	if not self.waitfor("done"):
	    self.report("TFTP Kernel\t\t\tFAIL")
	    return False
	else:
	    self.report("TFTP Kernel\t\t\tPASS")	    

	if "hynix" in hw:
	    self.sendcmd("nand write.e 0x06400000 0x400000 0x400000\n")
	else:
	    self.sendcmd("nand write.e 0x06400000 0x100000 0x400000\n")	 
	if not self.waitfor("OK"):
	    self.report("Kernel update\t\t\tFAIL")
	    return False
	else:
	    self.report("Kernel update\t\t\tPASS")

	if "hynix" in hw:
	    self.sendcmd("nand write.e 0x06400000 0x800000 0x400000\n")
	else:
	    self.sendcmd("nand write.e 0x06400000 0x500000 0x400000\n")
	if not self.waitfor("OK"):
	    self.report("Kernel rcvr update\t\tFAIL")
	    return False
	else:
	    self.report("Kernel rcvr update\t\tPASS")
	    
	self.sendcmd("tftp 0x06400000 rootfs_rcvr.ubi.img\n")
	if not self.waitfor("done"):
	    self.report("TFTP RootFS rcvr\t\tFAIL")
	    return False
	else:
	    self.report("TFTP RootFS rcvr\t\tPASS")	    

	if "hynix" in hw:
	    self.sendcmd("nand write.e 0x06400000 0xC00000 ${filesize}\n")
	else:
	    self.sendcmd("nand write.e 0x06400000 0x900000 ${filesize}\n")
	if not self.waitfor("OK"):
	    self.report("RootFS rcvr update\t\tFAIL")
	    return False
	else:
	    self.report("RootFS rcvr update\t\tPASS")	

	self.sendcmd("tftp 0x06400000 rootfs.ubi.img\n")
	if not self.waitfor("done"):
	    self.report("TFTP RootFS\t\t\tFAIL")
	    return False
	else:
	    self.report("TFTP RootFS\t\t\tPASS")	    

	if "hynix" in hw:
	    self.sendcmd("nand write.e 0x06400000 0x4C00000 ${filesize}\n")
	else:
	    self.sendcmd("nand write.e 0x06400000 0x4900000 ${filesize}\n")
	if not self.waitfor("OK"):
	    self.report("RootFS update\t\t\tFAIL")
	    return False
	else:
	    self.report("RootFS update\t\t\tPASS")	

	self.report("Firmware upgrade finished")
	return True

#-----------------------------------------------------------------
    def xmodem_send(self):
        if os.name == 'posix':
            if os.system("sz --xmodem --timeout 15 "+self.file+" > "+self.devtty+" < "+self.devtty) == 0:
                return True
            else:
                return False     
	t = 0
	while 1:
	    if self.ser.read(1) != chr(0x15):
		t = t + 1
		if t == 60 : return False
	    else:
		break
	p = 1
	file = open(self.file, 'rb') 
	s = file.read(128)
	while s:
	    s = s + '\xFF'*(128 - len(s))
	    chk = 0
	    for c in s:
		chk+=ord(c)
	    while 1:
		self.ser.write(chr(0x01))
		self.ser.write(chr(p))
		self.ser.write(chr(255 - p))
		self.ser.write(s)
		self.ser.write(chr(chk%256))
		self.ser.flush()

		answer = self.ser.read(1)
		if  answer == chr(0x15): continue
		if  answer == chr(0x06): break
		file.close()
		return False
	    s = file.read(128)
	    p = (p + 1)%256
	    if p == 0:
		self.report(str(file.tell())+" bytes sent over XMODEM")
	self.ser.write(chr(0x04))
	file.close()
	return True
#-----------------------------------------------------------------
    def upgrade_env(self):
	self.report("Save Environment")

	self.sendcmd("setenv serverip "+self.serverip+"\n")
	self.sendcmd("setenv ipaddr "+self.ipaddr+"\n")
	self.sendcmd("setenv ethaddr "+self.ethaddr+"\n")
	self.sendcmd("setenv eth1addr "+self.eth1addr+"\n")

	self.sendcmd("saveenv\n")
	if not self.waitfor("done"):
	    self.report("Save Environment\t\tFAIL")
	    return False	    
	else:
	    self.report("Save Environment\t\tPASS")

	self.report("Save Environment finished")
	return True
#-----------------------------------------------------------------
    def test_hw(self):
	#self.sendcmd("\nversion\n")
	self.sendcmd("reset\r")
	self.sendcmd("reset\r")
	if not self.waitfor2("U-Boot",2):
	    self.report("U-boot start\t\t\tFAIL")
	    return False
	self.report("Starting U-boot")    
	self.sendcmd("\r")
	self.waitfor("U-Boot")
	self.sendcmd("\r")

	if not self.waitfor("Marvell>>"):
	    self.report("U-boot start\t\t\tFAIL")
	    return False
	self.report("U-boot start\t\t\tPASS")

	self.report("Hardware test started")

	self.sendcmd("setenv ipaddr "+self.ipaddr+"\n")

	self.sendcmd("printenv\n")
	if not self.waitfor("Environment"):
	    self.report("U-boot Environment\t\tFAIL")
	    return False
	else:
	    self.report("U-boot Environment\t\tPASS")
	    
	nand_id = "Device 0: NAND"
	self.sendcmd("nand info\n")
	if not self.waitfor(nand_id):
	    self.report("NAND Identification\t\tFAIL")
	    return False
	else:
	    self.report("NAND Identification\t\tPASS")

	self.sendcmd("setenv ethact egiga0; ping "+self.serverip+"\n")
	if not self.waitfor("Using egiga0 device"):
	    self.report("Network (ping) test(egiga0)\tNO LINK")
	    return False
	else:
	    if not self.waitfor2("is alive",4):
	      self.report("Network (ping) test(egiga0)\tFAIL")
	      return False
	    else:
	      self.report("Network (ping) test(egiga0)\tPASS")

	      
	if config.getboolean('Testing', 'run_eth1_test'):
	    self.sendcmd("setenv ethact egiga1; ping "+self.serverip+"\n")
	    if not self.waitfor("Using egiga1 device"):
	      self.report("Network (ping) test(egiga1)\tNO LINK")
	      return False
	    else:
		if not self.waitfor2("is alive",4):
		   self.report("Network (ping) test(egiga1)\tFAIL")
		   return False
		else:
		   self.report("Network (ping) test(egiga1)\tPASS")
		    
	result=True
#	if ("512mb" in hw):
#	    self.sendcmd("mtest 0x0ff00000 0x10100000 0x00 1\n")
#	else:
	self.sendcmd("mtest 0x1ff00000 0x20100000 0x00 1\n")
	if not self.waitfor("with 0 errors"):
	    result=False
	    
#	if not ("512mb" in hw):
	self.sendcmd("mw.b 0x0fffff00 0x00 0x10\n")
	self.sendcmd("mw.b 0x1fffff00 0xFF 0x10\n")
	self.sendcmd("cmp.b 0x0fffff00 0x1fffff00 0x10\n")
	if not self.waitfor("Total of 0"):
	    result=False

	if (result): 
	    self.report("RAM test\t\t\tPASS")
	else:
	    self.report("RAM test\t\t\tFAIL")
	    return False
	    
	date = datetime.datetime.now()
	self.sendcmd("date "+date.strftime('%m%d%H%M%Y')+"\r")
	    
	self.sendcmd("version\n")  

	self.report("Hardware test finished")

	return True
#-----------------------------------------------------------------
    def boot(self):

	self.report("Software test started")
	self.sendcmd("reset\r")
	self.sendcmd("reset\r")


	if not self.waitfor2("U-Boot",1):
	    self.report("U-boot start\t\t\tFAIL")
	    return False
	else:
	    self.report("U-boot start\t\t\tPASS")

	if not self.waitfor("Starting kernel"):
	    self.report("Kernel start\t\t\tFAIL")
	    return False
	else:
	    self.report("Kernel start\t\t\tPASS")

	if not self.waitfor2("login:",10):
	    self.report("Login\t\t\tFAIL")
	    return False
	self.sendcmd("root\r")

	if not self.waitfor("Password:"):
	    self.report("Login\t\t\tFAIL")
	    return False
	self.sendcmd("nosoup4u\r")

	self.sendcmd("\r")
	if not self.waitfor("root"):
	    self.sendcmd("\r")
	    if not self.waitfor("root"):
	      self.report("Login\t\t\tFAIL")
	      return False

	self.sendcmd("pname\r")
      	if not self.vers_compar():
	    self.report("Version comparison\t\tFAIL")
	    return False
	else:    
	    self.report("Version comparison\t\tPASS")

	if config.getboolean('Testing', 'run_pci_test'):
	  self.sendcmd("lspci | grep -i Network\r")
	  self.waitfor("Network")
	  if not self.waitfor("Network"):
	    self.report("PCI express network module\tFAIL")
	    return False
	  else:
#	    self.sendcmd("lsusb | grep -i Bluetooth\r")
#	    self.waitfor("Bluetooth")
#	    if not self.waitfor("Bluetooth"):
#	      self.report("PCI express bluetooth module\tFAIL")
#	      return False
#	    else:
	      self.report("PCI express network module\tPASS")

	usbtest=config.get('Testing', 'run_usb_test')
	if usbtest != "no" and usbtest != "NO":
	#if config.getboolean('Testing', 'run_usb_test'):
	  self.sendcmd("lsusb -v | grep -o '8 Mass Storage' | wc -l\r")
	  if not self.waitfor(usbtest):
	      self.report("USB storage device test\t\tFAIL")
	      return False
	  else:
	      self.report("USB storage device test\t\tPASS")

	if config.getboolean('Testing', 'run_audio_test'):
	  self.sendcmd("audio-test.sh\r")
	  if not self.waitfor("TEST PASS"):
	    self.report("Audio test\t\t\tFAIL")
	    return False
	  else:
	    self.report("Audio test\t\t\tPASS")


	self.report("Software test finished")	
	return True
#-----------------------------------------------------------------
    def vers_compar(self):
	softversion = config.get('FTP', 'softversion')
	if softversion  == '':
	     self.log( "NO options: softversion")
	     return False	     
	v_svn=v_kernel=v_uboot=''
	try:
	  softverfile = open(softversion,'r')
	  while True:
	    line=softverfile.readline()
	    if line:
	      #pos = line.find("svn:")
	      line=line.replace('\n','')
	      line=line.replace('\r','')
	      if "svn:" in line:
		v_svn = line
	      if "svn_kernel:" in line:
		v_kernel = line
	      if "svn_uboot:" in line:
		v_uboot = line
	    else:
	      break
	  softverfile.close()
	  if v_svn == '' or v_kernel == '' or v_uboot == '':
	     self.log( "Error file "+softversion+" content")
	     return False

	except IOError:
	    self.log( "File "+softversion+" does not exist")
	    return False

	cntr=3;
	while True:
	  buffer = self.ser.readline()
	  self.log("RCVD: "+buffer)
	  if v_svn in buffer:
	    cntr-=1
	  if v_kernel in buffer:
	    cntr-=1
	  if v_uboot in buffer:
	    cntr-=1
	  if (buffer == ''):
	    break

	if cntr != 0:
	  return False

	return True
#-----------------------------------------------------------------
    def waitfor(self, string):
	#self.ser.flushInput()
	while True:
	    buffer = self.ser.readline()
	    self.log("RCVD: "+buffer)
	    if string in buffer:
		self.log("Found: "+string)
		return True
	    if (buffer == ''):
		self.log("Can't find: "+string)
		return False
#-----------------------------------------------------------------
    def waitfor2(self, string, wait):
	if self.waitfor(string):
	  return True
	time.sleep(wait)
	if self.waitfor(string):
	  return True
	return False
#-----------------------------------------------------------------   
    def log(self, string):
	#print "\n["+time.ctime(), self.tty+"]\t"+string

	logfile = open("logs/"+self.timestamp+self.tty+".log",'a')
	logfile.write(time.ctime()+"\t")
	logfile.write(string+"\n")
	logfile.close()
#-----------------------------------------------------------------	
    def report(self, string):
	self.lastLog=string
      	start_clr = '\033[0m'
	end_clr = '\033[0m'      
	if "FAIL" in string:
	    start_clr = '\033[1;31m'
	if "PASS" in string:
	    start_clr = '\033[1;32m'	    
	if "NO LINK" in string:
	    start_clr = '\033[1;33m'
	if os.name == 'posix':
	    print start_clr+"["+time.ctime(), self.tty+"]\t"+string+end_clr
	else:
	    print "["+time.ctime(), self.tty+"]\t"+string	    
	logfile = open("logs/"+self.timestamp+self.tty+".txt",'a')
	logfile.write(time.ctime()+"\t")
	logfile.write(string+"\n")
	logfile.close()
#-----------------------------------------------------------------
    def sendcmd(self, string):
	self.log("SENT: "+string)
	time.sleep(0.3)
	self.ser.flushInput()
	self.ser.write(string)
#-----------------------------------------------------------------
def get_maclist():
    print "Generating MAC addresses"
    mac = manufactureid << 16
    maclist = []
    while  len(maclist) < len(ttylist)*2 :
	ethaddr = "00:E1:75:%02X:%02X:%02X" % ((mac>>16)&0xff, (mac>>8)&0xff, mac&0xff)
	if ((mac>>16)&0xff) != manufactureid:
	   return maclist
	mac += 1
	try:
            devicesfile = open(deviceslist,'r')
            while True:
                line = devicesfile.readline()
                if line:
                    if ethaddr in line:
                        break
                else:
                    print "Free MAC found", ethaddr
                    maclist.append(ethaddr)
                    break
            devicesfile.close()
        except IOError:
            print "The file "+deviceslist+" does not exist"
            return maclist
    return maclist
#-----------------------------------------------------------------
def append_device(ethaddr, eth1addr):
#    deviceid = config.get('FTP', 'deviceid')
#    hw = config.get('FTP', 'hw')
#    sw = config.get('FTP', 'sw')
    version = config.get('FTP', 'version')
    try:	
	#devicesfile = open("devices.csv",'a')
	devicesfile = open(deviceslist,'a')
	devicesfile.write(deviceid+"."+hw+"."+sw+","+version+","+time.ctime()+","+ethaddr+","+eth1addr+"\n")
	devicesfile.close()
	return True
    except IOError:
	self.report("Append new device\t\tFAIL")
	return False
#-----------------------------------------------------------------
if (len(sys.argv) != 2):
    print "Usage:"
    print sys.argv[0], "<config file>"
    sys.exit(1)

config = ConfigParser.RawConfigParser()
config.read(sys.argv[1])

deviceid = config.get('FTP', 'deviceid')
hw = config.get('FTP', 'hw')
sw = config.get('FTP', 'sw')
manufactureid = config.getint('FTP', 'manufactureid')
deviceslist = config.get('FTP', 'deviceslist')

if os.name == 'posix':
    print "Running on Linux"
    # List of serial ports
    ttylist = config.get('Comport', 'ttylist').split()
else:
    print "Running on Windows"
    ttylist = config.get('Comport', 'comlist').split()

maclist = get_maclist()

if len(maclist) < len(ttylist)*2:
   print "NO FREE MAC ADDRES!"
   sys.exit(1)

print time.ctime(), "Power on device(s) now"

# IP subnet and first IP addres
subnet = config.get('Network', 'subnet')
ipaddr = config.getint('Network', 'ipaddr')

if not os.path.isdir('logs'):
   os.makedirs('logs')

xmodemlist = []

for tty in ttylist:
    if append_device(maclist[0], maclist[1]):
	current = xmodem_load(tty, subnet+str(ipaddr), maclist[0], maclist[1])
	xmodemlist.append(current)
	current.start()
    ipaddr += 1
    maclist = maclist[2:]

for xmodemle in xmodemlist:
    xmodemle.join()

print "\n",time.ctime(), "Done\n"

for xmodemle in xmodemlist:
   if xmodemle.status:
       print "Status ",xmodemle.tty,":\tpass"
   else:
       print "Status ",xmodemle.tty,":\tFAIL! [",xmodemle.lastLog,"]"
print "\n"

