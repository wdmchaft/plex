
OBJS = iremoted.o AppleRemote.o

CXX=g++
CC=g++

CFLAGS+=-Wall
LDFLAGS+=-framework IOKit -framework Carbon -framework ForceFeedback

all: $(OBJS)
	$(CXX) -o XBMCHelper $(LDFLAGS) $(OBJS)

.cpp.o:
	$(CXX) -c $(CFLAGS) $(DEFINES) $(INCLUDES) $< -o ${<:.cpp=.o}

.c.o:
	$(CC) -c $(CFLAGS) $(DEFINES) $(INCLUDES) $< -o ${<:.c=.o}

clean:
	$(RM) -rf *.o XBMCHelper
