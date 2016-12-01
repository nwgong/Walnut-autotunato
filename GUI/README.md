This is a quick guide for install and run the Walnut Actuator DEMO GUI application.

# Setup
- Go to the library directory to install necesasry libraries for this application.
- Before you start, please make sure you have the following environment on your platform:
  - [x] Python 2.7.x, in my computer, python 2.7.6 is installed and function right.
  - [x] PySerial for Python 2.7.x, this is included in the library sub-dir for windows, if Mac, please use pip tool to install.
  - [x] wxPython, the python GUI dev tool kit, a windows installation is also included.
  * please get above modules installed before start.
  
# Launch
 - The Walnut Actuator DEMO GUI is implemented using wxPython, it employed a simple dialog module for user to choose the right COM port 
 and setup correct baudrate,etc.
 - In order for the GUI to work correctly with the Teensy 3.2 board, please first choose the correct COM PORT of the Teensy on your system.
 Remember, leave the RTS/CTS, Xon/Xoff options both "un-checked".
 - Then, in the main GUI, select "File" -> "Terminal Settings" -> check "Local Echo" and check "LF Only", then, click OK.
 
 # Run the 12 Channel Actuator
 - Go to the menu "Actuator", first, select "HV Enable", the "set_hv command will be sent to the serial line to the Teensy module.
 - Once the "HV Enable" is checked, you can go to "Actuators", in the drop-down menu, choose any of the 12 Channel and click.
 - The coresponding "set_dac" command would be sent to the Teensy and in such a way, the Actuator will start to oscillate. 
