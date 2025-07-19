# readme

Made by Wyatt Richards

This is a dispenser for _TREMCO POLYSHIM II_ window wedge (specifically the 0.160" size). It is designed to work with a NEMA 23 closed-loop stepper motor with approximately 3.0 Nm of maximum output torque. Bearings are standard 802 22mm OD/ 8mm ID ones with a smaller bearing on the giullotine assembly specified as having 7mm OD / 3mm ID.

Oversized holes are for machine screw heat-set inserts (M3 and M4 sizes) with three pockets for M5 nuts on the motor housing assembly.

Electrical diagram(s) for this machine are not provided but are pretty straight forward. It is designed to use 20 vdc input from Dewalt brand batteries, which is split into three different voltages and are as follows:
* 20 to 35 vdc step up module for 35 vdc input to the stepper driver
* 7 vdc regulator for servo power
* 5 vdc for Raspberry PI Pico input (note that the RPI has an internal voltage regulator that brings it down to 3.3 vdc, thus all logic and LEDs are running on 3.3 volts and not 5)

Pin configurations are designed for ease of assembly and maintenance and are hard-coded into the .ino file, which is compiled using the standard arduino-cli package for Linux and uploaded manually to the RPI.

Please consult the instruction manual with questions, otherwise you can email me at wr1701@proton.me
