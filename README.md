<h1>Vivarium Controller</h1>
<h2>Current Implementation</h2>
<h3>EEPROM</h3>
EEPROM addresses are in use as follows.<br />
<br />
0 - 5, xCalM<br />
6 - 10, yCalM<br />
11 - 17, xCalC<br />
18 - 23, yCalC<br />
24, WiFiState<br />
25, NTPState<br />
26 - 57, ssid<br />
58 - 90, password<br />
91, Number of used Slave ID's<br />
<br />
<br />
<h3>Modbus</h3>
Register 1 = Slave ID<br />
Register 2 = Slave Capeability<br />
Register 3 = Sensor Error Codes (Slave Dependant)<br />
Register 4 = Total Modbus Errors<br />
Register 5+ = Data (Slave Dependant)<br />
<br />
<h4>Slave ID</h4>
The slaves all start off with this register set to 1 and an ID of 1. When the controller discovers a new slave with ID 1 it has the ability to create a message box saying a new slave has been detected.  This is currently disabled as the master is not yet setup to change the slave ID so i am using Slave ID1 (default) for testing untill i implement this code.<br />
<br />
<h4>Slave Capeabilities</h4>
Implemented on slaves as below and read by master when a new slave is detected (currently disabled, see above) Master does not do anything with this currently.<br />
<br />
00 = Not Detected<br />
01 - 05 =  Unassigned<br />
06 =  1 Qty DHT22 Temperatre & Hummidity sensor<br />
07 - 81 =  Unassigned<br />
82 =  2 Qty 10A 250VAC relays<br />
83 - FF =  Unassigned<br />
<br />
<br />
<br />
<h2>Intended Opperation (WIP - The current code is not currently implemented as below)</h2>
<h3>EEPROM</h3>
EEPROM addresses are in use as follows.<br />
<br />
0 - 5, xCalM<br />
6 - 10, yCalM<br />
11 - 17, xCalC<br />
18 - 23, yCalC<br />
24, WiFiState<br />
25, NTPState<br />
26 - 57, ssid<br />
58 - 90, password<br />
91, Number of used Slave ID's<br />
<br />
<br />
<h3>Modbus</h3>
Register 1 = Slave ID<br />
Register 2 = Slave Capeability<br />
Register 3 = Sensor Error Codes (Slave Dependant)<br />
Register 4 = Total Modbus Errors<br />
Register 5+ = Data (Slave Dependant)<br />
<br />
<h4>Slave ID</h4>
Slaves have to be added one at a time and the controller periodically polls Slave ID 1 to see if a new slave has been connected. The slaves all start off with this register set to 1 and an ID of 1. When the controller discovers a new slave it checks to see what the next free slave ID is and sets this new slave ID in this register. The slave then stores the new Slave ID in EEPROM and will use this the next time it is power cycled.  Slaves will have a reset button to enable this and any other stored settings to be erased ready for pairing to a new controller.<br />
<br />
<h4>Slave Capeabilities</h4>
The slave capeabilities are assigned to this register as follows. <br />
<br />
00 = Not Detected<br />
01 - 05 =  1 to 5 Qty DS18B20 Temperature sensors<br />
06 - 0A =  1 to 5 Qty DHT22 Temperatre & Hummidity sensors<br />
0B - 80 =  Unassigned<br />
81 - 85 =  1 to 5 Qty 10A 250VAC relays<br />
86 - 90 =  1 to 5 Qty 10A 250VAC relays with current reading<br />
91 - FF =  Unassigned<br />
