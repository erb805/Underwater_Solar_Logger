import serial
import csv
import matplotlib.pyplot as plt
import os
import time

DEBUG = 0
WRITETOFILE = 4

TIMEPACKET = 1
VOLTAGEPACKET = 2
CURRENTPACKET = 3
EOFPACKET = 4
STATE = VOLTAGEPACKET

PACKETS_RECVD = 0
PACKETS_DROPPED = 0

initialTime = time.time()
ser = serial.Serial('COM13', baudrate=115200)
ser.flushInput()
entry = [0,0,0]

#clear the output files
try:
    os.remove("voltage_data.csv")
    os.remove("current_data.csv")
except:
    pass
while True:
    ser_bytes = ser.read(4)
    PACKETS_RECVD += 1
    comb_val = int.from_bytes(ser_bytes, "big")
    #break out of loop if we hit button one on the remote
    if(comb_val == 0x20DF8877):
        break
    if(comb_val >> 24) == EOFPACKET:
        break
    if(comb_val >> 24 ) == TIMEPACKET:
        comb_val = comb_val & 0x00FFFFFF
        if(DEBUG):
            print("Received time value: ",comb_val)
        if(STATE == TIMEPACKET):
            entry[2] = comb_val
            STATE = WRITETOFILE
        else:
            print("dropped a packet")
            PACKETS_DROPPED += 1
            STATE = VOLTAGEPACKET
            entry.clear()
            entry = [0,0,0]
            
    elif(comb_val >> 24) == VOLTAGEPACKET:
        comb_val = comb_val & 0x00FFFFFF
        if(DEBUG):
            print(hex(comb_val))
        #convert from 24bit value (0-5V) to float 
        comb_val = float(comb_val)
        comb_val = comb_val/16777215.0
        comb_val = comb_val*5.0
        if(DEBUG):
            print("Received voltage value: ",comb_val)
        if(STATE == VOLTAGEPACKET):
            entry[0] = comb_val
            STATE = CURRENTPACKET
        else:
            print("dropped a packet")
            PACKETS_DROPPED += 1
            STATE = VOLTAGEPACKET
            entry.clear()
            entry = [0,0,0]
        
    elif(comb_val >> 24) == CURRENTPACKET:
        comb_val = comb_val & 0x00FFFFFF
        #convert from 24bit value (0-5V) to float 
        comb_val = float(comb_val)
        comb_val = comb_val/16777215.0
        comb_val = comb_val*1000.0
        if(DEBUG):
            print("Received current value: ",comb_val)
        if(STATE == CURRENTPACKET):
            entry[1] = comb_val
            STATE = TIMEPACKET
        else:
            print("dropped a packet")
            PACKETS_DROPPED += 1
            STATE = VOLTAGEPACKET
            entry.clear()
            entry = [0,0,0]

    if(STATE == WRITETOFILE):
        if(DEBUG):
            print("writing to file")
        print(entry)
        STATE = VOLTAGEPACKET
        with open("voltage_data.csv","a", newline = '') as f:
            writer = csv.writer(f,delimiter = ",")
            writer.writerow([entry[2],entry[0]])
        with open("current_data.csv","a", newline = '') as f:
            writer = csv.writer(f,delimiter = ",")
            writer.writerow([entry[2],entry[1]])
        entry.clear()
        entry = [0,0,0]
endTime = time.time()
print()
print("Packets Received: ", PACKETS_RECVD)
print("Packets Dropped: ", PACKETS_DROPPED*3)
print("Total KB recieved: ", round(PACKETS_RECVD*4/1000.0,2))
print("Total Transmission Time: ",endTime-initialTime)
print("kbits/s: ", round(((PACKETS_RECVD * 96 * 8)/(endTime-initialTime))/1000.0,2))
print("Connection Quality: ",round((PACKETS_RECVD/(PACKETS_RECVD+PACKETS_DROPPED))*100,2),"%")
voltages = []
times = []                          
#Now we graph our CSV file with matlab
with open("voltage_data.csv","r") as voltageFile:
    lines = csv.reader(voltageFile,delimiter=',')
    for row in lines:
        times.append(int(row[0])/1000/60)
        voltages.append(float(row[1]))

plt.scatter(times, voltages, color = 'r', s = 100)
plt.xticks(rotation = 25)
plt.xlabel('Time (minutes)')
plt.ylabel('Voltage (V)')
plt.title('Charge Voltage vs Time', fontsize = 20)
plt.show()

currents = []
times = []
with open("current_data.csv","r") as currentFile:
    lines = csv.reader(currentFile,delimiter=',')
    for row in lines:
        times.append(int(row[0])/1000/60)
        currents.append(float(row[1]))

plt.scatter(times, currents, color = 'g', s = 100)
plt.xticks(rotation = 25)
plt.xlabel('Time (minutes)')
plt.ylabel('Current (mA)')
plt.title('Charge Current vs Time', fontsize = 20)
plt.show()
                            
        
                            
        

   
            
            
            

