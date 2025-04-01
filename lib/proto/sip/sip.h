/*
 * sip.h
 *
 *  Created on: Oct 15, 2013
 *      Author: shved
 */

#ifndef SIP_H_
#define SIP_H_

#include <sys/time.h>

#include <osipparser2/osip_message.h>
#include <osipparser2/sdp_message.h>

#include "lib/proto/sip/service.pb-c.h"

#define CRLF "\r\n"

#define SIP__TYPE__CONTENT_TYPE__SDP				"application/sdp"
#define SIP__TYPE__CONTENT_TYPE__MEDIA_CONTROL		"application/media_control+xml"
#define SIP__TYPE__CONTENT_TYPE__DTMF_RELAY			"application/dtmf-relay"
#define SIP__TYPE__CONTENT_TYPE__PIDF				"application/pidf+xml"
#define SIP__TYPE__CONTENT_TYPE__RESOURCE_LISTS		"application/resource-lists+xml"

#define sip__type__accept__proto 					sip__type__content_type__proto
#define sip__type__accept_encoding__proto 			sip__type__encode__proto
#define sip__type__accept_language__proto 			sip__type__encode__proto
#define sip__type__alert_info__proto 				sip__type__encode__proto
#define sip__type__call_info__proto 				sip__type__encode__proto
#define sip__type__error_info__proto 				sip__type__encode__proto
#define sip__type__content_encoding__proto 			sip__type__encode__proto
#define sip__type__proxy_authentication__proto 		sip__type__authentication__proto
#define sip__type__proxy_authorization__proto 		sip__type__authorization__proto
#define sip__type__proxy_authenticate__proto 		sip__type__authenticate__proto
#define sip__type__contact__proto 					sip__type__address__proto
#define sip__type__record_route__proto 				sip__type__address__proto
#define sip__type__route__proto 					sip__type__address__proto
#define sip__type__other__proto 					sip__type__pair__proto
#define sip__type__via_params__proto 				sip__type__pair__proto
#define sip__type__headers__proto 					sip__type__pair__proto
#define sip__type__gen_params__proto 				sip__type__pair__proto
#define sip__type__url_params__proto				sip__type__pair__proto
#define sip__type__url_headers__proto				sip__type__pair__proto

#define sip__type__e_emails__proto					sip__type__char__proto
#define sip__type__p_phones__proto					sip__type__char__proto
#define sip__type__r_repeats__proto					sip__type__char__proto
#define sip__type__m_payloads__proto				sip__type__char__proto
#define sip__type__c_connections__proto				sip__type__sdp__connection__proto
#define sip__type__b_bandwidths__proto				sip__type__sdp__bandwidth__proto
#define sip__type__a_attributes__proto				sip__type__sdp__attribute__proto
#define sip__type__m_medias__proto					sip__type__sdp__media__proto
#define sip__type__t_descrs__proto					sip__type__sdp__time__proto

Sip__Type__CallId *sip__type__call_id__new(const char *, const char *);

Sip__Type__Cseq *sip__type__cseq__new(const char *, const char *);

Sip__Type__Pair *sip__type__pair__new(const char *, const char *);

Sip__Type__Uri *sip__type__uri__new(const char *, const char *, const char *, const char *, const char *,
                                    const char *);

Sip__Type__Address *sip__type__address__new(const char *);

Sip__Type__ContentType *sip__type__content_type__new(const char *, const char *);

Sip__Type__Authorization *sip__type__authorization__new(const char *, const char *, const char *,
                                                        const char *, const char *, const char *, const char *,
                                                        const char *, const char *, const char *,
                                                        const char *, const char *, const char *);

Sip__Type__Authenticate *sip__type__authenticate__new(const char *, const char *, const char *, const char *,
                                                      const char *, const char *, const char *, const char *,
                                                      const char *);

Sip__Type__Authentication *sip__type__authentication__new(const char *, const char *, const char *,
                                                          const char *, const char *);

Sip__Type__Encode *sip__type__encode__new(const char *);

Sip__Type__Via *sip__type__via__new(const char *, const char *, const char *, const char *, const char *);

Sip__Type__CallId *sip__type__call_id__proto(const osip_call_id_t *);

Sip__Type__Cseq *sip__type__cseq__proto(const osip_cseq_t *);

Sip__Type__Pair *sip__type__pair__proto(const osip_generic_param_t *);

Sip__Type__Uri *sip__type__uri__proto(const osip_uri_t *);

Sip__Type__Address *sip__type__address__proto(const osip_from_t *);

Sip__Type__ContentType *sip__type__content_type__proto(const osip_content_type_t *);

Sip__Type__Content *sip__type__content__proto(const osip_body_t *);

Sip__Type__Authorization *sip__type__authorization__proto(const osip_authorization_t *);

Sip__Type__Authenticate *sip__type__authenticate__proto(const osip_www_authenticate_t *);

Sip__Type__Authentication *sip__type__authentication__proto(const osip_authentication_info_t *);

Sip__Type__Encode *sip__type__encode__proto(const osip_accept_encoding_t *);

Sip__Type__Via *sip__type__via__proto(const osip_via_t *);

Sip__Type__Sdp__Connection *sip__type__sdp__connection__new(const char *, const char *, const char *,
                                                            const char *, const char *);

Sip__Type__Sdp__Bandwidth *sip__type__sdp__bandwidth__new(const char *, const char *);

Sip__Type__Sdp__Time *sip__type__sdp__time__new(const char *, const char *);

Sip__Type__Sdp__Key *sip__type__sdp__key__new(const char *, const char *);

Sip__Type__Sdp__Attribute *sip__type__sdp__attribute__new(const char *, const char *);

Sip__Type__Sdp__Media *sip__type__sdp__media__new(const char *, const char *, const char *, const char *,
                                                  const char *);

Sip__Type__Sdp *sip__type__sdp__new(const char *, const char *, const char *, const char *, const char *,
                                    const char *, const char *, const char *, const char *, const char *, const char *);

Sip__Type__Sdp__Connection *sip__type__sdp__connection__proto(const sdp_connection_t *);

Sip__Type__Sdp__Bandwidth *sip__type__sdp__bandwidth__proto(const sdp_bandwidth_t *);

Sip__Type__Sdp__Time *sip__type__sdp__time__proto(const sdp_time_descr_t *);

Sip__Type__Sdp__Key *sip__type__sdp__key__proto(const sdp_key_t *);

Sip__Type__Sdp__Attribute *sip__type__sdp__attribute__proto(const sdp_attribute_t *);

Sip__Type__Sdp__Media *sip__type__sdp__media__proto(const sdp_media_t *);

Sip__Type__Sdp *sip__type__sdp__proto(const sdp_message_t *);

Sip__Query *sip__query__proto(const osip_message_t *, int);

Sip__Answer *sip__answer__proto(const osip_message_t *, int);

#define sip__type__accept__unproto 					sip__type__content_type__unproto
#define sip__type__accept_encoding__unproto 		sip__type__encode__unproto
#define sip__type__accept_language__unproto 		sip__type__encode__unproto
#define sip__type__alert_info__unproto 				sip__type__encode__unproto
#define sip__type__call_info__unproto 				sip__type__encode__unproto
#define sip__type__error_info__unproto 				sip__type__encode__unproto
#define sip__type__content_encoding__unproto 		sip__type__encode__unproto
#define sip__type__proxy_authentication__unproto	sip__type__authentication__unproto
#define sip__type__proxy_authorization__unproto 	sip__type__authorization__unproto
#define sip__type__proxy_authenticate__unproto 		sip__type__authenticate__unproto
#define sip__type__contact__unproto 				sip__type__address__unproto
#define sip__type__record_route__unproto 			sip__type__address__unproto
#define sip__type__route__unproto 					sip__type__address__unproto
#define sip__type__other__unproto 					sip__type__pair__unproto
#define sip__type__via_params__unproto 				sip__type__pair__unproto
#define sip__type__headers__unproto 				sip__type__pair__unproto
#define sip__type__gen_params__unproto 				sip__type__pair__unproto
#define sip__type__url_params__unproto				sip__type__pair__unproto
#define sip__type__url_headers__unproto				sip__type__pair__unproto

#define sip__type__e_emails__unproto				sip__type__char__unproto
#define sip__type__p_phones__unproto				sip__type__char__unproto
#define sip__type__b_bandwidths__unproto			sip__type__sdp__bandwidth__unproto
#define sip__type__t_descrs__unproto				sip__type__sdp__time__unproto
#define sip__type__a_attributes__unproto			sip__type__sdp__attribute__unproto
#define sip__type__m_medias__unproto				sip__type__sdp__media__unproto
#define sip__type__r_repeats__unproto 				sip__type__char__unproto
#define sip__type__m_payloads__unproto 				sip__type__char__unproto
#define sip__type__c_connections__unproto			sip__type__sdp__connection__unproto

osip_call_id_t *sip__type__call_id__unproto(const Sip__Type__CallId *);

osip_cseq_t *sip__type__cseq__unproto(const Sip__Type__Cseq *);

osip_generic_param_t *sip__type__pair__unproto(const Sip__Type__Pair *);

osip_uri_t *sip__type__uri__unproto(const Sip__Type__Uri *);

osip_from_t *sip__type__address__unproto(const Sip__Type__Address *);

osip_content_type_t *sip__type__content_type__unproto(const Sip__Type__ContentType *);

osip_body_t *sip__type__content__unproto(const Sip__Type__Content *);

osip_authorization_t *sip__type__authorization__unproto(const Sip__Type__Authorization *);

osip_www_authenticate_t *sip__type__authenticate__unproto(const Sip__Type__Authenticate *);

osip_authentication_info_t *sip__type__authentication__unproto(const Sip__Type__Authentication *);

osip_accept_encoding_t *sip__type__encode__unproto(const Sip__Type__Encode *);

osip_via_t *sip__type__via__unproto(const Sip__Type__Via *);

sdp_connection_t *sip__type__sdp__connection__unproto(const Sip__Type__Sdp__Connection *);

sdp_key_t *sip__type__sdp__key__unproto(const Sip__Type__Sdp__Key *);

sdp_bandwidth_t *sip__type__sdp__bandwidth__unproto(const Sip__Type__Sdp__Bandwidth *);

sdp_time_descr_t *sip__type__sdp__time__unproto(const Sip__Type__Sdp__Time *);

sdp_attribute_t *sip__type__sdp__attribute__unproto(const Sip__Type__Sdp__Attribute *);

sdp_media_t *sip__type__sdp__media__unproto(const Sip__Type__Sdp__Media *);

sdp_message_t *sip__type__sdp__unproto(const Sip__Type__Sdp *);

osip_message_t *sip__query__unproto(const Sip__Query *, const char *, unsigned, int *id);

osip_message_t *sip__answer__unproto(const Sip__Answer *, unsigned, int *id);

enum {
	SIP__BIT__FROM,
	SIP__BIT__TO,
	SIP__BIT__CALLID,
	SIP__BIT__CSEQ,
	SIP__BIT__METHOD,
	SIP__BIT__REQUEST,
	SIP__BIT__URI = SIP__BIT__REQUEST,
	SIP__BIT__RESPONSE,
	SIP__BIT__REASON,
	SIP__BIT__VERSION,
	SIP__BIT__VIA,
	SIP__BIT__ALLOW,
	SIP__BIT__CONTENT,
	SIP__BIT__CONTENT_TYPE,
	SIP__BIT__AUTHENTICATE,
	SIP__BIT__AUTHENTICATION,
	SIP__BIT__AUTHORIZATION,
	SIP__BIT__PROXY_AUTHENTICATE,
	SIP__BIT__PROXY_AUTHENTICATION,
	SIP__BIT__PROXY_AUTHORIZATION,
	SIP__BIT__CONTACT,
	SIP__BIT__RECORD_ROUTE,
	SIP__BIT__ROUTE,
	SIP__BIT__CONTENT_ENCODING,
	SIP__BIT__ACCEPT,
	SIP__BIT__ACCEPT_ENCODING,
	SIP__BIT__ACCEPT_LANGUAGE,
	SIP__BIT__ALERT_INFO,
	SIP__BIT__CALL_INFO,
	SIP__BIT__ERROR_INFO,
	SIP__BIT__OTHER,
	SIP__BIT__30,
	SIP__BIT__31,
};

#define BITOF(__X)					(1UL << SIP__BIT##__X)
#define FROM_BIT					(BITOF(__FROM))
#define TO_BIT						(BITOF(__TO))
#define CALLID_BIT					(BITOF(__CALLID))
#define CSEQ_BIT					(BITOF(__CSEQ))
#define METHOD_BIT					(BITOF(__METHOD))
#define REQUEST_BIT					(BITOF(__REQUEST))
#define URI_BIT						(BITOF(__URI))
#define RESPONSE_BIT				(BITOF(__RESPONSE))
#define REASON_BIT					(BITOF(__REASON))
#define VERSION_BIT					(BITOF(__VERSION))
#define VIA_BIT						(BITOF(__VIA))
#define ALLOW_BIT					(BITOF(__ALLOW))
#define CONTENT_BIT					(BITOF(__CONTENT))
#define CONTENT_TYPE_BIT			(BITOF(__CONTENT_TYPE))
#define AUTHENTICATE_BIT			(BITOF(__AUTHENTICATE))
#define AUTHENTICATION_BIT			(BITOF(__AUTHENTICATION))
#define AUTHORIZATION_BIT			(BITOF(__AUTHORIZATION))
#define PROXY_AUTHENTICATE_BIT		(BITOF(__PROXY_AUTHENTICATE))
#define PROXY_AUTHENTICATION_BIT	(BITOF(__PROXY_AUTHENTICATION))
#define PROXY_AUTHORIZATION_BIT		(BITOF(__PROXY_AUTHORIZATION))
#define CONTACT_BIT					(BITOF(__CONTACT))
#define RECORD_ROUTE_BIT			(BITOF(__RECORD_ROUTE))
#define ROUTE_BIT					(BITOF(__ROUTE))
#define CONTENT_ENCODING_BIT		(BITOF(__CONTENT_ENCODING))
#define ACCEPT_BIT					(BITOF(__ACCEPT))
#define ACCEPT_ENCODING_BIT			(BITOF(__ACCEPT_ENCODING))
#define ACCEPT_LANGUAGE_BIT			(BITOF(__ACCEPT_LANGUAGE))
#define ALERT_INFO_BIT				(BITOF(__ALERT_INFO))
#define CALL_INFO_BIT				(BITOF(__CALL_INFO))
#define ERROR_INFO_BIT				(BITOF(__ERROR_INFO))
#define OTHER_BIT					(BITOF(__OTHER))
#define FULL_RESPONSE_BITSET		(0XFFFFFFFFUL)
#define FULL_REQUEST_BITSET			(0XFFFFFFFFUL)
#define FROMTOCALLSEQ_BITSET		(FROM_BIT|TO_BIT|CALLID_BIT|CSEQ_BIT)
#define RESPONSE_BITSET				(BITOF(__RESPONSE)|BITOF(__REASON)|BITOF(__VERSION)|BITOF(__CONTENT)|BITOF(__CONTENT_TYPE))
#define REQUEST_BITSET				(BITOF(__METHOD)|BITOF(__REQUEST)|BITOF(__URI)|BITOF(__CONTENT)|BITOF(__CONTENT_TYPE))
#define AUTHENTICATE_BITSET			(BITOF(__PROXY_AUTHENTICATE)|BITOF(__AUTHENTICATE))
#define AUTHORIZATION_BITSET		(BITOF(__PROXY_AUTHORIZATION)|BITOF(__AUTHORIZATION))
#define GENERIC_BITSET				(BITOF(__CONTACT)|BITOF(__VIA)|RECORD_ROUTE_BIT|ROUTE_BIT)
#define SHORT_RESPONSE_BITSET		(RESPONSE_BITSET|FROMTOCALLSEQ_BITSET|GENERIC_BITSET)
#define REGISTER_RESPONSE_BITSET	(RESPONSE_BITSET|FROMTOCALLSEQ_BITSET|AUTHENTICATE_BITSET|GENERIC_BITSET)
#define REGISTER_REQUEST_BITSET		(REQUEST_BITSET|FROMTOCALLSEQ_BITSET|AUTHORIZATION_BITSET|GENERIC_BITSET)

#define STATUS_RESPONSE_BITSET(s)	(s == SIP_OK ? FULL_RESPONSE_BITSET : SHORT_RESPONSE_BITSET)

#endif /* SIP_H_ */
