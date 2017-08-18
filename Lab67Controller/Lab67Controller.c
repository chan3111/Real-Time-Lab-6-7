#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <process.h>
#include <sys/neutrino.h>
#include <sys/netmgr.h>
#include <sys/dispatch.h>
#include <process.h>
#include <errno.h>
#include "proj.h"
#include "MyMessage.h"
#include <time.h>
#include <unistd.h>

#define MY_PULSE_CODE   _PULSE_CODE_MINAVAIL

//#define STANDALONE
#ifndef STANDALONE
//Input channel
name_attach_t *attach;
// Output connection
int server_coid;

int buttonUp = 0;

Input getInput() {

	char inputString[256];
	Input i;
	MyMessage	msg;
	int	receiveId;

	receiveId = MsgReceive(attach->chid, msg.data, sizeof(struct message), NULL);
	MsgReply(receiveId, EOK, msg.data, sizeof(struct message));

	strcpy(inputString, msg.value.text);

	//printf("Controller Received (LD,RD,LU,RU, or S): [%s]\n", msg.value.text);
	fflush(stdout);

	for ( i = 0 ; (i < sizeof(inMessage)) && (strcmp(inMessage[i],inputString) != 0) ; ++i ) {
		// do nothing
	}
	//printf("Matched! (LD,RD,LU,RU, or S): [%d]\n", i);
	fflush(stdout);
	return (i);
}


int	doOutput(Output out) {
	MyMessage msg;

	strcpy(msg.value.text, outMessage[out]);

	//printf("Sending! (LD,RD,LU,RU, or S): [%d]\n", out);
	fflush(stdout);
	if (MsgSend(server_coid, msg.data, sizeof(struct message), msg.data, sizeof(struct message)) == -1) {
		perror("Error sending message to display process\n");
		exit(1);
	}
	return EXIT_SUCCESS;
}
#else
Input getInput() {

	char inputString[256];
	Input i;

	//printf("Enter the input (LD,RD,LU,RU, or S):");
	fflush(stdout);
	scanf("%s", inputString);

	for ( i = 0 ; (i < sizeof(inMessage)) && (strcmp(inMessage[i],inputString) != 0) ; ++i ) {
		// do nothing
	}
	return (i);
}

int	doOutput(Output out) {
	printf("Output:[%s]\n",outMessage[out]);
	fflush(stdout);
	return EXIT_SUCCESS;
}
#endif

void* checkInput(){
	Input input;

	doOutput(CANCEL_ARM_MSG);
	input = getInput();
	if(input == RIGHT_BUTTON_UP || input == LEFT_BUTTON_UP){
		buttonUp = 1;
	}
}

int main(int argc, char *argv[]) {

	State	state = START_STATE;
	Input	input;
	pthread_attr_t attr;
	//Output	output;
	struct sigevent         event;
	struct itimerspec       itime;
	timer_t                 timer_id;
	int                     chid;
	int                     rcvid;
	my_message_t			tmsg;
	int						threadid;

	chid = ChannelCreate(0);

	event.sigev_notify = SIGEV_PULSE;
	event.sigev_coid = ConnectAttach(ND_LOCAL_NODE, 0, chid, _NTO_SIDE_CHANNEL, 0);
	event.sigev_priority = getprio(0);
	event.sigev_code = MY_PULSE_CODE;

	itime.it_value.tv_sec = 2;
	itime.it_value.tv_nsec = 0;
	itime.it_interval.tv_sec = 2;
	/* 500 million nsecs = .5 secs */
	itime.it_interval.tv_nsec = 0;

// setup for line buffering with buffer size 0
// On output - data is written right away - no buffering
setvbuf (stdout, NULL, _IOLBF, 0);

#ifndef STANDALONE
	// prepare the channel to receive inputs
	if ((attach = name_attach(NULL, "dev/controller_namespace", 0)) == NULL) {
		printf("Failed creating namespace for inputs connection");
		return EXIT_FAILURE;
	}

	// prepare connection to display
	if ((server_coid = name_open("dev/display_namespace", 0)) == -1) {
			perror("Could not connect to Display.");
			exit(EXIT_FAILURE);
	}
#endif

	while (state != STOP_STATE) {
		switch (state) {
		case READY_STATE:

			doOutput(READY_MSG);

			switch(input = getInput()) {
			case LEFT_BUTTON_DOWN:
				state = LEFT_DOWN_STATE;
				break;
			case RIGHT_BUTTON_DOWN:
				state = RIGHT_DOWN_STATE;
				break;
			case STOP_BUTTON:
				state = EXIT_STATE;
				break;
			default:
				//ignore
				break;
			}
			break;
		case LEFT_DOWN_STATE:
			doOutput(LEFT_DOWN_MSG);
			switch(input = getInput()) {
			case LEFT_BUTTON_UP:
				state = READY_STATE;
				break;
			case RIGHT_BUTTON_DOWN:
				state = ARMED_STATE;
				break;
			default:
				//ignore
				break;
			}
			break;
		case RIGHT_DOWN_STATE:
			doOutput(RIGHT_DOWN_MSG);
			switch(input = getInput()) {
			case RIGHT_BUTTON_UP:
				state = READY_STATE;
				break;
			case LEFT_BUTTON_DOWN:
				state = ARMED_STATE;
				break;
			default:
				//ignore
				break;
			}
			break;
		case ARMED_STATE:
			doOutput(ARMED_MSG);
			sleep(2);
			timer_create(CLOCK_REALTIME, &event, &timer_id);
			timer_settime(timer_id, 0, &itime, NULL);
			pthread_attr_init(&attr);
			pthread_create(&threadid, &attr, &checkInput, NULL);
			pthread_attr_destroy(&attr);
			rcvid = MsgReceive(chid, &tmsg, sizeof(tmsg), NULL);
			if(buttonUp == 0)
				state = PUNCH_STATE;
			else
				state = READY_STATE;
			buttonUp = 0;
			pthread_cancel(threadid);
			timer_delete(timer_id);
			break;
		case PUNCH_STATE:
			doOutput(PUNCH_MSG);
			sleep(1);
			state = READY_STATE;
			break;
		case EXIT_STATE:
			doOutput(EXIT_MSG);
			state = STOP_STATE;
			break;
		case START_STATE:
			state = READY_STATE;
			break;
		default:
			break;
		}
	}

	printf("Exiting now.\n");

#ifndef STANDALONE
	// cleanup the display message channel
	name_close(server_coid);
	// cleanup the input message channel
	name_detach(attach, 0);
#endif

	return EXIT_SUCCESS;
}
