OBJ = common.o main.o

all : $(OBJ)
	g++ -std=c++11 -o demo $(OBJ) -pthread

common.o : common.h common.cpp
	g++ -std=c++11 -c -o common.o common.cpp

main.o : main.cpp
	g++ -std=c++11 -c -o main.o main.cpp

clean :
	rm -f demo *.o
