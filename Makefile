all:
	g++ -std=c++11 -o a1.out -c Memory_request_manager.cpp
	g++ -std=c++11 -o a.out -c COL216_A5.cpp  

.SILENT:
	all
clean:
	rm -f ./a.out
	rm -f ./a1.out
