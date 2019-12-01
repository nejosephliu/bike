Hall Switch
=====

Uses the hall effect [sensor/switch](http://wiki.sunfounder.cc/index.php?title=Hall_Sensor_module) and GPIOTE to detect a magnet.  The LED on the nrf52 (and the LED on the switch itself) should toggle depending on the magnet.  Additionally prints the global state upon each transition.

Hardware Connections:  
Connect hall sensor power and ground to NRF 5V and ground.  
Connect hall sensor signal to NRF pin 15.

Based on the [button and switch](https://github.com/lab11/buckler/blob/master/software/apps/button_and_switch/main.c) app from the Buckler repository.
