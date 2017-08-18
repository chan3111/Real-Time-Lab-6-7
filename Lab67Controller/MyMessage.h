/*
 * MyMessage.h
 *
 *  Created on: Oct 31, 2016
 *      Author: king
 */

#ifndef MYMESSAGE_H_
#define MYMESSAGE_H_

struct message {
	char text[128];
};
typedef union {
	struct message value;
	char data[sizeof(struct message)];
} MyMessage;

typedef union {
        struct _pulse   pulse;
} my_message_t;

#endif /* MYMESSAGE_H_ */
