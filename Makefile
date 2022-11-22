all: uaf

uaf: uaf.cpp
	g++ -g -O0 -no-pie -o uaf uaf.cpp

clean:
	rm -rf uaf