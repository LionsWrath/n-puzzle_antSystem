CC=gcc

default: 
#	$(CC) -gstabs 15puzzle.c -lm -o 15puzzle.out
	$(CC) -gstabs 15puzzle_recursive.c -lm -o 15puzzle_recursive.out
#	$(CC) -gstabs 15puzzle_bfs.c -lm -o 15puzzle_bfs.out
	$(CC) -gstabs 15puzzle_recursive_parallel.c -lm -o 15puzzle_recursive_parallel.out -lpthread
