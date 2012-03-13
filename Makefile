TARGET = tvbgone
SRC = main.c codes.c

CC = msp430-gcc

CFLAGS = -Wall -Os -g -mmcu=msp430g2452 -DF_CPU=8000000 -DNA_CODES=1 -DEU_CODES=1

#LDFLAGS = -Wl,-Map=$(TARGET).map,--cref

#OBJ = $(SRC:.c=.o) $(ASRC:.S=.o) 

all: $(TARGET)

$(TARGET): $(SRC:.c=.o)
	$(CC) $(SRC:.c=.o) -o $@

clean:
	rm -f $(TARGET) $(SRC:.c=.o)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

