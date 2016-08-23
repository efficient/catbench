#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
	if(argc < 2) {
		printf("USAGE: %s <command> [argument]...\n", argv[0]);
		return 1;
	}

	setpgid(0, 0);
	execvp(argv[1], argv + 1);
	return 1;
}
