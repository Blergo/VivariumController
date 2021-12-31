**Current Implementation**<br />
<br />
**Modbus**<br />
Register 1 = Slave ID<br />
Register 2 = Slave Capeability<br />
Register 3 = Sensor Error Codes (Slave Dependant)<br />
Register 4 = Total Modbus Errors<br />
Register 5+ = Data (Slave Dependant)<br />
<br />
**Slave ID**<br />
The slaves all start off with this register set to 1 and an ID of 1. When the controller discovers a new slave with ID 1 it has the ability to create a message box saying a new slave has been detected.  This is currently disabled as the master is not yet setup to change the slave ID so i am using Slave ID1 (default) for testing untill i implement this code.<br />
<br />
**Slave Capeabilities**<br />
Implemented on slaves as below and read by master when a new slave is detected (currently disabled, see above) Master does not do anything with this currently.<br />
<br />
<br />
<br />
**Intended Opperation (WIP - The current code is not currently implemented as below)**<br />
<br />
**Modbus**<br />
Register 1 = Slave ID<br />
Register 2 = Slave Capeability<br />
Register 3 = Sensor Error Codes (Slave Dependant)<br />
Register 4 = Total Modbus Errors<br />
Register 5+ = Data (Slave Dependant)<br />
<br />
**Slave ID**<br />
Slaves have to be added one at a time and the controller periodically polls Slave ID 1 to see if a new slave has been connected. The slaves all start off with this register set to 1 and an ID of 1. When the controller discovers a new slave it checks to see what the next free slave ID is and sets this new slave ID in this register. The slave then stores the new Slave ID in EEPROM and will use this the next time it is power cycled.  Slaves will have a reset button to enable this and any other stored settings to be erased ready for pairing to a new controller.<br />
<br />
**Slave Capeabilities**<br />
The slave capeabilities are assigned to this register as follows. <br />
<br />
00 - 04 =  1 to 5 Qty DS18B20 Temperature sensors<br />
05 - 09 =  1 to 5 Qty DHT22 Temperatre & Hummidity sensors<br />
0A - 7F =  Unassigned<br />
80 - 84 =  1 to 5 Qty 10A 250VAC relays<br />
85 - 89 =  1 to 5 Qty 10A 250VAC relays with current reading<br />
86 - FF =  Unassigned<br />
