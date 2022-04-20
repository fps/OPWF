# Introduction

This repository contains firmware for a pickup winder based on a Ramps 1.4 board, RepRapDiscount Full Graphics Smart Controller, a 200 steps per revolution stepper motor, a servo and an Arduino Mega 2560 microcontroller board.

![Video showing the winder in action](https://www.youtube.com/shorts/mcej-a_-2Q8)

# Installation

- Assemble the pickup winder as can be seen in the picture below
- Use e.g. the Arduino IDE to flash the sketch in the src/ directory to the Mega 2560

![Assembled winder](https://github.com/fps/OPWF/blob/master/IMG_20220405_183842.jpg)

# Assembly

- 3d print the motor holder and pickup attachment plate (or think about an alternative way to secure the motor - I successfully used some blocks of wood and some double sided tape)
- Cut a piece of wood to approximately 20 cm by 20 cm that serves as the base
- Mount the arduino Mega 2560 in the back left corner using ca. two wood screws
- Mount the Ramps 1.4 board on top of the Arduino
- Attach at least the Z-direction stepper driver and set it to 4 microsteps using the jumpers before inserting the driver board.
- Attach the servo to the second servo pin from the left (it's the one corresponding to the digital pin D6 - this is important for the PWM and interrupt management)
- Secure the servo cable a little bit with a piece of tape so that it won't entangle with the pickup wire
- Attach the servo on a piece of wood to raise it up a little from the base board. Position it in such a way that the center of the servo axis aligns with the center of a pickup bobbin when attached to the motor
- At the front right attach two felt-padded pieces of wood with 2 screws. These serve as pickup wire tensioner. 
- Figure out a way to connect your stepper motor to the corresponding pins on the Ramps board
- Secure the stepper motor cables with some tape so that they don't entangle with the pickup wire
- Mount the display using two wood screws, but screw them in at an angle. The back of the display will be held by the flat cables.

# Usage

- Press the encoder button on the display to enter/leave a setting
- Rotate the encoder to change a setting. Since the display is so godawfully slow the code waits for a timeout until updating the display.
- Press the STOP button on the display to start the winding process
- Press the STOP button to stop the winding process, too ;)
- Press the STOP button twice (doubleclick) to reset the "Current Winds" counter to 0

# CAVEATS

- The button handling is done purely in software without interrupts, so sometimes presses are missed.

# LICENSE

- To be figured out

# Author

- Florian Paul Schmidt
- Florian Hofmann  

