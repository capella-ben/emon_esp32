# emon_esp32
Implementaion of emonlib for esp32 using the Adruino toolchain.

This is a basic copy of the calcIrms function from emonlib.  I had to do this because there was no way to easily set the ADC bits using the emonlib.

I also found that not all the ESP32 ADC's behave the same.

This sketch calculates the power usage and cisplays it on a web page.  No complex JSON, or XML, it just displays a simple tab seperated file.  Heaps easier to actually use  :-)

You need to write the config setting using the Write_Config_to_NVS.ino sketch first.  

This link shows you the circuit you need fro interfacing to a current transformeer:  https://openenergymonitor.org/emon/buildingblocks/ct-sensors-interface

This is a work in progress.

