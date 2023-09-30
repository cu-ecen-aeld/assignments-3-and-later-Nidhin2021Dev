#AARCH_64_CC = aarch64-none-linux-gnu-gcc
CC = aarch64-none-linux-gnu-gcc
#CC = gcc

CFLAGS = -Wall

SRC = writer.c
EXEC = writer 
OBJ = $(SRC:.c=.o)
$(EXEC):$(SRC)
	$(CC) $(CFLAGS) -c $(SRC) -o $(OBJ)
	$(CC) $(OBJ) -o $(EXEC)

#CROSS_COMPILE:$(SRC)
#	$(AARCH_64_CC) $(CFLAGS) -c $(SRC) -o $(OBJ)
#	$(AARCH_64_CC) $(OBJ) -o $(EXEC)
clean:
	rm -f writer
