# Simple sender for common scripts for the Super Res microscope controller:

import serial
import serial.tools.list_ports as port_list
from serial import SerialException

# script to configure the triggering modes, clocks, etc:
script_1 = [
    "STOP_SEQ",
    "0,off,SET_STATE_CLK",
    "0,1000,SET_PERIOD_CLK",
    "1,rise,3,2,3,SET_PRC"  #  {trg_id =1, mode trigger= rise, burst=3, skip=2, offset=3}
]

# script to configure the sequencer chain
script_2 = [
    "CLEAR_SQ",
    "clk,0,trg,0,prc,1,las,3,out,2,SET_CHAIN_SEQ"
]

# start sequencer from clock activation:
script_3 = [
    "on, SET_STATE_CLK",
    "START_SEQ"
]

script_0 = [
    "STOP_SEQ",
    "0,off,SET_STATE_CLK",

    "0,1000,SET_PERIOD_CLK",
    "1,rise,3,2,3,SET_PRC",

    "CLEAR_SQ",
    "clk,0,trg,0,prc,1,las,3,out,2,SET_CHAIN_SEQ"

    "on, SET_STATE_CLK",
    "START_SEQ"
]

script_4 = [
    "STOP_SEQ",
    "0,off,SET_STATE_CLK",

    "0,1000,SET_PERIOD_CLK",

    "CLEAR_SQ",
    "clk,0,las,0,SET_CHAIN_SEQ"

    "STATUS_SEQ",

    "START_SEQ",
    "on, SET_STATE_CLK"
]

script_5 = [

    "0,off,SET_STATE_CLK",
    "0,1000,SET_PERIOD_CLK",
    "0,1,SET_STATE_CLK",
]

scriptArray = [script_0, script_1, script_2, script_3, script_4, script_5]

###################################################

def enterCommand():
    while True:
        try:
            index = input("Please enter a number: ")
            sendScript(scriptArray[int(index)])# + text)
        except ValueError:
            print("Not valid index for script_# ")

# Script sender:
def sendScript(script):
    i = 1
    for command in script:
        print(str(i) + "> " + command)
        i = i + 1
        ser.write(command.encode('utf-8'))
        # for val in command:
        #     ser.write(val.encode('utf-8'))
        ser.write(b'\n') # include end command character because I did not write it on the script list.

# Check continuously for data received on serial port:
def echoSerial():
    while True:
        msg = str(ser.readline()) # read until EOL characters

        #if len(msg) > 0:
        #        break
        print(msg)


###############################################################
# MAIN:

ports = list(port_list.comports())
for p in ports: print(p)

portName = "/dev/cu.usbmodem4072371"

try:
    ser = serial.Serial(portName, 38400)
    ser.flushInput()
except SerialException:
    print("Port unavailable")
    exit()


while True:
    enterCommand()
    echoSerial()


###############################################################
