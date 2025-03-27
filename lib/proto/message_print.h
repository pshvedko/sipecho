/*
 * message_print.h
 *
 *  Created on: Nov 23, 2013
 *      Author: shved
 */

#ifndef MESSAGE_PRINT_H_
#define MESSAGE_PRINT_H_

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <google/protobuf-c/protobuf-c.h>

#define PROTOBUF_C_ASSERT(condition) assert(condition)
#define PROTOBUF_C_ASSERT_NOT_REACHED() assert(0)

#define MESSAGE_PRINT_TAB	"  "

typedef void (*protobuf_c_print)(const ProtobufCFieldDescriptor *, const void *, void *, int, int);

#define protobuf_c_message_member(message, field) 		(void *)(((char *)(message)) + (field)->offset)
#define protobuf_c_message_number(message, field)		(void *)(((char *)(message)) + (field)->quantifier_offset)

size_t protobuf_c_field_size(ProtobufCType);

void message_print(const ProtobufCMessage *, protobuf_c_print print, void *, int);
void protobuf_c_message_print(const ProtobufCMessage *, FILE *);

#endif /* MESSAGE_PRINT_H_ */
