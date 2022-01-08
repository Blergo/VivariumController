<h1>Vivarium Controller</h1>
<h2>Modules</h2>
https://github.com/Blergo/Vivariumcontroller---2Relay <br />
https://github.com/Blergo/VivariumController---DHT22-Slave <br />
https://github.com/Blergo/VivariumController---3PortHub <br />
<br />
<br />
<h2>EEPROM</h2>
EEPROM addresses are in use as follows.<br />
<br />
0 - 3, xCalM<br />
4 - 7, yCalM<br />
8 - 11, xCalC<br />
12 - 15, yCalC<br />
16, WiFiState<br />
17, NTPState<br />
18 - 49, ssid<br />
50 - 81, password<br />
82 - 83, Number of used Slave ID's<br />
<br />
<br />
<h2>Modbus</h2>
Register 1 = Slave ID<br />
Register 2 = Slave Capeability<br />
Register 3 = Sensor Error Codes (Slave Dependant)<br />
Register 4 = Total Modbus Errors<br />
Register 5+ = Data (Slave Dependant)<br />
<br />
<h3>Slave ID</h3>
The slaves all start off with this register set to 1 and an ID of 1. There is a Pair Slave button in the slave settings manu item that scans for slaves with ID1 and when one is found the slave ID is set to the next available ID (Current Slaves +1) by updating the slaves ID Register and the Current slaves Value is updated. The slave will detect the change in the slave ID register and save this value to EEPROM before triggering a reboot to apply the new Slave ID. <br />
<br />
<h3>Slave Capeabilities</h3>
Implemented on slaves as below and read by master when a new slave is detected (currently disabled, see above) Master does not do anything with this currently.<br />
<br />
00 = Not Detected<br />
01 - 05 =  Unassigned<br />
06 =  1 Qty DHT22 Temperatre & Hummidity sensor<br />
07 - 81 =  Unassigned<br />
82 =  2 Qty 10A 250VAC relays<br />
83 - FF =  Unassigned<br />
