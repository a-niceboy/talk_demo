OBJ = user.o main.o

all : $(OBJ)
	g++ -std=c++11 -o demo $(OBJ) -pthread

user.o : user.h user.cpp
	g++ -std=c++11 -c -o user.o user.cpp

main.o : main.cpp
	g++ -std=c++11 -c -o main.o main.cpp

clean :
	rm -f demo *.o
