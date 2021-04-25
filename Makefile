all:
	g++ -std=c++11 -o a.out col216a5.cpp

.SILENT:
	all
clean:
	rm -f ./a.out
