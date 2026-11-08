CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2
LDFLAGS = -lm

TARGET = csvstat
SRCS = main.c csv.c table.c output.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
