/*
 * message_clone.h
 *
 *  Created on: Nov 30, 2013
 *      Author: shved
 */

#ifndef MESSAGE_CLONE_H_
#define MESSAGE_CLONE_H_

#include <google/protobuf-c/protobuf-c.h>

#define protobuf_c_message_alloc(descriptor)			malloc(((const ProtobufCMessageDescriptor *)(descriptor))->sizeof_message)
#define protobuf_c_message_setup(descriptor, message)	protobuf_c_message_init((const ProtobufCMessageDescriptor *)(descriptor), (message))

void protobuf_c_message_clone(ProtobufCMessage *, const ProtobufCMessage *);

#endif /* MESSAGE_CLONE_H_ */
