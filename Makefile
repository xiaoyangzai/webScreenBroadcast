CC=gcc

SRCS=$(wildcard *.cpp *.c)
TOBJS= $(patsubst %.cpp,%.o,$(SRCS))
OBJS= $(patsubst %.c,%.o,$(TOBJS))
LDFLAGS=-lcrypto


TARGET=server
all: $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)


clean:
	@rm *.o $(TARGET) -f
