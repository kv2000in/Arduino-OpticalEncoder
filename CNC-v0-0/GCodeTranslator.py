#!/usr/bin/python
# -*- coding: utf-8 -*-
'''
Code to translate gcode into motion
<F-2520> = one rotation of motor A optical disk
<R-2520> = reverse motorA
<f-892> = One rotation of motor B Optical disk
<r-892> = reverse
'''
import serial
import time
ser = serial.Serial()
ser.baudrate = 9600
ser.port = '/dev/tty.wchusbserialfa140'
ser.timeout=30;
ser.open()
time.sleep(5)
try:
	ser.write('<f-25200>\n<F-25200>\n')
	print(ser.readline().split('-')[0])
	ser.write('<F-25200>\n')
	print(ser.readline().split('-')[0])
	ser.write('<r-25200>\n')
	print(ser.readline().split('-')[0])
	ser.write('<R-25200>\n')
	print(ser.readline().split('-')[0])
except Exception as inst:
	print(type(inst))    # the exception instance
	print(inst.args)     # arguments stored in .args
ser.close() 
