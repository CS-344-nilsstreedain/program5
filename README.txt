Assignment 5: Multi-threaded Producer Consumer Pipeline

Write a program that creates 4 threads to process input from standard input as follows
- Thread 1, called the Input Thread, reads in lines of characters from the standard input.
- Thread 2, called the Line Separator Thread, replaces every line separator in the input by a space.
- Thread 3, called the Plus Sign thread, replaces every pair of plus signs, i.e., "++", by a "^".
- Thread 4, called the Output Thread, write this processed data to standard output as lines of exactly 80 characters.

Example usage:
1. Compile the program with gcc --std=gnu99 -o line_processor main.c -lpthread.
2. Run the program with ./line_processor.
3. Provide input to the program, and it will print the processed output.
