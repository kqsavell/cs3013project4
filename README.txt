Kyle Savell & Antony Qin
CS3013 Operating Systems
Project 4: Simulated Virtual Memory

Files:
p4.c - C file containing the code for the virtual memory.
p4 - Executable file that runs the virtual memory simulation.
Makefile - Compiles p4.c into p4.
disk.txt - The simulated disk for the memory.
test.txt - Test input for p4.

Summary:
This program simulates virtual memory by first having an underlying physical memory (a 64-byte array) which is split into 4 pages of 16 bytes each. For each process that uses the physical memory, any number of virtual pages can be used and mapped to any of the physical pages using a page table, which itself takes up a physical page. Processes implement virtual addresses, which are dependent on the virtual pages of the process, thus meaning that virtual addresses could be implemented anywhere in the physical memory depending on the mapping.
To use this program, there are three main instructions that are used; map, store and load:
Map - Creates a page table for a process if one does not exist, and maps a virtual page to a physical page. Write permissions for a virtual page are also implemented with this function.
Store - Stores an integer at the supplied virtual address of a process.
Load - Loads a value from the supplied virtual address of a process.

Running the Program:
In the command line, "./p4 [process] [intruction] [address] [value]" will run the program. "process" is the process number that the specific instruction line will use (0-3), "instruction" is the instruction that will be executed (map, store or load), "address" is the virtual address that will be used for the instruction and process, and "value" is the page permission for map (0 = read only, 1 - read and write), the value to put in memory for store (1-255), and is unused for load.
Alternatively, multiple instruction lines can be piped in using a text file. Ex: "./p4 < test.txt"

Testing:
Testing was done with "test1.txt", "test2,txt" and "test3.txt". We piped these files into p4 to run multiple instructions back-to-back. We mainly tested the program against the example instructions that were shown in the rubric, as tested by "test1.txt". "test2.txt" tests edge cases where errors should occur. "test3.txt" tests the case where 4 processes are active at once. The output of these tests can be found in the files "test1_output.txt", "test2_output.txt" and "test3_output.txt".
