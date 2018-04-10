# Laser-Scanner-Controller---SUPER-RES-ATOM---TEENSY3.X

This is code written for the Teensy:

* Teensy 3.6: 180 MHz Cortex-M4F
* Teensy 3.5: 120 MHz Cortex-M4F
* Teensy 3.2 (3.3V regulator) and 3.1: 72 MHz Cortex-M4
* Teensy LC : 48 MHz Cortex-M0+

NOTE: On Teensy Teensy 3.5 and 3.6 the native DACs are on pins A21 and A22), A14 on the Teensy 3.1/3.2, and A12 on the Teensy LC

This code implements:

	-  A very robust serial parser with human readable console output;
	-  Double display buffering using interruptions capable of setting the inter-point interval up to about 10 microseconds/point (mais je dois tester si les mirroirs suivent et ajuster les delais necessaires)
	- OpenGL-like 2D renderer capable of applying rotation, scaling and translations;
	- Un ensemble de "graphic primitives" comprising lines, circles, rectangles, squares (je suis en train de faire le "zigzag" qui pourra etre adaptatif de facon interactive (i.e., changer la region d'interet, et la resolution);
	- Ces primitives peuvent etre composes a la suite pour creer des figures arbitraires;

Alvaro Cassinelli
Paris, 10 April 2018
