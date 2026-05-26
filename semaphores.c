#include <unistd.h> // For usleep
#include <stdlib.h> // For rand_r
#include <stdio.h>  // For printf
#include <mpi.h>    // For OpenMPI

#define N_VEHICULOS     10
#define TAG_SOLICITUD   1
#define TAG_PERMISO     2
#define TAG_CRUCE_FIN   3
#define TAG_ESTADISTICA 4

#define DELAY_MIN 10000 // 10 ms
#define DELAY_MAX 50000 // 50 ms

void simular_cruce(int rank, int size) {
    unsigned int semilla = rank + 1 * size);
    unsigned int tiempo_cruce = rand_r(&semilla) % (DELAY_MAX - DELAY_MIN) + DELAY_MIN;
    usleep(tiempo_cruce);

}

void mpi_run(int *argc, char ***argv) {
    MPI_Init(argc, argv);
    int rank, size;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    simular_cruce(rank, size);
    MPI_Finalize();
}

int main(int argc, char *argv[]) {
    mpi_run(&argc, &argv);
    return 0;
}