/*
 * message_print.c
 *
 *  Created on: Nov 23, 2013
 *      Author: shved
 * Stolen from: https://github.com/michaelstorm/Tangent/blob/master/src/message_print.c
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "lib/proto/message_print.h"

size_t protobuf_c_field_size(ProtobufCType type) {

	switch (type) {
		case PROTOBUF_C_TYPE_SINT32:
		case PROTOBUF_C_TYPE_INT32:
		case PROTOBUF_C_TYPE_UINT32:
		case PROTOBUF_C_TYPE_SFIXED32:
		case PROTOBUF_C_TYPE_FIXED32:
		case PROTOBUF_C_TYPE_FLOAT:
		case PROTOBUF_C_TYPE_ENUM:
			return 4;
		case PROTOBUF_C_TYPE_SINT64:
		case PROTOBUF_C_TYPE_INT64:
		case PROTOBUF_C_TYPE_UINT64:
		case PROTOBUF_C_TYPE_SFIXED64:
		case PROTOBUF_C_TYPE_FIXED64:
		case PROTOBUF_C_TYPE_DOUBLE:
			return 8;
		case PROTOBUF_C_TYPE_BOOL:
			return sizeof(protobuf_c_boolean);
		case PROTOBUF_C_TYPE_STRING:
		case PROTOBUF_C_TYPE_MESSAGE:
			return sizeof(void *);
		case PROTOBUF_C_TYPE_BYTES:
			return sizeof(ProtobufCBinaryData);
	}
	PROTOBUF_C_ASSERT_NOT_REACHED();
	return 0;
}

static void print_tabs(FILE *out, int tabs) {
	for (int i = 0; i < tabs; i++)
		fprintf(out, MESSAGE_PRINT_TAB);
}

static void print_nl(FILE *out, const int tabs) {

	fprintf(out, "\n");
	print_tabs(out, tabs);
}

static void label_name_print(const ProtobufCFieldDescriptor *field, FILE *out) {

	switch (field->label) {
		case PROTOBUF_C_LABEL_REQUIRED:
			fprintf(out, "required ");
			break;
		case PROTOBUF_C_LABEL_NONE:
		case PROTOBUF_C_LABEL_OPTIONAL:
			fprintf(out, "optional ");
			break;
		case PROTOBUF_C_LABEL_REPEATED:
			fprintf(out, "repeated ");
			break;
	}
}

static void type_name_print(const ProtobufCFieldDescriptor *field, FILE *out) {

	label_name_print(field, out);

	switch (field->type) {
		case PROTOBUF_C_TYPE_INT32:
			fprintf(out, "int32 %s", field->name);
			break;
		case PROTOBUF_C_TYPE_SINT32:
			fprintf(out, "sint32 %s", field->name);
			break;
		case PROTOBUF_C_TYPE_SFIXED32:
			fprintf(out, "sfixed32 %s", field->name);
			break;
		case PROTOBUF_C_TYPE_INT64:
			fprintf(out, "int64 %s", field->name);
			break;
		case PROTOBUF_C_TYPE_SINT64:
			fprintf(out, "sint64 %s", field->name);
			break;
		case PROTOBUF_C_TYPE_SFIXED64:
			fprintf(out, "sfixed64 %s", field->name);
			break;
		case PROTOBUF_C_TYPE_UINT32:
			fprintf(out, "uint32 %s", field->name);
			break;
		case PROTOBUF_C_TYPE_FIXED32:
			fprintf(out, "fixed32 %s", field->name);
			break;
		case PROTOBUF_C_TYPE_UINT64:
			fprintf(out, "uint64 %s", field->name);
			break;
		case PROTOBUF_C_TYPE_FIXED64:
			fprintf(out, "fixed64 %s", field->name);
			break;
		case PROTOBUF_C_TYPE_FLOAT:
			fprintf(out, "float %s", field->name);
			break;
		case PROTOBUF_C_TYPE_DOUBLE:
			fprintf(out, "double %s", field->name);
			break;
		case PROTOBUF_C_TYPE_BOOL:
			fprintf(out, "bool %s", field->name);
			break;
		case PROTOBUF_C_TYPE_ENUM: {
			const ProtobufCEnumDescriptor *desc = field->descriptor;
			fprintf(out, "%s %s", desc->name, field->name);
			break;
		}
		case PROTOBUF_C_TYPE_STRING:
			fprintf(out, "string %s", field->name);
			break;
		case PROTOBUF_C_TYPE_BYTES:
			fprintf(out, "bytes %s", field->name);
			break;
		case PROTOBUF_C_TYPE_MESSAGE: {
			const ProtobufCMessageDescriptor *desc = field->descriptor;
			fprintf(out, "%s %s", desc->name, field->name);
			break;
		}
	}
	fprintf(out, " = ");
}

static void print_out(const ProtobufCFieldDescriptor *field, const void *member, void *out, const int tabs, const int pos) {

	if (field->label == PROTOBUF_C_LABEL_REPEATED && pos) {
		fprintf(out, ", ");
	} else {
		print_nl(out, tabs);
		type_name_print(field, out);
	}
	switch (field->type) {
//		case PROTOBUF_C_TYPE_GROUP: // NOT SUPPORTED
		case PROTOBUF_C_TYPE_SFIXED32:
		case PROTOBUF_C_TYPE_SINT32:
		case PROTOBUF_C_TYPE_INT32:
			fprintf(out, "%d", *(const int32_t *) member);
			break;
		case PROTOBUF_C_TYPE_FIXED32:
		case PROTOBUF_C_TYPE_UINT32:
			fprintf(out, "%u", *(const uint32_t *) member);
			break;
		case PROTOBUF_C_TYPE_SFIXED64:
		case PROTOBUF_C_TYPE_SINT64:
		case PROTOBUF_C_TYPE_INT64:
			fprintf(out, "%"PRId64, *(const int64_t *) member);
			break;
		case PROTOBUF_C_TYPE_FIXED64:
		case PROTOBUF_C_TYPE_UINT64:
			fprintf(out, "%"PRIu64, *(const uint64_t *) member);
			break;
		case PROTOBUF_C_TYPE_FLOAT:
			fprintf(out, "%f", *(const float *) member);
			break;
		case PROTOBUF_C_TYPE_DOUBLE:
			fprintf(out, "%lf", *(const double *) member);
			break;
		case PROTOBUF_C_TYPE_BOOL:
			fprintf(out, "%s", *(const protobuf_c_boolean *) member ? "true" : "false");
			break;
		case PROTOBUF_C_TYPE_STRING:
			fprintf(out, "\"%s\"", *(char * const *) member);
			break;
		case PROTOBUF_C_TYPE_BYTES: {
			const ProtobufCBinaryData * bd = ((const ProtobufCBinaryData*) member);
			fprintf(out, "[%zu] ", bd->len);
			int i;
			int printable = 1;
			for (i = 0; i < bd->len; i++) {
				fprintf(out, "%02x ", bd->data[i]);
				if (printable && !isprint(bd->data[i]))
					printable = 0;
			}
			if (printable && bd->len > 0) {
				fprintf(out, "(\"");
				for (i = 0; i < bd->len; i++)
					fprintf(out, "%c", bd->data[i]);
				fprintf(out, "\")");
			}
			break;
		}
		case PROTOBUF_C_TYPE_MESSAGE:
			fprintf(out, "{");
			message_print(*(ProtobufCMessage * const *) member, print_out, out, tabs + 1);
			print_nl(out, tabs);
			fprintf(out, "}");
			break;
		case PROTOBUF_C_TYPE_ENUM: {
			const int value = *(const int *) member;
			const ProtobufCEnumValue * evalue = protobuf_c_enum_descriptor_get_value(field->descriptor,
					value);
			fprintf(out, "%d (%s)", evalue->value, evalue->name);
			break;
		}
	}
}

static void required_field_out(const ProtobufCFieldDescriptor *field, const void *member,
		const protobuf_c_print print, void *out, const int tab, const int pos) {

	return print(field, member, out, tab, pos);
}

static void optional_field_out(const ProtobufCFieldDescriptor *field, const void *member,
		const protobuf_c_boolean *has, const protobuf_c_print print, void *out, const int tab) {

	if (field->type == PROTOBUF_C_TYPE_MESSAGE || field->type == PROTOBUF_C_TYPE_STRING)
		if (*(const void * const *) member)
			return print(field, member, out, tab, 0);
		else
			return;
	else if (*has)
		return print(field, member, out, tab, 0);
}

static void repeated_field_out(const ProtobufCFieldDescriptor *field, const void *member, const size_t *count,
		const protobuf_c_print print, void *out, const int tab) {

	const size_t size = protobuf_c_field_size(field->type);
	const char *array = *(char * const *) member;
	for (size_t i = 0; i < *count; ++i) {
		required_field_out(field, array, print, out, tab, i);
		array += size;
	}
}

void message_print(const ProtobufCMessage *message, const protobuf_c_print print, void *out, const int tab) {

	if (!message)
		return;

	PROTOBUF_C_ASSERT(message->descriptor->magic == PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC);

	unsigned i = 0;
	for (; i < message->descriptor->n_fields; ++i) {
		const ProtobufCFieldDescriptor *field = message->descriptor->fields + i;
		const void *member = protobuf_c_message_member(message, field);
		const void *number = protobuf_c_message_number(message, field);

		switch (field->label) {
			case PROTOBUF_C_LABEL_REQUIRED:
				required_field_out(field, member, print, out, tab, 0);
				break;
			case PROTOBUF_C_LABEL_NONE:
			case PROTOBUF_C_LABEL_OPTIONAL:
				optional_field_out(field, member, number, print, out, tab);
				break;
			case PROTOBUF_C_LABEL_REPEATED:
				repeated_field_out(field, member, number, print, out, tab);
				break;
		}
	}
}

void protobuf_c_message_print(const ProtobufCMessage *message, FILE *out) {

	fprintf(out, "message %s %s = {", message->descriptor->name, message->descriptor->short_name);

	message_print(message, print_out, out, 1);

	fprintf(out, "\n}\n");
}
