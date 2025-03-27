/*
 * message_xml.h
 *
 *  Created on: Nov 23, 2013
 *      Author: shved
 */

#ifndef MESSAGE_INIT_XML_H_
#define MESSAGE_INIT_XML_H_

#include <google/protobuf-c/protobuf-c.h>

#include "lib/common/xml.h"

void protobuf_c_message_from_xml(ProtobufCMessage *, const xml_element_t *);
void protobuf_c_message_to_xml(const ProtobufCMessage *, xml_element_t *);

#endif /* MESSAGE_INIT_XML_H_ */
