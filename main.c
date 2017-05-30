#include <unistd.h>
#include <stdio.h>
#include "lamport.h"
#include <time.h>
#include <stdlib.h>

void wait_sec(int min, int max) {
    unsigned int time = (rand() % (max - min)) + min;
    sleep(time);
}

int main(int argc, char** argv) {
  return 0;
}
