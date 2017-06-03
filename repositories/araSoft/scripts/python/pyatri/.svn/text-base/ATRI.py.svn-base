# pyatri module

import socket
import sys
import struct
import array
import string
import time

# ATRI packet controller interface
# ATRI control packets are "sizeof(AtriControlPacket_t"'s
# This is BBBB64B in Struct speak
class ATRIPC:
    def __init__(self):
        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.buffer = struct.Struct("BBBB64B")
        self.wbbuf = struct.Struct("BH")
        self.SOF = ord('<')
        self.EOF = ord('>')
        self.I2C_DIRECT = 0
        self.WB_WRITE = 1
        self.WB_READ = 0
        self.DEST_DBSTATUS = 2
        self.DEST_WB = 1
        self.DEST_D1 = 3
        self.DEST_D2 = 4
        self.DEST_D3 = 5
        self.DEST_D4 = 6
        
    def connect(self):
        self.sock.connect('/tmp/atri_control')
        
    def disconnect(self):
        self.sock.close()

    # Access the dbstatus destination...
    def dbstatus(self):
        data = []
        self.packet(self.DEST_DBSTATUS, data)
        return data

    # I2C destination access
    def i2c_read(self, stack, address, data):
        length = len(data)
        if length == 0:
            return
        address |= 0x01
        dat = [ self.I2C_DIRECT, address, length ]
        self.packet(self.DEST_D1+stack, dat)
        data[:] = dat[2:]
        return

    def i2c_write(self, stack, address, data):
        length = len(data)
        if length == 0:
            return
        # Fix the address if someone screwed up
        address &= 0xFE        
        dat = [ self.I2C_DIRECT, address ]
        dat[2:2+len(data)] = data
        self.packet(self.DEST_D1+stack, dat)
        return
    
    # WB destination access
    def wb_write(self, address, data):
        length = len(data)
        if length == 0:
            return
        dat = [ self.WB_WRITE, (address & 0xFF00)>>8, address & 0x00FF]
        dat[3:3+len(data)] = data
        self.packet(self.DEST_WB, dat)
        data[0] = dat[0]
        
    def wb_read(self, address, data):
        length = len(data)
        if length == 0:
            return
        dat = [ self.WB_READ, (address & 0xFF00)>>8, address & 0x00FF, length]
        self.packet(self.DEST_WB, dat)
        data[:] = dat

    # Packet generator
    def packet(self, destination, data):
        length = len(data)
        full_data = [0]*64
        if length > 0:
            full_data[0:len(data)] = data
        full_data[len(data)] = self.EOF
        b = self.buffer.pack(self.SOF,destination,0,length,*full_data)
        self.sock.sendall(b)
        resp = self.sock.recv(self.buffer.size)
        rdata = self.buffer.unpack(resp)
        if rdata[3] > 0:
            data[:] = rdata[4:4+rdata[3]]
        else:
            del data[:]

# ATRI FX2 interface
# FX2 control packets are "sizeof(Fx2ControlPacket_t)"'s
# This is "BBHHH64B"
class ATRIFX2:
    def __init__(self):
        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.buffer = struct.Struct("BBHHH64B")
        self.response = struct.Struct("hBBHHH64B")

    def connect(self):
        self.sock.connect('/tmp/fx2_control')

    def disconnect(self):
        self.sock.close()

    # 'data' will be modified here.
    def control(self, bmRequestType, bRequest, wValue, wIndex, data):
        length = len(data)
        full_data = [0]*64
        full_data[0:len(data)] = data
        b = self.buffer.pack(bmRequestType, bRequest, wValue, wIndex, length, *full_data)
        self.sock.sendall(b)
        resp = self.sock.recv(self.response.size)
        rdata = self.response.unpack(resp)
        if rdata[5] > 0:
            data[:] = rdata[6:6+rdata[5]]
        else:
            del data[:]
        return rdata[0]
    
    # 'data' is replaced with the data read from I2C if successful
    def i2c_read(self, address, data):
        # Force the address to have the low-bit set
        address = address | 0x1
        status = self.control(0xC0, 0xB4, address, 0x00, data)
        if status < 0:
            print 'error %d reading from %d' % (status, address)
            return -1
        return 0

    def i2c_write(self, address, data):
        address = address & 0xFE
        status = self.control(0x40, 0xB4, address, 0x00, data)
        if status < 0:
            print 'error %d writing to %d' % (status, address)
            return -1
        return 0    

class Scalars:
    def __init__(self, data):
        self.raw = data

    # Accesses the scalars via TDA and channel mapping
    # Stack is zero-based, as is channel.
    def tda_channel(self, stack, channel):
        # Calculate the start.
        start = stack*8 + channel*2
        rate = (self.raw[start] + (self.raw[start+1] << 8))*32
        return rate

class FirmwareIdent:
    # Firmware ident format:
    # Data comes in "0,1,2,3,4,5,6,7"
    # 0-3 are the identifier, but print them out 3->0
    # 4 is the revision
    # 5 is the major and minor numbers (major = high nybble, minor = low nyb)
    # 6 is the day
    # 7 is the board revision and month (rev = high nyb, month = low nyb)
    def __init__(self, data):
        self.data = data
        self.ident = string.joinfields(map(chr, self.data[0:4]), "")[::-1]
        self.board_rev = chr(ord('A')+((self.data[7] & 0xF0)>>4))
        self.ver_month = (self.data[7] & 0x0F)
        self.ver_day = (self.data[6])
        self.ver_major = (self.data[5] & 0xF0)>>4
        self.ver_minor = (self.data[5] & 0x0F)
        self.ver_rev = (self.data[4])
        
    def str(self):
        fwstr = "%s rev %c v%d.%d.%d (%d/%d)" % (self.ident, self.board_rev, self.ver_major, self.ver_minor, self.ver_rev, self.ver_month, self.ver_day)
        return fwstr

class Stack:
    def __init__(self, atri, number):
        self.number = number
        self.atri = atri
        self.tda = TDA(number, atri)
        self.dda = DDA(number, atri)

class TDA:
    def __init__(self, number, atri):
        self.number = number
        self.atri = atri

    def temperature(self):
        # address is 0x92
        data = [0]*2
        self.atri.pc.i2c_read(self.number, 0x92, data)
        if (len(data) < 2):
            print "-- NACK on temp sensor"
            return 0
        temp = (data[0] << 4) | (data[1]>>4)
        if temp & 0x0800:
            temp = (~temp) & 0xFFF
            temp = temp + 1
            temp = -1*temp
        return float(temp)/16

    def voltage(self):
        # address is 0xFC, need to write to it first
        data = [ 0x1A ]
        self.atri.pc.i2c_write(self.number, 0xFC, data)
        rdata = [0]*3
        self.atri.pc.i2c_read(self.number, 0xFC, rdata)
        if (len(rdata) < 2):
            print "-- NACK on hot swap controller"
            return 0
        voltage = (rdata[0] << 4) | ((rdata[2] & 0xF0)>>4)
        return float(voltage)*6.65/4096

    def current(self):
        # address is 0xFC, need to write to it first
        data = [ 0x1A ]
        self.atri.pc.i2c_write(self.number, 0xFC, data)
        data = [0]*3
        self.atri.pc.i2c_read(self.number, 0xFC, data)
        if (len(data) < 2):
            print "-- NACK on hot swap controller"
            return 0
        current = (data[1] << 4) | ((data[2] & 0x0F))
        return float(current)*(105.84/4096)*(1/0.2)



class DDA:
    def __init__(self, number, atri):
        self.number = number
        self.atri = atri

    def temperature(self):
        # address is 0x90
        data = [0]*2
        self.atri.pc.i2c_read(self.number, 0x90, data)
        if (len(data) < 2):
            print "-- NACK on temp sensor"
            return 0
        temp = (data[0] << 4) | (data[1]>>4)
        if temp & 0x0800:
            temp = (~temp) & 0xFFF
            temp = temp + 1
            temp = -1*temp
        return float(temp)/16

    def voltage(self):
        # address is 0xE4, need to write to it first
        data = [ 0x1A ]
        self.atri.pc.i2c_write(self.number, 0xE4, data)
        rdata = [0]*3
        self.atri.pc.i2c_read(self.number, 0xE4, rdata)
        if (len(rdata) < 2):
            print "-- NACK on hot swap controller"
            return 0
        voltage = (rdata[0] << 4) | ((rdata[2] & 0xF0)>>4)
        return float(voltage)*6.65/4096

    def current(self):
        # address is 0xE4, need to write to it first
        data = [ 0x1A ]
        self.atri.pc.i2c_write(self.number, 0xE4, data)
        data = [0]*3
        self.atri.pc.i2c_read(self.number, 0xE4, data)
        if (len(data) < 2):
            print "-- NACK on hot swap controller"
            return 0
        current = (data[1] << 4) | ((data[2] & 0x0F))
        return float(current)*(105.84/4096)*(1/0.2)

    

class ATRI:
    def __init__(self):
        self.fx2 = ATRIFX2()
        self.pc = ATRIPC()
        self.cur_status = [ 0, 0, 0, 0]
        self.ident = None
        self.stacks = [ Stack(self,0), Stack(self,1), Stack(self,2), Stack(self,3) ]

    def tda(self, stack):
        if (self.cur_status[stack] & 0x4):
            return self.stacks[stack].tda
        else:
            return None

    def dda(self, stack):
        if (self.cur_status[stack] & 0x1):
            return self.stacks[stack].dda
        else:
            return None

    def stack(self, number):
        return self.stacks[number]
        
    def connect(self):
        self.fx2.connect()
        self.pc.connect()
        self.update_dbstatus()
        self.update_ident()
        
    def disconnect(self):
        self.fx2.disconnect()
        self.pc.disconnect()
        
    def voltage(self):
        data = [ 0x05 ]
        self.fx2.i2c_write(0x96, data)
        data = [ 0x00 ]
        self.fx2.i2c_read(0x97, data)
        volts = float(data[0])*0.0605
        return volts

    def current(self):
        data = [ 0x04 ]
        self.fx2.i2c_write(0x96, data)
        data = [ 0x00 ]
        self.fx2.i2c_read(0x97, data)
        amps = float(data[0])*0.0755
        return amps

    def update_dbstatus(self):
        self.cur_status = self.pc.dbstatus()

    def update_ident(self):
        ident = [0]*8
        self.pc.wb_read(0x0, ident)
        self.ident = FirmwareIdent(ident)

    def dbstatus(self, stack):
        return self.cur_status[stack]

    def identify(self):
        return self.ident    

    def scalars(self):
        raw = [0]*32
        self.pc.wb_read(0x0100,raw)
        return Scalars(raw)
    
