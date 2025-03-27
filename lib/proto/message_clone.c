/*
 * message_copy.c
 *
 *  Created on: Nov 30, 2013
 *      Author: shved
 */

#include <string.h>
#include <math.h>

#include "message_print.h"
#include "message_clone.h"

static void message_clone(ProtobufCMessage *message, const ProtobufCMessage *original);

static inline void int32_field_clone(int *member, const int *original) {

	member[0] = original[0];
}

static inline void int64_field_clone(long long int *member, const long long int *original) {

	member[0] = original[0];
}

static inline void uint32_field_clone(unsigned int *member, const unsigned int *original) {

	member[0] = original[0];
}

static inline void uint64_field_clone(unsigned long long int *member, const unsigned long long int *original) {

	member[0] = original[0];
}

static inline void float_field_clone(float *member, const float *original) {

	member[0] = original[0];
}

static inline void double_field_clone(double *member, const double *original) {

	member[0] = original[0];
}

static inline void bool_field_clone(int *member, const int *original) {

	member[0] = original[0];
}

static inline void string_field_clone(char **member, const char * const *original) {

	if (!original[0])
		return;

	member[0] = strdup(original[0]);
}

static inline void message_field_clone(ProtobufCMessage **member,
		const ProtobufCMessageDescriptor *descriptor, const ProtobufCMessage * const *original) {

	if (!original[0])
		return;

	PROTOBUF_C_ASSERT(original[0]->descriptor->magic == PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC);
	PROTOBUF_C_ASSERT(original[0]->descriptor == descriptor);

	member[0] = protobuf_c_message_alloc(descriptor);
	if (member[0]) {
		protobuf_c_message_setup(descriptor, member[0]);
		message_clone(member[0], original[0]);
	}
}

static void field_clone(const ProtobufCFieldDescriptor *field, void *member, const void *original) {

	switch (field->type) {
		case PROTOBUF_C_TYPE_SFIXED32:
		case PROTOBUF_C_TYPE_SINT32:
		case PROTOBUF_C_TYPE_INT32:
			int32_field_clone(member, original);
			break;
		case PROTOBUF_C_TYPE_FIXED32:
		case PROTOBUF_C_TYPE_UINT32:
			uint32_field_clone(member, original);
			break;
		case PROTOBUF_C_TYPE_SFIXED64:
		case PROTOBUF_C_TYPE_SINT64:
		case PROTOBUF_C_TYPE_INT64:
			int64_field_clone(member, original);
			break;
		case PROTOBUF_C_TYPE_FIXED64:
		case PROTOBUF_C_TYPE_UINT64:
			uint64_field_clone(member, original);
			break;
		case PROTOBUF_C_TYPE_FLOAT:
			float_field_clone(member, original);
			break;
		case PROTOBUF_C_TYPE_DOUBLE:
			double_field_clone(member, original);
			break;
		case PROTOBUF_C_TYPE_BOOL:
			bool_field_clone(member, original);
			break;
		case PROTOBUF_C_TYPE_STRING:
			string_field_clone(member, original);
			break;
		case PROTOBUF_C_TYPE_BYTES:
			// TODO: bytes_field_clone(member, origin);
			break;
		case PROTOBUF_C_TYPE_MESSAGE:
			message_field_clone(member, field->descriptor, original);
			break;
		case PROTOBUF_C_TYPE_ENUM:
			// TODO: enum_field_clone(member, field->descriptor, origin);
			break;
	}
}

static void required_field_clone(const ProtobufCFieldDescriptor *field, void *member,
		const ProtobufCMessage *source) {

	field_clone(field, member, protobuf_c_message_member(source, field));
}

static void optional_field_clone(const ProtobufCFieldDescriptor *field, void *member, protobuf_c_boolean *has,
		const ProtobufCMessage *source) {

	const protobuf_c_boolean *one = protobuf_c_message_number(source, field);
	if (one[0]) {
		required_field_clone(field, member, source);
		has[0] = one[0];
	}
}

static void repeated_field_clone(const ProtobufCFieldDescriptor *field, char **array, size_t *count,
		const ProtobufCMessage *source) {

	const size_t *number = protobuf_c_message_number(source, field);
	if (!number[0])
		return;

	const char * const *original = protobuf_c_message_member(source, field);
	if (!original[0])
		return;

	unsigned size = protobuf_c_field_size(field->type);
	array[0] = malloc(number[0] * size);
	if (!array[0])
		return;

	while (count[0] < number[0]) {
		field_clone(field, array[0] + count[0] * size, original[0] + count[0] * size);
		count[0] += 1;
	}
}

static void message_clone(ProtobufCMessage *message, const ProtobufCMessage *original) {

	if (!message || !original)
		return;

	PROTOBUF_C_ASSERT(message->descriptor->magic == PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC);

	unsigned i = 0;
	for (; i < message->descriptor->n_fields; ++i) {
		const ProtobufCFieldDescriptor *field = message->descriptor->fields + i;
		void *member = protobuf_c_message_member(message, field);
		void *number = protobuf_c_message_number(message, field);

		switch (field->label) {
			case PROTOBUF_C_LABEL_REQUIRED:
				required_field_clone(field, member, original);
				break;
			case PROTOBUF_C_LABEL_NONE:
			case PROTOBUF_C_LABEL_OPTIONAL:
				optional_field_clone(field, member, number, original);
				break;
			case PROTOBUF_C_LABEL_REPEATED:
				repeated_field_clone(field, member, number, original);
				break;
		}
	}
}

void protobuf_c_message_clone(ProtobufCMessage *to, const ProtobufCMessage *from) {

	protobuf_c_message_init(from->descriptor, to);

	message_clone(to, from);
}
