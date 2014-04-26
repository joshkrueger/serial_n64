==================================
Arduino to Nintendo 64 Adapter
==================================


Materials
=========

* Nintendo 64

* Computer

* An Arduino. I used the UNO with an AtMega328 running at 16MHz. A
  different chip may have different timings, and a different speed will most
  certainly require modifications to the timing code.

* Two wires to connect arduino with nintendo 64

Quick Setup
===========
To just get things up and running, here's what to do:

Hooking the N64 to the Arduino
------------------------------
The N64 controller cord had 3 wires: ground, +3.3V, and data. The pin-out is shown in Figure 1 (left).

1. +3.3V (red) - connect to nothing

2. Data (white) - connect to Arduino digital I/O 8

3. GND (black) - connect to Arduino ground

The wire colors may or may not vary depending on the model, I would check that
the wire colors match the pin-out before you connect them up.

.. figure:: https://github.com/maxpowel/Wii-N64-Controller/raw/master/connections.png
    :alt: Gamecube and N64 controller connections

    Figure 1: Pin numbers for an N64 plug (left) and a Gamecube socket (right).
    Credit to this diagram goes to the `Cube64 project`_.

Compiling the Code
------------------
``wii_n64.pde`` provided is the entire source. You can open it with the
Arduino IDE and compile it, that should work.

Running it
----------
Once the code is loaded on the Arduino, and everything is hooked up, it's ready
to use. When the Arduino first powers up, it waits for a signal from the N64.
**Therefore, the Arduino must be powered on and ready (wait a couple seconds)
before the N64 is turned on**. Then turn the N64 on and it should be good to go.

If you turn the N64 off to e.g. load a new game, you'll need to reset the
Arduino. Just hit the reset button when the N64 is off before you turn it back
on.

Data Enconding
==============
Ensure that your script send 2 unsigned bytes for the controller buttons
and other 2 signed bytes for the stick position

One bit per button in the following order (2 first bytes):
A, B, Z, Start, Dup, Ddown, Dleft, Dright, 0, 0, L, R, Cup, Cdown, Cleft, Cright

You can check the example to see how it works in python


Signaling
---------
The protocol is simple, it uses low pulses of either 1μs or 3μs to indicate a 1 bit or 0 bit respectively. Bits come in every 4μs, so a 1 bit is 1μs low followed by 3μs high.

This microsecond timing is no problem for the AtMega328, but it does cut it kind of close. At 16MHz I get exactly 16 clock cycles per microsecond. Which is for the most part plenty, but one code path where the loops iterate on a byte boundary with a 1μs budget takes exactly 16 cycles.

Coding
------
I coded the entire signaling routine (sending and receiving) in C, and then analyzed the assembly output, calculated the number of cycles each branch took using the `AVR Instruction Set manual`_, and added in the necessary number of "nop" instructions. Then re-compiled and tested.

.. _AVR Instruction Set manual: http://www.atmel.com/dyn/resources/prod_documents/doc0856.pdf

After some trial and error, I was successfully sending and receiving commands from a gamecube controller. The N64 was easy after that, since they used the same encoding.

Resources
=========
* The `Cube64 Project`_
* `Gamecube Controller Protocol information`_
* `Nintendo 64 Controller Protocol information`_
* `N64/Gamecube to USB adapter Project`_ had some code that was useful as a reference
* `N64 to GameCube conversion project`_ (not sure why anyone would want to go in this direction)
* `Gamecube-N64-Controller`_
* `Use an Arduino with an N64 controller`_


.. _Cube64 Project: http://cia.vc/stats/project/navi-misc/cube64
.. _Gamecube Controller Protocol information: http://www.int03.co.uk/crema/hardware/gamecube/gc-control.htm
.. _Nintendo 64 Controller Protocol information: http://www.mixdown.ca/n64dev/
.. _N64/Gamecube to USB adapter Project: http://www.raphnet.net/electronique/gc_n64_usb/index_en.php
.. _N64 to GameCube conversion project: http://www.raphnet.net/electronique/x2wii/index_en.php
.. _Gamecube-N64-Controller: https://github.com/brownan/Gamecube-N64-Controller
.. _Use an Arduino with an N64 controller: http://www.instructables.com/id/Use-an-Arduino-with-an-N64-controller/step4/Arduino-Code-in-Depth/
