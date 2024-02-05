# DCLK_MK4

A shot clock system for competitve dodgeball. The system consists of two devices: display and controller.

##Status
This project is in progress...

###Controller
A first iteration PCB was order and successfully programmed. The firmware was tested with the MCU powered externally (without battery or linear regulator).

####Problem with Linear Regulator
The BLE functionality on the PCB was not functional when powered by battery. Trouble shooting revealed the linear regulator is not acceptable for the current requirement of the antenna. Voltage spike when the antenna turns on crash the MCU.
![VDD rail when antenna turned on](https://github.com/ebertmx/DODGECLOCK_MK4/assets/87283949/7e6d919e-b373-41c0-876a-24a9da3ff127)

A second iteration board is in progress to improve performance and fix the power delivery problems.

###Display
The display 7-segment frame was pritned with TPU and succesffuly passed all impact tests.
A nrf52 dongle will be used as the MCU for the display to save on time, costs, and assembly requirements.
The LED strip, power supply, and nrf52 dongle have been tested in a bench top environment succeffully.

![image](https://github.com/ebertmx/DODGECLOCK_MK4/assets/87283949/9de9cff6-2c16-4b95-a077-c568292037b8)

##Description
###Controller
The controller device includes
- A PCB and MCU with BLE capabilities
- An LCD
- Several user inputs
- a buzzer

The controller devices is held by the ref and used to control the shot clock count. 

The device acts as a peripheral/server. It accepts secure connections and provide clock and state information. This device is battery powered; thus, it tries to mimize power at all times. The device prioritizes the built in LCD and user controls over the BLE connection; consequently, the ref always knows the correct shot clock time and can control the state.

![image](https://github.com/ebertmx/DODGECLOCK_MK4/assets/87283949/3552f4a8-60b5-484d-9c1b-a054e468b64c)


###Display
The display device includes
- a WS2813B LED strip formed into a 7-segment pattern
- A PCB and MCU with BLE capablilities
- 2 user buttons for pairing and mode seletion
- A power supply

The display device is plugged into a wall outlet or similar power source and mounted within view of the court. The players and ref can reference this display. 

This device acts as a central/client. It initiates a secure connection with the controller and recieves time and state information if possible. If a connection is lost of cannot be estabilished it enters a dormant state and displays nothing. Otherwise, it displays the current count down time from the controller.


 
