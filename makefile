CC = g++
CFLAGS = -Wall -Wexta -std=c++11
LDFLAGS = -lpthread

SRCS = server.cpp client.cpp
TARGETS = server client
OBJS = $(SRC:.cpp=.o)

all: $(TARGETS)

$(TARETS): %: %.0
	$(CC) %(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGETS)
