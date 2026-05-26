#include <stdio.h>  // For printf
#include <mpi.h>    // For OpenMPI

void mpi_run(int *argc, char ***argv) {
    MPI_Init(argc, argv);
    int rank, size;


    MPI_Finalize();
}

void intersection_sim() {
    
}

int main(int argc, char *argv[]) {
    mpi_run(&argc, &argv);
    return 0;
}