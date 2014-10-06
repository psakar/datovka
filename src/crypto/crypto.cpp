

#include <cassert>
#include <openssl/bio.h>
#include <openssl/cms.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ts.h>
#include <openssl/x509v3.h>

#include "src/crypto/crypto.h"
#include "src/log/log.h"


static
X509_STORE *ca_certs = NULL;


/*!
 * @brief Read certificate file.
 */
static
int X509_store_add_cert_file(X509_STORE *store, const char *fname);


/*!
 * @brief Load CMS from buffer.
 *
 * @return NULL on failure.
 */
static
CMS_ContentInfo * load_cms(const void *raw, size_t raw_len);


/*!
 * @brief Verifies CMS signature.
 */
static
int cms_verify_signature(CMS_ContentInfo *cms);


#define CERT_FILE0 "trusted_certs/all_trusted.pem"
#define CERT_FILE1 "trusted_certs/postsignum_qca_root.pem"
#define CERT_FILE2 "trusted_certs/postsignum_qca_sub.pem"
#define CERT_FILE3 "trusted_certs/postsignum_qca2_root.pem"
#define CERT_FILE4 "trusted_certs/postsignum_qca2_sub.pem"


/* ========================================================================= */
/*
 * @brief Initialises cryptographic back-end.
 */
int init_crypto(void)
/* ========================================================================= */
{
	debug_func_call();

	ERR_load_crypto_strings();
	ERR_load_CMS_strings();
	//ERR_free_strings();

	if (NULL != ca_certs) {
		X509_STORE_free(ca_certs); ca_certs = NULL;
	}

	ca_certs = X509_STORE_new();
	if (NULL == ca_certs) {
		goto fail;
	}

//	if (0 != X509_store_add_cert_file(ca_certs, CERT_FILE0)) {
//		goto fail;
//	}
	if (0 != X509_store_add_cert_file(ca_certs, CERT_FILE1)) {
		goto fail;
	}
	if (0 != X509_store_add_cert_file(ca_certs, CERT_FILE2)) {
		goto fail;
	}
	if (0 != X509_store_add_cert_file(ca_certs, CERT_FILE3)) {
		goto fail;
	}
	if (0 != X509_store_add_cert_file(ca_certs, CERT_FILE4)) {
		goto fail;
	}

	return 0;

fail:
	if (NULL != ca_certs) {
		X509_STORE_free(ca_certs); ca_certs = NULL;
	}
	return -1;
}


/* ========================================================================= */
/*
 * Verifies signed message signature.
 */
int verify_raw_message_signature(const void *raw, size_t raw_len)
/* ========================================================================= */
{
	int ret;

	CMS_ContentInfo *cms = NULL;

	debug_func_call();

	cms = load_cms(raw, raw_len);
	if (NULL == cms) {
		logError("%s\n", "Could not load CMS.");
		goto fail;
	}

	ret = cms_verify_signature(cms);

	CMS_ContentInfo_free(cms); cms = NULL;

	return ret;

fail:
	if (NULL != cms) {
		CMS_ContentInfo_free(cms);
	}
	ERR_free_strings();
	return -1;
}


/* ========================================================================= */
/*
 * Read certificate file.
 */
static
int X509_store_add_cert_file(X509_STORE *store, const char *fname)
/* ========================================================================= */
{
	FILE *fin;
	X509 *x509 = NULL;
	unsigned long err;

	debug_func_call();

	assert(NULL != store);
	if (NULL == store) {
		goto fail;
	}
	assert(NULL != fname);
	if (NULL == fname) {
		goto fail;
	}

	fin = fopen(fname, "r");
	if (fin == NULL) {
		logError("Cannot open certificate file '%s'.\n", fname);
		goto fail;
	}

	x509 = PEM_read_X509(fin, NULL, NULL, NULL);
	if (x509 == NULL) {
		logError("Cannot parse certificate file '%s'.\n", fname);
		while (0 != (err = ERR_get_error())) {
			logError("openssl error: %s\n",
			    ERR_error_string(err, NULL));
		}
		goto fail;
	}

	if (X509_STORE_add_cert(store, x509) == 0) {
		err = ERR_get_error();
		logError("Cannot store certificate(s) from file '%s'.\n",
		    fname);
		while (0 != (err = ERR_get_error())) {
			logError("openssl error: %s\n",
			    ERR_error_string(err, NULL));
		}
		goto fail;
	}

	X509_free(x509); x509 = NULL;
	fclose(fin); fin = NULL;

	return 0;

fail:
	if (fin != NULL) {
		fclose(fin);
	}
	if (x509 != NULL) {
		X509_free(x509);
	}
	return -1;
}


/* ========================================================================= */
/*
 * Load CMS from buffer.
 */
static
CMS_ContentInfo * load_cms(const void *raw, size_t raw_len)
/* ========================================================================= */
{
	BIO *bio = NULL;
	CMS_ContentInfo *cms = NULL;
	unsigned long err;

	debug_func_call();

	bio = BIO_new_mem_buf((void *) raw, raw_len);
	if (NULL == bio) {
		logError("%s\n", "Cannot create memory BIO.");
		while (0 != (err = ERR_get_error())) {
			fprintf(stderr, "openssl error '%s'\n",
			    ERR_error_string(err, NULL));
		}
		goto fail;
	}

	cms = d2i_CMS_bio(bio, NULL);
	if (NULL == cms) {
		logError("%s\n", "Cannot parse CMS.");
		while (0 != (err = ERR_get_error())) {
			logError("openssl error: %s\n",
			    ERR_error_string(err, NULL));
		}
		goto fail;
	}

	BIO_free(bio); bio = NULL;

	return cms;

fail:
	if (NULL != bio) {
		BIO_free(bio);
	}
	return NULL;
}


/* ========================================================================= */
/*
 * Verifies CMS signature.
 */
static
int cms_verify_signature(CMS_ContentInfo *cms)
/* ========================================================================= */
{
	const ASN1_OBJECT *asn1;
	int nid;
	unsigned long err;

	debug_func_call();

	assert(NULL != cms);

	/* Must be signed data. */
	asn1 = CMS_get0_type(cms);
	nid = OBJ_obj2nid(asn1);
	switch (nid) {
	case NID_pkcs7_signed:
		break;
	default:
		assert(0);
		goto fail;
		break;
	}

	if (!CMS_verify(cms, NULL, NULL, NULL, NULL,
	        CMS_NO_SIGNER_CERT_VERIFY)) {
		logWarning("%s\n", "Could not verify CMS.");
		while (0 != (err = ERR_get_error())) {
			logError("openssl error: %s\n",
			    ERR_error_string(err, NULL));
		}
		return 0;
	}

	return 1;

fail:
	return -1;
}
