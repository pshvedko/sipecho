/*
 * message_xml.c
 *
 *  Created on: Nov 23, 2013
 *      Author: shved
 */

#include "lib/proto/message_xml.h"
#include "lib/proto/message_clone.h"
#include "lib/proto/message_print.h"

static const xml_text_t xname = xml_text_INIT("?xml");
static const xml_text_t vname = xml_text_INIT("version");
static const xml_text_t vtext = xml_text_INIT("1.0");
static const xml_text_t ename = xml_text_INIT("encoding");
static const xml_text_t etext = xml_text_INIT("utf-8");

static int normal_field_name(const char *in, char *out) {
    char *begin = out;
    int i = 0;
    do {
        if (in[0] == '_') {
            i++;
            in++;
            continue;
        } else if (i == 1) {
            out[0] = '_';
            out++;
        } else if (i == 2) {
            out[0] = '-';
            out++;
        } else if (i >= 3) {
            out[0] = ':';
            out++;
            i -= 3;
            if (i) {
                in -= i;
                i = 0;
                continue;
            }
        }
        i = 0;
        out[0] = in[0];
        in++;
        out++;
    } while (*in);

    out[0] = in[0];

    return out - begin;
}

static protobuf_c_boolean int32_field_set(int32_t *member, const xml_element_t *node) {
    if (!node)
        return 0;
    if (xml_text_is_null(&node->content))
        return 0;
    member[0] = xml_text_int32(&node->content);
    return 1;
}

static protobuf_c_boolean uint32_field_set(uint32_t *member, const xml_element_t *node) {
    if (!node)
        return 0;
    if (xml_text_is_null(&node->content))
        return 0;
    member[0] = xml_text_uint32(&node->content);
    return 1;
}

static protobuf_c_boolean int64_field_set(int64_t *member, const xml_element_t *node) {
    if (!node)
        return 0;
    if (xml_text_is_null(&node->content))
        return 0;
    member[0] = xml_text_int64(&node->content);
    return 1;
}

static protobuf_c_boolean uint64_field_set(uint64_t *member, const xml_element_t *node) {
    if (!node)
        return 0;
    if (xml_text_is_null(&node->content))
        return 0;
    member[0] = xml_text_uint64(&node->content);
    return 1;
}

static protobuf_c_boolean float_field_set(float *member, const xml_element_t *node) {
    if (!node)
        return 0;
    if (xml_text_is_null(&node->content))
        return 0;
    member[0] = xml_text_float(&node->content);
    return 1;
}

static protobuf_c_boolean double_field_set(double *member, const xml_element_t *node) {
    if (!node)
        return 0;
    if (xml_text_is_null(&node->content))
        return 0;
    member[0] = xml_text_double(&node->content);
    return 1;
}

static protobuf_c_boolean bool_field_set(protobuf_c_boolean *member, const xml_element_t *node) {
    if (!node)
        return 0;
    if (xml_text_is_null(&node->content))
        return 0;
    member[0] = xml_text_bool(&node->content);
    return 1;
}

static void message_xml(ProtobufCMessage *message, const xml_element_t *node, const xml_element_t *parent);

static protobuf_c_boolean message_field_set(ProtobufCMessage **member, const ProtobufCMessageDescriptor *descriptor,
                                    const xml_element_t *node, const xml_element_t *parent) {
    PROTOBUF_C_ASSERT(descriptor->magic == PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC);

    if (!parent)
        return 0;
    ProtobufCMessage *message = protobuf_c_message_alloc(descriptor);
    if (message) {
        protobuf_c_message_setup(descriptor, message);
        message_xml(message, node, parent);
    } else
        return 0;
    member[0] = message;
    return 1;
}

static protobuf_c_boolean enum_field_set(uint32_t *member, const ProtobufCEnumDescriptor *descriptor,
                                 const xml_element_t *node) {
    PROTOBUF_C_ASSERT(descriptor->magic == PROTOBUF_C__ENUM_DESCRIPTOR_MAGIC);

    if (!node)
        return 0;
    unsigned n = descriptor->n_values;
    while (n--) {
        char name[(strlen(descriptor->values[n].name)) + 1];
        normal_field_name(descriptor->values[n].name, name);
        if (!xml_text_cmp3(&node->name, name)) {
            member[0] = descriptor->values[n].value;
            return 1;
        }
    }
    return 0;
}

static protobuf_c_boolean string_field_set(char **member, const xml_element_t *node) {
    if (!node)
        return 0;
    if (xml_text_is_null(&node->content))
        return 0;
    member[0] = xml_text_dup0(node->content.value, node->content.length);
    return 1;
}

static const xml_element_t *optional_field_init(const ProtobufCFieldDescriptor *field, void *member,
                                                protobuf_c_boolean *has, const xml_element_t *node,
                                                const xml_element_t *parent) {
    protobuf_c_boolean set[1] = {0};
    if (has == NULL)
        has = set;

    int text = 0;

    const xml_element_t *next = node;

    if (memcmp(field->name, "__", 2) == 0) {
        text = 2;
        node = parent->attribute;
    }

    char name[(strlen(field->name + text) + 1)];
    normal_field_name(field->name + text, name);
    while (node) {
        if (!xml_text_cmp3(&node->name, name)) {
            if (!text) {
                next = node;
                node = node->children;
            }
            break;
        }
        node = node->next;
        if (!text) {
            next = node;
        }
    }

    switch (field->type) {
        case PROTOBUF_C_TYPE_SFIXED32:
        case PROTOBUF_C_TYPE_SINT32:
        case PROTOBUF_C_TYPE_INT32:
            has[0] = int32_field_set(member, node);
            break;
        case PROTOBUF_C_TYPE_FIXED32:
        case PROTOBUF_C_TYPE_UINT32:
            has[0] = uint32_field_set(member, node);
            break;
        case PROTOBUF_C_TYPE_SFIXED64:
        case PROTOBUF_C_TYPE_SINT64:
        case PROTOBUF_C_TYPE_INT64:
            has[0] = int64_field_set(member, node);
            break;
        case PROTOBUF_C_TYPE_FIXED64:
        case PROTOBUF_C_TYPE_UINT64:
            has[0] = uint64_field_set(member, node);
            break;
        case PROTOBUF_C_TYPE_FLOAT:
            has[0] = float_field_set(member, node);
            break;
        case PROTOBUF_C_TYPE_DOUBLE:
            has[0] = double_field_set(member, node);
            break;
        case PROTOBUF_C_TYPE_BOOL:
            has[0] = bool_field_set(member, node);
            break;
        case PROTOBUF_C_TYPE_STRING:
            has[0] = string_field_set(member, node);
            break;
        case PROTOBUF_C_TYPE_BYTES:
            // FIXME
            break;
        case PROTOBUF_C_TYPE_MESSAGE:
            has[0] = message_field_set(member, field->descriptor, node, next);
            break;
        case PROTOBUF_C_TYPE_ENUM:
            has[0] = enum_field_set(member, field->descriptor, node);
            break;
    }

    return next;
}

static const xml_element_t *required_field_init(const ProtobufCFieldDescriptor *field, void *member,
                                                const xml_element_t *node, const xml_element_t *parent) {
    return optional_field_init(field, member, NULL, node, parent);
}

static const xml_element_t *repeated_field_init(const ProtobufCFieldDescriptor *field, char **array,
                                                size_t *count, const xml_element_t *node, const xml_element_t *parent) {
    while (node) {
        unsigned size = protobuf_c_field_size(field->type);
        char member[size];
        protobuf_c_boolean found = 0;
        node = optional_field_init(field, member, &found, node, parent);
        if (!node)
            break;
        if (found) {
            *count += 1;
            *array = realloc(*array, *count * size);
            memcpy(*array + *count * size - size, member, size);
        }
        node = node->next;
    }
    return node;
}

static void message_xml(ProtobufCMessage *message, const xml_element_t *node, const xml_element_t *parent) {
    if (!message)
        return;

    PROTOBUF_C_ASSERT(message->descriptor->magic == PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC);

    unsigned i = 0;
    for (; i < message->descriptor->n_fields; ++i) {
        const ProtobufCFieldDescriptor *field = message->descriptor->fields + i;
        void *member = protobuf_c_message_member(message, field);
        void *number = protobuf_c_message_number(message, field);

        switch (field->label) {
            case PROTOBUF_C_LABEL_REQUIRED:
                required_field_init(field, member, node, parent);
                break;
            case PROTOBUF_C_LABEL_NONE:
                // FIXME
                break;
            case PROTOBUF_C_LABEL_OPTIONAL:
                if (field->type != PROTOBUF_C_TYPE_MESSAGE && field->type != PROTOBUF_C_TYPE_STRING)
                    optional_field_init(field, member, number, node, parent);
                else
                    optional_field_init(field, member, NULL, node, parent);
                break;
            case PROTOBUF_C_LABEL_REPEATED:
                repeated_field_init(field, member, number, node, parent);
                break;
        }
    }
}

static void print_xml(const ProtobufCFieldDescriptor *field, const void *member, void *out, int tab, int foo) {
    xml_element_t *add = xml_element_new(xml_element_node, xml_shape_open, NULL, NULL);
    if (!add)
        return;

    if (memcmp(field->name, "__", 2) == 0) {
        char name[(strlen(field->name + 2)) + 1];
        add->name = xml_text_dup(name, normal_field_name(field->name + 2, name));
        add->type = xml_attribute_node;
        xml_element_add(out, add);
    } else {
        char name[(strlen(field->name)) + 1];
        add->name = xml_text_dup(name, normal_field_name(field->name, name));
        if (field->type != PROTOBUF_C_TYPE_MESSAGE) {
            xml_element_t *txt = xml_element_new(xml_text_node, xml_shape_single, NULL, NULL);
            if (!txt) {
                xml_element_free(add);
                return;
            }
            if (field->type == PROTOBUF_C_TYPE_ENUM) {
                txt->type = xml_element_node;
            }
            xml_element_add(out, add);
            xml_element_add(add, txt);
        } else
            xml_element_add(out, add);
    }

    xml_text_t text = xml_text_INITIALIZER;
    switch (field->type) {
        case PROTOBUF_C_TYPE_SFIXED32:
        case PROTOBUF_C_TYPE_SINT32:
        case PROTOBUF_C_TYPE_INT32:
            text = xml_text_dup_int32(member);
            break;
        case PROTOBUF_C_TYPE_FIXED32:
        case PROTOBUF_C_TYPE_UINT32:
            text = xml_text_dup_uint32(member);
            break;
        case PROTOBUF_C_TYPE_SFIXED64:
        case PROTOBUF_C_TYPE_SINT64:
        case PROTOBUF_C_TYPE_INT64:
            text = xml_text_dup_int64(member);
            break;
        case PROTOBUF_C_TYPE_FIXED64:
        case PROTOBUF_C_TYPE_UINT64:
            text = xml_text_dup_uint64(member);
            break;
        case PROTOBUF_C_TYPE_FLOAT:
            text = xml_text_dup_float(member);
            break;
        case PROTOBUF_C_TYPE_DOUBLE:
            text = xml_text_dup_double(member);
            break;
        case PROTOBUF_C_TYPE_BOOL:
            text = xml_text_dup_bool(member);
            break;
        case PROTOBUF_C_TYPE_STRING:
            text = xml_text_dup_string(member);
            break;
        case PROTOBUF_C_TYPE_BYTES:
            // FIXME
            break;
        case PROTOBUF_C_TYPE_ENUM: {
            const ProtobufCEnumValue *enumvalue = protobuf_c_enum_descriptor_get_value(field->descriptor,
                *(int *) member);
            text = xml_text_set(enumvalue->name, strlen(enumvalue->name));
            break;
        }
        case PROTOBUF_C_TYPE_MESSAGE: {
            const ProtobufCMessage *message = *(ProtobufCMessage * const *) member;
            message_print(message, print_xml, add, 1 + tab);
            if (!message) {
                add->shape = xml_shape_single;
            }
            return;
        }
    }

    if (add->children)
        if (add->children->type == xml_element_node)
            add->children->name = text;
        else
            add->children->content = text;
    else
        add->content = text;
}

void protobuf_c_message_from_xml(ProtobufCMessage *message, const xml_element_t *root) {
    message_xml(message, root->children, root);
}

void protobuf_c_message_to_xml(const ProtobufCMessage *message, xml_element_t *root) {
    xml_element_t *elm1 = xml_element_new(xml_element_node, xml_shape_single, &xname, NULL);
    xml_element_t *elm2 = xml_element_new(xml_attribute_node, xml_shape_open, &vname, &vtext);
    xml_element_t *elm3 = xml_element_new(xml_attribute_node, xml_shape_open, &ename, &etext);

    xml_element_add(elm1, elm2);
    xml_element_add(elm1, elm3);
    xml_element_add(root, elm1);

    message_print(message, print_xml, root, 0);
}
