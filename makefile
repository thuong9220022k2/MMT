CC = g++
CFLAGS = -c -I/usr/include/json-c
LIBS = -lsfml-graphics -lsfml-window -lsfml-system -lsfml-network -lpthread -ljsoncpp

# Object files
OBJS = client.o

# Executable name
EXEC = client

# Default target
all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $(EXEC) $(LIBS)

# Individual source files
client.o: client.cpp
	$(CC) $(CFLAGS) client.cpp -o client.o

# Clean target
clean:
	rm -f $(OBJS) $(EXEC)