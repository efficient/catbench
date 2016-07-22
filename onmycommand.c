#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void ignore(int sig) {(void) sig;}

int main(int argc, char **argv) {
	if(argc < 2) {
		printf("USAGE: %s <command> [<argument>...]\n\n"
				"Awaits SIGUSR1, then runs the specified command.\n",
				argv[0]);
		return 1;
	}

	for(int arg = 0; arg < argc -1; ++arg)
		argv[arg] = argv[arg + 1];
	argv[argc - 1] = NULL;

	struct sigaction setup = {.sa_handler = ignore};
	if(sigaction(SIGUSR1, &setup, NULL)) {
		perror("Installing signal handler");
		return 2;
	}

	while(true) {
		pause();

		if(!fork()) {
			execvp(argv[0], argv);
			perror("Spawning process");
		}
	}
}
