FLAGS=-m32 -g2 -fstack-protector-all

all: byterun.o main.o
	$(CXX) $(FLAGS) -o frequency_analyzer byterun.o main.o

main.o: main.cpp byterun.h
	$(CXX) $(FLAGS) -std=c++17 -c main.cpp

byterun.o: byterun.c byterun.h
	$(CC) $(FLAGS) -c byterun.c

clean:
	$(RM) *.a *.o *.bc *~ frequency_analyzer