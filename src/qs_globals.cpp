#include "../include/qs_globals.hpp"

// Define the global variables
WaitCondition WC_NEED_MORE_DATA;
WaitCondition WC_FILE_FIFO_WRITE;

Map<int, double> SMETERCORRECTMAP;
double SMETERCORRECT = 0.0;
