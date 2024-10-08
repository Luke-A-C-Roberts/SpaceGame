CC := clang
OPTIONS := -O3 -g -Wall -Wextra -Wpedantic

default:
	$(CC) $(OPTIONS) -o game src/main.cpp

clean:
	rm build/
