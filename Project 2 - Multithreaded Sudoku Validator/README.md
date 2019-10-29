# Project 2 - Multithreaded Sudoku Validator

A multi-threaded Sudoku solution validator for analysis of threading performance. The program will read in an input file of a 9x9 grid of values and use different amounts of threads specified by the user to validate the grid as a Sudoku solution.

### Task

**Part 1 - Passing parameters to each thread**

• A parent thread creates each worker thread based on the option specified by the user:

1: 9 grid threads, 1 column thread, 1 row thread
2: 9 grid threads, 9 column threads, 1 row thread
3: 9 grid threads, 9 column threads, 9 row threads

• Sets of 9 threads will each individually validate their respective type (grid/row/column), while single threads will validate all of their respective type. Single threads have two possible methods of region validation:

Method 1: Perform a ```pthread_exit()``` when encountered by a single invalid region.
Method 2: Perform a ```break``` to the next region when encountered by a single invalid region.

• Each thread is assigned a ```struct``` of parameters that account for which row(s) and/or column(s) they are validating, as well as a reference to the saved sudoku board.

```
/* Create struct to pass into each thread as parameters */ 
typedef struct
{
int row;
int col;
int (*board)[9];
} parameters;
```

**Part 2 - Returning results to the parent thread**

• After each region of the board has been validated, it will pass its results to the parent in the form of changing a 0 to a 1 within a global array ```result[27]```. The parent thread checks this thread at the end to verify that every region is valid.

***Other***

• Code analysis and statistical analysis of each threading option/method can be found in ```report.pdf```.

