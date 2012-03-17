TARGET = tvbgone
SRC = main.c sync.c codes.c

CC = msp430-gcc

CFLAGS = -mmcu=msp430g2452 -Wall -Os -g \
-DF_CPU=7999488 -DNA_CODES=0 -DEU_CODES=1 \
-ffunction-sections -fdata-sections

LDFLAGS = -Wl,-Map=$(TARGET).map,--gc-sections

all: $(TARGET)

$(TARGET): $(SRC:.c=.o)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRC:.c=.o) -o $@

%.o: %.c %.h
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f $(TARGET) $(SRC:.c=.o) $(TARGET).map

prog: $(TARGET)
	mspdebug rf2500 "prog $(TARGET)"

