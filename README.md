This project involves the development of an assembler for assembly language programming, written in C. The primary goal is to understand how an assembler converts assembly language code into machine code that can be executed by a CPU. 
The project covers the pre-processing, first pass, and second pass stages of assembling code, handling macros, and managing code instructions and data directives.
During pre-processing, macros are identified and handled by adding them to a macro table and replacing them in the source file.
During the first pass, labels and directives are processed, and the instruction and data counters are updated.
During the second pass, the actual machine code is generated based on the processed information from the first pass.
The project aims to provide a comprehensive understanding of how assembly code is translated into machine code and executed by the CPU.
Additional instructions include handling different segments of code and data, entry and extern directives, and linking the assembled code correctly.
