CC = mpicc
CFLAGS = -Wall -Wextra -std=c11 -O2
MPIFLAGS = -np 5
TARGET = interseccion_mpi
SRC = interseccion_mpi.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

run: $(TARGET)
	mpirun $(MPIFLAGS) ./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean run
