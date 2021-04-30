all:
	g++ -std=c++11 	 -MMD -MP -c DRAM.cpp -o a2.o
	g++ -std=c++11   -MMD -MP -c Memory_request_manager.cpp -o a1.o
	g++ -std=c++11   -MMD -MP -c COL216_A5.cpp  -o a.o
	g++  ./a2.o ./a1.o ./a.o -o final

.SILENT:
	all
clean:
	rm -f *.o
	rm -f *.d
	rm -f final
	rm -f final.exe

