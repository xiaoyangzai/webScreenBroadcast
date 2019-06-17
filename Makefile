CC=gcc

SRCS=$(wildcard *.cpp *.c)
TOBJS= $(patsubst %.cpp,%.o,$(SRCS))
OBJS= $(patsubst %.c,%.o,$(TOBJS))
LDFLAGS=-lcrypto -ljpeg -L./libjpeg/lib -lX11 -lpthread
CFLAGS=-I./libjpeg/include


TARGET=server
all: $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)


clean:
	@rm *.o $(TARGET) -f
