TARGET = tvbgone
SRC = main.c codes.c

CC = msp430-gcc

CFLAGS = -mmcu=msp430g2452 -Wall -Os -g \
-DF_CPU=8000000 -DNA_CODES=0 -DEU_CODES=1

LDFLAGS = -Wl,-Map=$(TARGET).map

all: $(TARGET)

$(TARGET): $(SRC:.c=.o)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRC:.c=.o) -o $@

clean:
	rm -f $(TARGET) $(SRC:.c=.o) $(TARGET).map

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

