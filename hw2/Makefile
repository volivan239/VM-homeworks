all: build/main.o build/gc_runtime.o build/bytefile.o build/runtime.o build/callstack.o build/interpreter.o
	$(CXX) -g -m32 build/gc_runtime.o build/runtime.o build/bytefile.o build/callstack.o build/interpreter.o build/main.o -o build/interpreter

build/main.o: build src/main.cpp src/include/interpreter.h src/include/runtime.h src/include/bytefile.h src/include/callstack.h
	$(CXX) -O2 -I src/include -g -fstack-protector-all -m32 -c src/main.cpp -o build/main.o

build/interpreter.o: build src/interpreter.cpp src/include/interpreter.h src/include/callstack.h src/include/bytefile.h
	$(CXX) -O2 -I src/include -g -fstack-protector-all -m32 -c src/interpreter.cpp -o build/interpreter.o

build/callstack.o: build src/callstack.cpp src/include/callstack.h
	$(CXX) -O2 -I src/include -g -fstack-protector-all -m32 -c src/callstack.cpp -o build/callstack.o

build/bytefile.o: build src/bytefile.cpp src/include/bytefile.h
	$(CXX) -O2 -I src/include -g -fstack-protector-all -m32 -c src/bytefile.cpp -o build/bytefile.o

build/gc_runtime.o: build src/gc_runtime.s
	$(CC) -O2 -I src/include -g -fstack-protector-all -m32 -c src/gc_runtime.s -o build/gc_runtime.o

build/runtime.o: build src/runtime.c src/include/runtime.h
	$(CC) -O2 -I src/include -g -fstack-protector-all -m32 -c src/runtime.c -o build/runtime.o

build:
	mkdir build

clean:
	$(RM) -r *.a *.o *~ build *.bc logs
