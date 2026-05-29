#define _DEFAULT_SOURCE

#include <mpi.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define N_VEHICULOS 10
#define N_CARRILES 4

#define TAG_SOLICITUD 1
#define TAG_PERMISO 2
#define TAG_CRUCE_FIN 3
#define TAG_ESTADISTICA 4

#define RANK_MASTER 0
#define RANK_NORTE 1
#define RANK_SUR 2
#define RANK_ESTE 3
#define RANK_OESTE 4

#define BUFF_LEN 256
#define MAX_COLA (N_CARRILES * (N_VEHICULOS + 1))

const char *NOMBRE_RANK[] = {
    "Coordinador",
    "Norte",
    "Sur",
    "Este",
    "Oeste"
};

typedef struct {
    int carril;
    int id;
} Solicitud;

typedef struct {
    Solicitud datos[MAX_COLA];
    int inicio;
    int cantidad;
} ColaSolicitudes;

static bool cola_vacia(const ColaSolicitudes *cola) {
    return cola->cantidad == 0;
}

static bool cola_llena(const ColaSolicitudes *cola) {
    return cola->cantidad == MAX_COLA;
}

static void encolar(ColaSolicitudes *cola, Solicitud solicitud) {
    if (cola_llena(cola)) {
        fprintf(stderr, "[Coordinador] Error: cola de solicitudes llena.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int posicion = (cola->inicio + cola->cantidad) % MAX_COLA;
    cola->datos[posicion] = solicitud;
    cola->cantidad++;
}

static Solicitud desencolar(ColaSolicitudes *cola) {
    Solicitud solicitud = cola->datos[cola->inicio];
    cola->inicio = (cola->inicio + 1) % MAX_COLA;
    cola->cantidad--;
    return solicitud;
}

static void imprimir_vehiculo(const Solicitud *solicitud) {
    printf("[%s-%03d]", NOMBRE_RANK[solicitud->carril], solicitud->id);
}

static void imprimir_encabezado(int size, int n_vehiculos) {
    printf("======================================================\n");
    printf("  SIMULADOR DE INTERSECCION DE TRAFICO CON MPI\n");
    printf("  Procesos: %d | Carriles: %d | Vehiculos/carril: %d\n",
           size, N_CARRILES, n_vehiculos);
    printf("======================================================\n");
    printf("\n");
    printf("[Coordinador] Parametros distribuidos. Iniciando...\n");
    fflush(stdout);
}

static void autorizar_cruce(const Solicitud *solicitud, bool desde_cola) {
    int permiso = solicitud->id;
    MPI_Send(&permiso, 1, MPI_INT, solicitud->carril, TAG_PERMISO,
             MPI_COMM_WORLD);

    printf("[Coordinador] ");
    imprimir_vehiculo(solicitud);
    if (desde_cola) {
        printf(" autorizado (era el siguiente en cola). Cruce OCUPADO.\n");
    } else {
        printf(" autorizado. Cruce OCUPADO.\n");
    }
    fflush(stdout);
}

static void simular_cruce(void) {
    usleep(2000u + (unsigned int)(rand() % 4000));
}

static int distribuir_parametros(int rank) {
    int n_vehiculos = (rank == RANK_MASTER) ? N_VEHICULOS : 0;
    MPI_Bcast(&n_vehiculos, 1, MPI_INT, RANK_MASTER, MPI_COMM_WORLD);
    return n_vehiculos;
}

static void fase_uno(int rank, int n_vehiculos) {
    if (rank == RANK_MASTER) {
        printf("[%s] Sistema iniciado. N_VEHICULOS=%d distribuido a %d carriles.\n",
               NOMBRE_RANK[rank], n_vehiculos, N_CARRILES);

        for (int carril = RANK_NORTE; carril <= RANK_OESTE; carril++) {
            char buffer[BUFF_LEN];
            MPI_Recv(buffer, BUFF_LEN, MPI_CHAR, carril, TAG_SOLICITUD,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("%s", buffer);
        }
        printf("\n");
        fflush(stdout);
    } else {
        char buffer[BUFF_LEN];
        snprintf(buffer, sizeof(buffer),
                 "[%-5s] listo. Esperando autorizacion para cruzar.\n",
                 NOMBRE_RANK[rank]);
        MPI_Send(buffer, (int)strlen(buffer) + 1, MPI_CHAR, RANK_MASTER,
                 TAG_SOLICITUD, MPI_COMM_WORLD);
    }
}

static void fase_dos_coordinador(int size, int n_vehiculos) {
    ColaSolicitudes cola = {0};
    Solicitud activo = {0};
    bool cruce_ocupado = false;
    bool carril_terminado[N_CARRILES + 1] = {false};
    int carriles_terminados = 0;
    int vehiculos_completados = 0;
    int total_esperado = N_CARRILES * n_vehiculos;

    imprimir_encabezado(size, n_vehiculos);

    while (carriles_terminados < N_CARRILES || cruce_ocupado ||
           !cola_vacia(&cola)) {
        if (!cruce_ocupado && !cola_vacia(&cola)) {
            activo = desencolar(&cola);
            cruce_ocupado = true;
            autorizar_cruce(&activo, true);
            continue;
        }

        int id = 0;
        MPI_Status status;
        int tag_esperado = cruce_ocupado ? MPI_ANY_TAG : TAG_SOLICITUD;

        MPI_Recv(&id, 1, MPI_INT, MPI_ANY_SOURCE, tag_esperado,
                 MPI_COMM_WORLD, &status);

        if (status.MPI_TAG == TAG_SOLICITUD) {
            int carril = status.MPI_SOURCE;

            if (id == -1) {
                if (!carril_terminado[carril]) {
                    carril_terminado[carril] = true;
                    carriles_terminados++;
                }
                continue;
            }

            Solicitud solicitud = {carril, id};
            imprimir_vehiculo(&solicitud);
            printf(" -> solicita cruce");

            if (cruce_ocupado || !cola_vacia(&cola)) {
                encolar(&cola, solicitud);
                printf(" (en cola: %d esperando)\n", cola.cantidad);
            } else {
                printf("\n");
                activo = solicitud;
                cruce_ocupado = true;
                autorizar_cruce(&activo, false);
            }
            fflush(stdout);
        } else if (status.MPI_TAG == TAG_CRUCE_FIN) {
            Solicitud finalizado = {status.MPI_SOURCE, id};
            imprimir_vehiculo(&finalizado);
            printf(" cruce completado. Cruce LIBRE.\n");
            fflush(stdout);

            vehiculos_completados++;
            cruce_ocupado = false;
        }
    }

    printf("\n[Coordinador] Fase 2 completa: %d / %d vehiculos cruzaron sin accidentes.\n",
           vehiculos_completados, total_esperado);
    fflush(stdout);
}

static void fase_dos_carril(int rank, int n_vehiculos) {
    srand((unsigned int)time(NULL) + (unsigned int)(rank * 97));

    for (int id = 1; id <= n_vehiculos; id++) {
        int permiso = 0;

        MPI_Send(&id, 1, MPI_INT, RANK_MASTER, TAG_SOLICITUD,
                 MPI_COMM_WORLD);
        MPI_Recv(&permiso, 1, MPI_INT, RANK_MASTER, TAG_PERMISO,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        if (permiso != id) {
            fprintf(stderr,
                    "[%s-%03d] Error: permiso inesperado recibido (%d).\n",
                    NOMBRE_RANK[rank], id, permiso);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        simular_cruce();
        MPI_Send(&id, 1, MPI_INT, RANK_MASTER, TAG_CRUCE_FIN,
                 MPI_COMM_WORLD);
    }

    int fin = -1;
    MPI_Send(&fin, 1, MPI_INT, RANK_MASTER, TAG_SOLICITUD, MPI_COMM_WORLD);
}

static void fase_dos(int rank, int size, int n_vehiculos) {
    if (rank == RANK_MASTER) {
        fase_dos_coordinador(size, n_vehiculos);
    } else {
        fase_dos_carril(rank, n_vehiculos);
    }
}

static void correr_fases(int *argc, char **argv[]) {
    MPI_Init(argc, argv);

    int rank = 0;
    int size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != 5) {
        if (rank == RANK_MASTER) {
            fprintf(stderr,
                    "Error: se requieren EXACTAMENTE 5 procesos. Ejecutando con %d procesos.\n",
                    size);
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int n_vehiculos = distribuir_parametros(rank);
    fase_uno(rank, n_vehiculos);

    MPI_Barrier(MPI_COMM_WORLD);
    fase_dos(rank, size, n_vehiculos);

    MPI_Finalize();
}

int main(int argc, char *argv[]) {
    correr_fases(&argc, &argv);
    return EXIT_SUCCESS;
}
