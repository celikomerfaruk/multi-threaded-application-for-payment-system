CC=g++

simulation:	main.cpp
		$(CC) main.cpp -pthread -std=c++11 -o simulation