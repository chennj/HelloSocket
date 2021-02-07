MAKEFILE= Makefile
SOURCES += server.cpp crc_allocator.cpp

INCLUDE = -I./ -I../common/include/
CXX		= g++
LIBS	= -lpthread
FLAGS	= -Wall

TARGET	= server
all: $(TARGET)
server:
	$(CXX) -o server -std=c++11 $(SOURCES) $(INCLUDE) $(LIBS) $(FLAGS) 

.PHONY: clean
clean:
	rm -rf server 