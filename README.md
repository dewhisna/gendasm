Description
-----------

GenDasm is a Generic Code-Seeking Diassembler engine.  It allows you to enter known starting vectors for a given code image for the micro.  It will disassemble the code and follow through branches to assist in the separation of code and data.

Its included Fuzzy Function Analyzer companion uses a DNA Sequence Alignment Algorithm to locate similar code in multiple binaries to facilitate reverse-engineering and/or code recovery.

The original purpose of the Fuzzy Function Analyzer was to assist in code recovery where the source code for the current binaries got lost, yet the source code for an old binary was retained.  The Fuzzy Function Analyzer allows you to match up known functions between the two binaries so you can concentrate on disassembling and reverse engineering the parts that are different and recover the code for the current binary that got lost.

The reason for fuzzy matching of the functions is because of absolute addresses causing differences in a normal diff even when the code is identical, making it otherwise difficult to discern which function is which.

This disassembler currently supports the M6811 micro and the AVR series of micros and can easily be expanded to include additional micros.

The origin of the tool started with the m6809dis and m6811dis code-seeking disassembler tools that I originally created when working with GM automotive engine controllers and with SuperFlo Dynamometers.


Target Assemblers
-----------------

The M6811DIS portion of this tool targets the [ASxxxx Cross Assemblers](https://shop-pdp.net/index.php) by Alan Baldwin at Kent State University.  Note that this assembler requires a patch to remove the direct addressing mode optimization.  That is, opcodes using addresses in the $0000-$00FF range can use a shorter version of the opcode and the assembler is smart enough to figure that out and optimize your code.  However, not all original binary files are properly optimized, and in order to roundtrip to the same non-optimized binary, the optimizer in the assembler must be removed or disabled.  See the corresponding patch in the support folder.

The AVR portion of this tool targets the [AVRA, Assembler for the Atmel AVR microcontroller family](https://github.com/Ro5bert/avra).

If you disassemble code on this disassembler for one of these platforms, you will be able to use the corresponding assembler to reassemble it back into an identical binary image file.  Roundtrip testing is part of the test suite.


History:
--------

The first version of m6809dis was written in 1992 in Borland Pascal in Windows and targeted the 6809.  That version was reworked for the 6811 in 1996 to create m6811dis, but still in Borland Pascal 7.0, with minor updates through 1999.  In May 2002, m6811dis was rewritten to C++ using MSVC++ 5.0, but still in Windows (the original m6809dis got abandoned).

The Fuzzy Function Analyzer was first invented in mid-2002, following the m6811dis port to C++, and was proven functional for several projects in February 2003.  In June 2014, I decided to finally open-source m6811dis and created [the first public version](https://github.com/dewhisna/m6811dis), after cleaning it up a bit, and making it work for Linux, but without the Fuzzy Function Analyzer (apart from legacy source code fragments) due to lack of time.

I had always intended m6811dis to get morphed into a generic-disassembly platform.  In June 2021, I found that I needed a Disassembler and Fuzzy-Function Analyzer for the AVR series micros.  So, I reworked the code again to modernize it to C++20 and bring the Fuzzy-Function Analyzer back from the dead.  I added AVR support.  Fixed numerous bugs.  Made things more generic in the support of big-endian vs. little-endian, Harvard vs Von Neumann architecture, RISC vs CISC, byte-opcodes vs word-opcodes, etc.  And thus [gendasm](https://github.com/dewhisna/gendasm) was born.


License
-------

GenDasm - Generic Code-Seeking Disassembler Engine

Copyright (c) 2021 Donna Whisnant, a.k.a. Dewtronics.

Contact: <http://www.dewtronics.com/>

