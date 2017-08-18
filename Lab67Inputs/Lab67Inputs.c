#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <process.h>
#include <sys/neutrino.h>
#include <sys/netmgr.h>
#include <sys/dispatch.h>
#include "MyMessage.h"

int main(int argc, char *argv[]) {
	char	inputString[256];
	int		server_coid;
	int 	done = 0;
	MyMessage	msg;

	// setup for line buffering with buffer size 0
	// On output - data is written right away - no buffering
	setvbuf (stdout, NULL, _IOLBF, 0);

	// prepare connection to display
	if ((server_coid = name_open("dev/controller_namespace", 0)) == -1) {
			perror("Could not connect to Controller.");
			exit(EXIT_FAILURE);
	}

	while (!done) {
		printf("Enter the input: LD, RD, LU, RU, S, or Q(to quit):");
		scanf("%s", inputString);

		if (strcmp(inputString, "Q") == 0) {
			done = 1;
		} else {
			strcpy(msg.value.text,inputString);
			if (MsgSend(server_coid, msg.data, sizeof(msg.data), msg.data, sizeof(msg.data)) == -1) {
				perror("MsgSend error sending to controller\n");
				exit(1);
			}
		}
	}
	name_close(server_coid);
	exit (EXIT_SUCCESS);
}
