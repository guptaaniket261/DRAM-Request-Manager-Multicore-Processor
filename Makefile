all:
	g++ -std=c++11 -o a.out COL216_A5.cpp  Memory_request_manager.cpp

.SILENT:
	all
clean:
	rm -f ./a.out
