#include "lamport.h"
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

const unsigned long MSG_MAX_SIZE = 1024;

pthread_mutex_t clk_mutex = PTHREAD_MUTEX_INITIALIZER;

int lamport_send_to_all(void const* data, unsigned long len, MPI_Datatype dtype, int tag, unsigned long* clock, int size, int my_rank) {
  pthread_mutex_lock(&clk_mutex);

  int pos = 0;
  void *send_buf = malloc(MSG_MAX_SIZE);
  MPI_Comm comm = MPI_COMM_WORLD;

  MPI_Pack(clock, 1, MPI_UNSIGNED_LONG, send_buf, MSG_MAX_SIZE, &pos, comm);
  MPI_Pack(data, len, dtype, send_buf, MSG_MAX_SIZE, &pos, comm);

  int i;
  for (i = 0; i < size; i++) {
    if (i != my_rank) {
      int ret = MPI_Send(send_buf, pos, MPI_PACKED, i, tag, comm);
      if (ret) {
        return ret;
      }
    }
  }

  (*clock)++;
  pthread_mutex_unlock(&clk_mutex);
  return 0;
}



int lamport_send(void const* data, unsigned long len, MPI_Datatype dtype, int dest,
    int tag, MPI_Comm comm,
    unsigned long* clock, send_fun send_f) {
  pthread_mutex_lock(&clk_mutex);

  int pos = 0;
  void *send_buf = malloc(MSG_MAX_SIZE);

  MPI_Pack(clock, 1, MPI_UNSIGNED_LONG, send_buf, MSG_MAX_SIZE, &pos, comm);
  MPI_Pack(data, len, dtype, send_buf, MSG_MAX_SIZE, &pos, comm);

  int ret = send_f(send_buf, pos, MPI_PACKED, dest, tag, comm);

  (*clock)++;
  pthread_mutex_unlock(&clk_mutex);
  return ret;
}

int lamport_recv(void* data, unsigned long len, MPI_Datatype dtype, int source,
    int tag, MPI_Comm comm, MPI_Status* status,
    unsigned long* clock, recv_fun recv_f) {
  unsigned long msg_clock;
  return lamport_recv_clk(data, len, dtype, source, tag, comm, status, clock, &msg_clock, recv_f);
}

int lamport_recv_clk(void* data, unsigned long len, MPI_Datatype dtype, int source,
    int tag, MPI_Comm comm, MPI_Status* status,
    unsigned long* clock, unsigned long* msg_clock, recv_fun recv_f) {
  pthread_mutex_lock(&clk_mutex);

  int pos = 0;
  void *recv_buf = malloc(MSG_MAX_SIZE);

  int ret = recv_f(recv_buf, MSG_MAX_SIZE, MPI_PACKED, source, tag, comm, status);
  if (ret < 0)
    goto recv_err;

  MPI_Unpack(recv_buf, MSG_MAX_SIZE, &pos, msg_clock, 1, MPI_UNSIGNED_LONG, comm);
  MPI_Unpack(recv_buf, MSG_MAX_SIZE, &pos, data, len, dtype, comm);

  *clock = ulmax(*clock, *msg_clock) + 1;
recv_err:
  pthread_mutex_unlock(&clk_mutex);
  return ret;
}

unsigned long ulmax(unsigned long a, unsigned long b) {
  return a > b ? a : b;
}
