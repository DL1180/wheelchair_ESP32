*This is hard coded for GPIO pins 8 & 9.*

Connections:
Echo pin is connected to the top of the 1k resistor, Trig is connected to GPIO9, and GPIO8 is connected to the junction between the 1k and 2k resistors. Remember to ground the 2k resistor.

To run:
cd ~/esp_projects/wheelchair_esp32/ultrasonic_1
source ~/esp/esp-idf/export.sh
idf.py build
idf.py -p /dev/ttyACM0 flash monitor

To exit from monitor:
ctrl + ]