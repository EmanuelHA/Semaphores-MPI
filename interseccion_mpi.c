#include <threads.h>  // thrd_sleep
#include <stdlib.h>   // rand, srand
#include <string.h>   // strlen
#include <stdio.h>    // printf
#include <time.h>     // time
#include <mpi.h>      // MPI

#define N_VEHICULOS     10
#define N_CARRILES      4

#define TAG_SOLICITUD   1
#define TAG_PERMISO     2
#define TAG_CRUCE_FIN   3
#define TAG_ESTADISTICA 4

#define RANK_MASTER     0
#define RANK_NORTE      1
#define RANK_SUR        2
#define RANK_ESTE       3
#define RANK_OESTE      4

#define BUFF_LEN        256

const char *NOMBRE_RANK[] = {
    "Coordinador",
    "Norte", 
    "Sur", 
    "Este", 
    "Oeste"
};

int vehiculos_cruzados;
int carriles_vaciados; // se usará en cada iteración para acumular el estado finalizado de cada carril usando reduce
int accidentes;
double espera_acumulada;
double tiempo_simulacion;

#define DELAY_MIN 1500000   // 1.5 ms en nanosegundos
#define DELAY_MAX 5000000   // 5 ms en nanosegundos

void delay(long nanosegundos) {
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = nanosegundos;
    
    thrd_sleep(&ts, NULL);
}

void simular_cruce(int rank, int size) {
    srand((unsigned int)(time(NULL) + (rank << (rank + size))));
    long tiempo_cruce_ns = (long)(rand() % (DELAY_MAX - DELAY_MIN) + DELAY_MIN);

    delay(tiempo_cruce_ns);
}



void fase_uno(int rank) {
    if (rank == RANK_MASTER) {
        printf("[%s] Sistema iniciado. N_VEHICULOS=%d distribuido a 4 carriles.\n", 
               NOMBRE_RANK[rank], N_VEHICULOS);

        for (int i = 1; i < 5; i++) {
            char buffer[256];
            MPI_Recv(buffer, BUFF_LEN, MPI_CHAR, i, TAG_SOLICITUD, 
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("%s", buffer);
        }
    }
    else {
        const char* msg = "Listo. Esperando autorización para cruzar.\n";
        size_t msg_len = strlen(msg) + 1; // + 1 para '\0' implicito en msg
        MPI_Send((void*)msg, (int)msg_len, MPI_CHAR, RANK_MASTER, 
                 TAG_SOLICITUD, MPI_COMM_WORLD);
    }
}

void fase_dos(int rank, int size){
    if (rank == RANK_MASTER) {
        printf("======================================================\n");
        printf("     SIMULADOR DE INTERSECCION DE TRAFICO CON MPI\n");
        printf("   Procesos: %d | Carriles: 4 | Vehiculos/carril: %d\n", size, N_VEHICULOS);
        printf("======================================================\n");
        printf("[%s] Parametros distribuidos. Iniciando...\n", NOMBRE_RANK[rank]);
        
        while(carriles_vaciados != N_CARRILES) {
            carriles_vaciados = N_CARRILES;
        }
    } else {
        
    }
}

void correr_fases(int *argc, char **argv[]) {
    MPI_Init(argc, argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != 5) {
        if (rank == RANK_MASTER) {
            fprintf(stderr, 
                "Error: Se requieren EXACTAMENTE 5 procesos.\n  - Ejecutando con %d procesos.", size);
        }
        MPI_Finalize(); // Usar MPI_Abort(...) como sugiere la tarea causa error
        exit(EXIT_FAILURE);
    }
    
    fase_uno(rank);
    fase_dos(rank, size);
    
    MPI_Finalize();
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    correr_fases(&argc, &argv);
    return 0;
}