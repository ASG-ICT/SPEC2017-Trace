sim:trace_py.o trace.o
	g++ -g trace_py.o trace.o -o sim
trace_py.o:trace_py.cpp
	g++ -g -c trace_py.cpp
trace.o:trace.cpp
	g++ -g -c trace.cpp
clean:
	rm -f *.o
