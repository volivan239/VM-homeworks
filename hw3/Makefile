FLAGS=-m32 -g2 -fstack-protector-all

all: byterun.o runtime.o gc_runtime.o main.o
	$(CXX) $(FLAGS) -o frequency_analyzer byterun.o runtime.o gc_runtime.o main.o

main.o: main.cpp byterun.h
	$(CXX) $(FLAGS) -std=c++17 -c main.cpp

byterun.o: byterun.c runtime.h byterun.h
	$(CC) $(FLAGS) -c byterun.c

gc_runtime.o: gc_runtime.s
	$(CC) $(FLAGS) -c gc_runtime.s

runtime.o: runtime.c runtime.h
	$(CC) $(FLAGS) -c runtime.c

clean:
	$(RM) *.a *.o *.bc *~ frequency_analyzer