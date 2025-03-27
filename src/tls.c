/*
 * tls.c
 *
 *  Created on: Nov 18, 2013
 *      Author: shved
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/tcp.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/conf.h>
#include <openssl/engine.h>
#include <openssl/safestack.h>

#include "net.h"
#include "log.h"

/**
 *
 */
static struct TLS_CTX {

	unsigned reference;
	SSL_CTX *context;

} __extra = { 0, NULL };

static int __id = 1;

/**
 *
 */
static int tls_free(const net_t *net) {

	struct TLS_CTX *TLS = net->foo;
	if (!TLS)
		return -1;

	log_debug("%s: %p %u", __PRETTY_FUNCTION__, TLS->context, TLS->reference);

	if (TLS->reference != 0) {
		TLS->reference--;
		return 0;
	} else
		SSL_CTX_free(TLS->context);

	sk_SSL_COMP_free(SSL_COMP_get_compression_methods());

	CONF_modules_unload(1);
	CONF_modules_free();
	ENGINE_cleanup();
	ERR_remove_state(0);
	ERR_free_strings();
	EVP_cleanup();
	CRYPTO_cleanup_all_ex_data();

	TLS->context = NULL;

	return 0;
}

/**
 *
 */
static int __password(char *buf, int num, int rwflag, void *userdata) {

	if (!userdata)
		return 0;
	int len = strlen(userdata);
	if (!len)
		return 0;
	if (num < len + 1)
		return 0;
	strcpy(buf, userdata);

	return len;
}

/**
 *
 */
int __verify(int preverify_ok, X509_STORE_CTX * store) {

//	char buf[256];
//	X509 *err_cert;
//	int err, depth;
//
//	err_cert = X509_STORE_CTX_get_current_cert(store);
//	err = X509_STORE_CTX_get_error(store);
//	depth = X509_STORE_CTX_get_error_depth(store);
//
//	X509_NAME_oneline(X509_get_subject_name(err_cert), buf, 256);
//
//	if (depth > verify_depth /* depth -1 */) {
//		preverify_ok = 0;
//		err = X509_V_ERR_CERT_CHAIN_TOO_LONG;
//		X509_STORE_CTX_set_error(store, err);
//	}
//	if (!preverify_ok) {
//		OSIP_TRACE(
//				osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL, "verify error:num=%d:%s:depth=%d:%s\n", err,
//						X509_verify_cert_error_string(err), depth, buf));
//	} else {
//		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL, "depth=%d:%s\n", depth, buf));
//	}
//	/*
//	 * At this point, err contains the last verification error. We can use
//	 * it for something special
//	 */
//	if (!preverify_ok && (err == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT)) {
//		X509_NAME_oneline(X509_get_issuer_name(store->current_cert), buf, 256);
//		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL, "issuer= %s\n", buf));
//	}
//
//	if (tls_verify_client_certificate > 0)
//		return preverify_ok;
//
//	if (!preverify_ok && (err == X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN)) {
//		X509_NAME_oneline(X509_get_issuer_name(store->current_cert), buf, 256);
//		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL, "issuer= %s\n", buf));
//		preverify_ok = 1;
//		X509_STORE_CTX_set_error(store, X509_V_OK);
//	}
//
//	if (!preverify_ok && (err == X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT)) {
//		X509_NAME_oneline(X509_get_issuer_name(store->current_cert), buf, 256);
//		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL, "issuer= %s\n", buf));
//		preverify_ok = 1;
//		X509_STORE_CTX_set_error(store, X509_V_OK);
//	}
//
//	if (!preverify_ok && (err == X509_V_ERR_CERT_HAS_EXPIRED)) {
//		X509_NAME_oneline(X509_get_issuer_name(store->current_cert), buf, 256);
//		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL, "issuer= %s\n", buf));
//		preverify_ok = 1;
//		X509_STORE_CTX_set_error(store, X509_V_OK);
//	}
//
//	if (!preverify_ok && (err == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY)) {
//		X509_NAME_oneline(X509_get_issuer_name(store->current_cert), buf, 256);
//		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL, "issuer= %s\n", buf));
//		preverify_ok = 1;
//		X509_STORE_CTX_set_error(store, X509_V_OK);
//	}
//
//	if (!preverify_ok && (err == X509_V_ERR_CERT_UNTRUSTED)) {
//		X509_NAME_oneline(X509_get_issuer_name(store->current_cert), buf, 256);
//		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL, "issuer= %s\n", buf));
//		preverify_ok = 1;
//		X509_STORE_CTX_set_error(store, X509_V_OK);
//	}
//
//	if (!preverify_ok && (err == X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE)) {
//		X509_NAME_oneline(X509_get_issuer_name(store->current_cert), buf, 256);
//		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL, "issuer= %s\n", buf));
//		preverify_ok = 1;
//		X509_STORE_CTX_set_error(store, X509_V_OK);
//	}
//
	preverify_ok = 1; /* configured to accept anyway! */
	return preverify_ok;
}

#define PASSWORD	"pa55phra5e"
#define CERTFILE	"cert.pem"
#define ROOTFILE	"root.pem"
#define PKEYFILE	"pkey.pem"
#define DHPARAMS	"dh.pem"
#define VERIFYDEPTH	10

/**
 *
 */
static int tls_init(const net_t *net) {

	struct TLS_CTX *TLS = net->foo;
	if (!TLS)
		return -1;

	log_debug("%s: %p %u", __PRETTY_FUNCTION__, TLS->context, TLS->reference);

	if (TLS->context != NULL ) {
		TLS->reference++;
		return 0;
	}
	SSL_load_error_strings();
	SSL_library_init();

	TLS->context = SSL_CTX_new(TLSv1_server_method());

	RSA *rsa = RSA_generate_key(512, RSA_F4, NULL, NULL );
	if (rsa) {
		if (!SSL_CTX_ctrl(TLS->context, SSL_CTRL_SET_TMP_RSA, 0, rsa)) {
			RSA_free(rsa);
			log_alert("RSA_generate_key failed");
			return -1;
		}
		RSA_free(rsa);
	}

	SSL_CTX_set_session_id_context(TLS->context, (void *) &__id, sizeof(__id));
	SSL_CTX_set_default_passwd_cb_userdata(TLS->context, PASSWORD);
	SSL_CTX_set_default_passwd_cb(TLS->context, __password);

	if (!SSL_CTX_use_certificate_file(TLS->context, CERTFILE, SSL_FILETYPE_PEM)) {
		log_alert("SSL_CTX_use_certificate_file failed");
		return -1;
	}

	if (!SSL_CTX_load_verify_locations(TLS->context, ROOTFILE, NULL )) {
		log_alert("SSL_CTX_load_verify_locations failed");
		return -1;
	} else if (0) {
		int mode = SSL_VERIFY_NONE;

		SSL_CTX_set_verify(TLS->context, mode, &__verify);
		SSL_CTX_set_verify_depth(TLS->context, VERIFYDEPTH + 1);
	}
	SSL_CTX_ctrl((TLS->context), SSL_CTRL_OPTIONS,
			SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION
					| SSL_OP_CIPHER_SERVER_PREFERENCE 
#if OPENSSL_VERSION_NUMBER > 0x0090819fL
					| SSL_OP_NO_COMPRESSION
#endif
					, NULL );

	if (!SSL_CTX_use_PrivateKey_file(TLS->context, PKEYFILE, SSL_FILETYPE_PEM)) {
		log_alert("SSL_CTX_use_PrivateKey_file failed");
		return -1;
	}
	if (!SSL_CTX_check_private_key(TLS->context)) {
		log_alert("SSL_CTX_check_private_key failed");
		return -1;
	}

	BIO *bio = BIO_new_file(DHPARAMS, "r");
	if (!bio) {
		log_alert("BIO_new_file failed");
		return -1;
	}

	DH *dh = PEM_read_bio_DHparams(bio, NULL, NULL, NULL );
	if (!dh) {
		BIO_free(bio);
		log_alert("PEM_read_bio_DHparams failed");
		return -1;
	}
	if (SSL_CTX_ctrl(TLS->context, SSL_CTRL_SET_TMP_DH, 0, dh) < 0) {
		log_alert("SSL_CTX_ctrl failed");
		BIO_free(bio);
		DH_free(dh);
		return -1;
	}
	BIO_free(bio);
	DH_free(dh);

	__id++;

	return 0;
}

/**
 *
 */
static int tls_open(const net_t *net, const char *host, unsigned short port) {

	log_debug("%s: %s:%hu", __PRETTY_FUNCTION__, host, net->flag + port);

	int s = socket(net->family, net->type, net->protocol);
	if (s != -1) {
		int yes = 1;
		int not = 0;
		int flag;

		flag = fcntl(s, F_GETFL);
		if (flag != -1) {
			flag |= O_NONBLOCK;
			if (fcntl(s, F_SETFL, flag) == -1) {
				close(s);
				return -1;
			}
		}

		setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
#ifdef SO_REUSEPORT
		setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(int));
#endif
		setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &not, sizeof(int));

		struct sockaddr_in la;

		memset(&la, 0, sizeof(la));

		la.sin_addr.s_addr = inet_addr(host);
		la.sin_family = abs(net->family);
		la.sin_port = htons(net->flag + port);

		if (bind(s, (struct sockaddr *) &la, sizeof(la)) != -1) {
			if (listen(s, 32) != -1) {
				return s;
			}
		}
		close(s);
	}
	return -1;
}

/**
 *
 */
static int tls_connect(const net_t *net, const char *host, unsigned short port) {

	log_debug("%s: Not Implemented yet", __PRETTY_FUNCTION__);

	return -1;
}

#include <assert.h>

/**
 *
 */
static int tls_read(int s, mem_t *m, void *foo) {

	if (foo) {
		if (m->end < m->length) {
			int n = SSL_read(foo, m->buffer + m->end, m->length - m->end);
			if (SSL_get_error(foo, n) == SSL_ERROR_NONE) {
				m->end += n;
				return n;
			}
		}
	}
	return -1;
}

/**
 *
 */
static int tls_send(int s, mem_t *m, void *foo) {

	if (foo) {
		if (m->done < m->end) {
			int n = SSL_write(foo, m->buffer + m->done, m->end - m->done);
			if (SSL_get_error(foo, n) == SSL_ERROR_NONE) {
				m->done += n;
				return n;
			}
		} else if (m->done == m->end)
			return 0;
	}
	return -1;
}

/**
 *
 */
static int tls_accept(int s, struct sockaddr *a, socklen_t *l) {

	int c = accept(s, a, l);

	struct sockaddr_in *sin = (struct sockaddr_in *) a;

	log_debug("%s: %i/%s:%hu", __PRETTY_FUNCTION__, c, inet_ntoa(sin->sin_addr), ntohs(sin->sin_port));

	return c;
}

/**
 *
 */
static int tls_dismiss(void **foo) {

	if (!foo)
		return -1;

	SSL *ssl = *foo;
	if (ssl) {
		int n = SSL_shutdown(ssl);
		if (!n) {
			int s = SSL_get_fd(ssl);
			if (s != -1) {
				shutdown(s, 1);
				SSL_shutdown(ssl);
			}
		}
		SSL_free(ssl);
	}
	*foo = NULL;

	return 0;
}

/**
 *
 */
static int tls_assign(const net_t *net, int fd, int type, void **foo) {

	struct TLS_CTX *TLS = net->foo;
	if (!TLS)
		return -1;

	if (type != NET_TYPE_ACCEPT)
		return 0;

	SSL *ssl = *foo = SSL_new(TLS->context);
	if (ssl) {
		if (1 == SSL_set_fd(ssl, fd))
			if (1 == SSL_accept(ssl)) {
				return 0;
			}
	}
	return tls_dismiss(foo);
}

/**
 *
 */
const net_t __g_net_TLS = {
	"TLS",
	1,
	IPPROTO_TCP,
	AF_INET,
	SOCK_STREAM,
	tls_init,
	tls_free,
	tls_open,
	tls_read,
	tls_send,
    NULL,
	tls_connect,
	tls_accept,
	tls_assign,
	tls_dismiss,
	&__extra };
