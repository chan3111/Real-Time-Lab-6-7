#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <process.h>
#include <sys/neutrino.h>
#include <sys/dispatch.h>
#include "MyMessage.h"

int main(int argc, char *argv[]) {
	int receiveId;
	MyMessage msg;
	name_attach_t *attach;

	// setup for line buffering with buffer size 0
	// On output - data is written right away - no buffering
	setvbuf (stdout, NULL, _IOLBF, 0);

	if ((attach = name_attach(NULL, "dev/display_namespace", 0)) == NULL) {
		printf("Failed to create namespace for controller connection");
		return EXIT_FAILURE;
	}

	while(1) {
		receiveId = MsgReceive(attach->chid, msg.data, sizeof(struct message), NULL);
		MsgReply(receiveId, EOK, msg.data, sizeof(struct message));

		printf("Display Received: [%s]\n", msg.value.text);
	}
	name_detach(attach, 0);
	return EXIT_SUCCESS;
}
