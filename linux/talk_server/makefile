OBJ = user.o wrap.o talk_server.o main.o

all : $(OBJ)
	g++ -std=c++11 -o demo $(OBJ) -pthread

user.o : user.h user.cpp
	g++ -std=c++11 -c -o user.o user.cpp

wrap.o : wrap.h wrap.cpp
	g++ -std=c++11 -c -o wrap.o wrap.cpp

talk_server.o : talk_server.h talk_server.cpp
	g++ -std=c++11 -c -o talk_server.o talk_server.cpp -pthread

main.o : main.cpp
	g++ -std=c++11 -c -o main.o main.cpp

clean :
	rm -f demo *.o
