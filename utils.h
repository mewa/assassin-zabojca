#pragma once

struct mpi_info {
  int rank;
  int size;
  unsigned long *clk;
  struct rating *ratings_arr;
}
