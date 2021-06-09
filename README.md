Description
-----------

GenDasm is a Generic Code-Seeking Diassembler engine.  It allows you to enter known starting vectors for a given code image for the micro.  It will disassemble the code and follow through branches to assist in the separation of code and data.

Its included Fuzzy Function Analyzer companion uses a DNA Sequence Alignment Algorithm to locate similar code in multiple binaries to facilitate reverse-engineering and/or code recovery.

The original purpose of the Fuzzy Function Analyzer was to assist in code recovery where the source code for the current binaries got lost, yet the source code for an old binary was retained.  The Fuzzy Function Analyzer allows you to match up known functions between the two binaries so you can concentrate on disassembling and reverse engineering the parts that are different and recover the code for the current binary that got lost.

The reason for fuzzy matching of the functions is because of absolute addresses causing differences in a normal diff even when the code is identical, making it otherwise difficult to discern which function is which.

This disassembler currently supports the M6811 micro and the AVR series of micros and can easily be expanded to include additional micros.

The origin of the tool started with the m6809dis and m6811dis code-seeking disassembler tools that I originally created when working with GM automotive engine controllers and with SuperFlo Dynamometers.


License
-------

GenDasm - Generic Code-Seeking Disassembler Engine

Copyright (c) 2021 Donna Whisnant, a.k.a. Dewtronics.

Contact: <http://www.dewtronics.com/>

