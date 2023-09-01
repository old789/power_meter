# power_meter
Collection of power consumption data using ESP8266 with the PZEM004v03 module and visualise them.
~~It was done in a hurry.~~ Rebuilt.

Now the probe has an OLED display and can function independently like a simple powermeter.
Configuration of the probe can be made using a simple command line over a serial connection. 
A module PZEM004v03 now connected over hardware serial because softserial was bad with WiFi ( the probe had random reboots ) therefore a debugging console moved to the second serial ( output only ).

So, a probe can operate in 3 modes:
1. A simple powermeter ( switches S1 and S2 are off ).
2. A powermeter with collection data and sends it over WiFi to remote server ( S1 is off and S2 is on ).
3. Configuration over a serial connection ( S1 is on, S2 is anything ).
 
