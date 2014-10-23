

#include <assert.h>
#include <ctype.h> /* isdigit(3) */
#include <openssl/bio.h>
#include <openssl/cms.h>
#include <openssl/err.h>
#include <openssl/evp.h> /* OpenSSL_add_all_algorithms() */
#include <openssl/pem.h>
#include <openssl/ts.h>
#include <openssl/x509v3.h>
#include <string.h>

#include "src/compat/compat_win.h"
#include "src/crypto/crypto.h"
#include "src/log/log_c.h"


/*
 * Directory to search for certificate. The name mist end with a slash.
 */
#define CERT_DIR "trusted_certs/"

/*
 * Defies path prefix where to search for CERT_DIR.
 * The directory name must contain trailing slash.
 */
#ifndef CERT_PATH_PREFIX
#define CERT_PATH_PREFIX ""
#endif /* CERT_PATH_PREFIX */

/*!
 * @brief Holds NULL-terminated list of PEM encoded certificate files.
 */
extern const char *pem_files[];


/*!
 * @brief Holds a PEM encoded certificate string and a certificate name.
 */
struct pem_str {
	const char *name; /*!< Certificate name. */
	const char *pem; /*!< PEM encoded certificate. */
};


/*!
 * @brief Holds NULL-terminated list of PEM encoded certificates.
 */
extern const struct pem_str pem_strs[];

/*!
 * @brief Certificates used for CMS validation.
 */
static
X509_STORE *ca_certs = NULL;


/*!
 * @brief Read certificate from a DER string.
 *
 * @param[in,out] store   Certificate storage.
 * @param[in]     der_str Certificate string.
 * @return 0 certificate was parsed and was stored, -1 else.
 */
static
int X509_store_add_cert_der(X509_STORE *store, const char *der_str);


/*!
 * @brief Read certificate file.
 *
 * @param[in,out] store      Certificate storage.
 * @param[in]     fname      Full name of file containing certificates.
 * @param[out]    num_loaded Number of loaded certificates.
 * @return 0 if at least one certificate could be loaded, -1 else.
 */
static
int X509_store_add_cert_file(X509_STORE *store, const char *fname,
    int *num_loaded);


/*!
 * @brief Load CMS from buffer.
 *
 * @return NULL on failure.
 */
static
CMS_ContentInfo * load_cms(const void *raw, size_t raw_len);


/*!
 * @brief Load X509 from buffer.
 *
 * @return NULL on failure.
 */
static
X509 * load_x509(const void *raw, size_t raw_len);


/*!
 * @brief Verifies CMS signature.
 *
 * @param[in,out] cms       CMS message.
 * @param[in]     ca_store  Certificate against which to check the CMS
 *                          signature.
 * @param[in]     crl_check Whether to check CRL if available.
 * @return  1 if signature valid,
 *          0 if signature invalid,
 *         -1 on error.
 */
static
int cms_verify_signature(CMS_ContentInfo *cms, X509_STORE *ca_store,
    int crl_check);


/*!
 * @brief Get RFC 3161 time-stamp value.
 *
 * @param[in] cms       Time-stamp CMS.
 * @param[out] utc_time Parsed time value.
 * @return 0 on success, -1 on failure.
 */
static
int cms_get_timestamp_value(CMS_ContentInfo *cms, time_t *utc_time);


/*!
 * @brief Check certificate date.
 *
 * @param x509     Certificate to check.
 * @param utc_time Date to check the certificate against.
 * @return  1 if check ok,
 *          0 if check not ok,
 *         -1 on error.
 */
static
int x509_check_date(const X509 *x509, time_t utc_time);


/*!
 * @brief Converts ASN1_TIME into time_t UTC time.
 *
 * @param[in]  asn1_time ASN1 time string.
 * @param[out] utc_time Output UTC time.
 * @return 0 on success, -1 on failure.
 */
static
int asn1_time_to_utc_time(const ASN1_TIME *asn1_time, time_t *utc_time);


/*!
 * @brief Portable version of timegm().
 */
static
time_t timegm_utc(struct tm *tm);


/*!
 * @brief Converts X509 certificate to DER.
 *
 * @param[in]  x509    Certificate.
 * @param[out] der     Pointer to allocated buffer. Use free() top free memory.
 * @param[out] der_len Size of the allocate buffer.
 */
static
int x509_get_pem(X509 *x509, void **der, size_t *der_len);


//#define PRINT_CERTS 1


#if defined PRINT_CERTS
/*!
 * @brief Prints x509.
 */
static
int x509_printf(X509 *x509, FILE *fout);
#endif /* PRINT_CERTS */


/* ========================================================================= */
/*
 * @brief Initialises cryptographic back-end.
 */
int init_crypto(void)
/* ========================================================================= */
{
	const char **pem_file;
	const struct pem_str *pem_desc;
	int loaded_certificates = 0, loaded_at_once;
#define MAX_PATH_LEN 256
	size_t crt_dir_path_len, file_name_len;
	char file_path[MAX_PATH_LEN];

	debug_func_call();

	OpenSSL_add_all_algorithms(); /* Needed for CMS validation. */

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

	/* Load from files. */
	pem_file = pem_files;
	assert(NULL != pem_file);
	while (NULL != *pem_file) {
		/* Construct full file name. */
		crt_dir_path_len = strlen(CERT_PATH_PREFIX CERT_DIR);
		if (crt_dir_path_len >= MAX_PATH_LEN) {
			log_error("File path buffer is to short for '%s%s'.\n",
			    CERT_PATH_PREFIX, CERT_DIR);
			continue;
		}
		memcpy(file_path, CERT_PATH_PREFIX CERT_DIR, crt_dir_path_len);
		file_path[crt_dir_path_len] = '\0';
		file_name_len = strlen(*pem_file);
		if ((crt_dir_path_len + file_name_len) >= MAX_PATH_LEN) {
			log_error(
			    "File path buffer is to short for '%s%s%s'.\n",
			    CERT_PATH_PREFIX, CERT_DIR, *pem_file);
			continue;
		}
		memcpy(file_path + crt_dir_path_len, *pem_file, file_name_len);
		file_path[crt_dir_path_len + file_name_len] = '\0';

		if (0 != X509_store_add_cert_file(ca_certs, file_path,
		             &loaded_at_once)) {
			log_warning("Could not load certificate file '%s'.\n",
			    file_path);
		} else {
			loaded_certificates += loaded_at_once;
		}
		++pem_file;
	}

	/* Load from built-in certificates. */
	pem_desc = pem_strs;
	assert(NULL != pem_desc);
	while ((NULL != pem_desc->name) && (NULL != pem_desc->pem)) {
		if (0 != X509_store_add_cert_der(ca_certs, pem_desc->pem)) {
			log_warning("Could not load certificate '%s'.\n",
			    pem_desc->name);
		} else {
			++loaded_certificates;
		}
		++pem_desc;
	}

	if (0 == loaded_certificates) {
		log_error("%s\n", "Did not load any certificate.");
	}

	return 0;

fail:
	if (NULL != ca_certs) {
		X509_STORE_free(ca_certs); ca_certs = NULL;
	}
	return -1;
#undef MAX_PATH_LEN
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
		log_error("%s\n", "Could not load CMS.");
		goto fail;
	}

	ret = cms_verify_signature(cms, ca_certs, 0);

	CMS_ContentInfo_free(cms); cms = NULL;

	return ret;

fail:
	if (NULL != cms) {
		CMS_ContentInfo_free(cms);
	}
	return -1;
}


/* ========================================================================= */
/*
 * Verifies whether message signature was valid at given date.
 */
int verify_raw_message_signature_date(const void *raw, size_t raw_len,
    time_t utc_time, int crl_check)
/* ========================================================================= */
{
	int ret;
	CMS_ContentInfo *cms = NULL;
	STACK_OF(X509) *signers = NULL;
	unsigned long err;
	int num_signers;

	debug_func_call();

	cms = load_cms(raw, raw_len);
	if (NULL == cms) {
		log_error("%s\n", "Could not load CMS.");
		goto fail;
	}

	ret = cms_verify_signature(cms, ca_certs, crl_check);

	if (1 == ret) {
		signers = CMS_get0_signers(cms);
		if (NULL == signers) {
			log_error("%s\n", "Could not get CMS signers.");
			while (0 != (err = ERR_get_error())) {
				log_error("openssl error: %s\n",
				    ERR_error_string(err, NULL));
			}
			ret = -1;
			goto fail;
		}

		num_signers = sk_X509_num(signers);
		if (1 != num_signers) {
			log_error("Only one CMS signer expected, got %d.\n",
			    num_signers);
			ret = -1;
			goto fail;
		}

		ret = x509_check_date(sk_X509_value(signers, 0), utc_time);

		sk_X509_free(signers); signers = NULL;
	}

	CMS_ContentInfo_free(cms); cms = NULL;

	return ret;

fail:
	if (NULL != cms) {
		CMS_ContentInfo_free(cms);
	}
	if (NULL != signers) {
		sk_X509_free(signers);
	}
	return -1;
}


/* ========================================================================= */
/*
 * Verifies qualified time-stamp signature and parses the time-stamp
 *     value. Time-stamp format follows RFC 3161.
 */
int verify_qualified_timestamp(const void *data, size_t data_len,
    time_t *utc_time)
/* ========================================================================= */
{
	int ret;
	CMS_ContentInfo *cms = NULL;
//	unsigned long err;
#if defined PRINT_CERTS
	STACK_OF(X509) *signers = NULL;
#endif /* PRINT_CERTS */

	debug_func_call();

	cms = load_cms(data, data_len);
	if (NULL == cms) {
		log_error("%s\n", "Could not load CMS.");
		goto fail;
	}

	/*
	 * The timestamps are provided by the postsignum.cz certification
	 * authority.
	 * TODO -- Currently the signing certificate is not checked against
	 * any other certificate.
	 */
	ret = cms_verify_signature(cms, NULL, 0);
#if defined PRINT_CERTS
	signers = CMS_get0_signers(cms);
	x509_printf(sk_X509_value(signers, 0), stderr);
	sk_X509_free(signers); signers = NULL;
#endif /* PRINT_CERTS */

	if (-1 != ret) {
		assert(NULL != utc_time);
		if (NULL == utc_time) {
			goto fail;
		}

		if (-1 == cms_get_timestamp_value(cms, utc_time)) {
			goto fail;
		}
	}

	CMS_ContentInfo_free(cms); cms = NULL;

	return ret;

fail:
	if (NULL != cms) {
		CMS_ContentInfo_free(cms);
	}
	return -1;
}


/* ========================================================================= */
/*
 * Retrieve signing certificate from supplied CMS.
 */
int cms_signing_cert(const void *data, size_t data_len, void **sign_cert,
    size_t *cert_len)
/* ========================================================================= */
{
	int ret;
	CMS_ContentInfo * cms = NULL;
	STACK_OF(X509) *signers = NULL;
	int num_signers;

	cms = load_cms(data, data_len);
	if (NULL == cms) {
		log_error("%s\n", "Could not load CMS.");
		goto fail;
	}

	ret = cms_verify_signature(cms, NULL, 0);

	if (1 != ret) {
		log_error("%s\n", "Could not validate CMS");
		goto fail;
	}

	signers = CMS_get0_signers(cms);
	if (NULL == signers) {
		goto fail;
	}

	num_signers = sk_X509_num(signers);
	if (1 != num_signers) {
		log_error("Only one CMS signer expected, got %d.\n",
		    num_signers);
		goto fail;
	}

	if (-1 == x509_get_pem(sk_X509_value(signers, 0),
	        sign_cert, cert_len)) {
		goto fail;
	}

	sk_X509_free(signers); signers = NULL;

	CMS_ContentInfo_free(cms); cms = NULL;

	return 0;
fail:
	if (NULL != cms) {
		CMS_ContentInfo_free(cms);
	}
	if (NULL != signers) {
		sk_X509_free(signers);
	}
	return -1;
}


/* ========================================================================= */
/*
 * Get some certificate information.
 */
int cert_information(const void *data, size_t data_len,
    char **o, char **ou, char **n, char **c)
/* ========================================================================= */
{
	X509 *x509 = NULL;
	const X509_NAME *subject;
	const STACK_OF(X509_NAME_ENTRY) *entries;
	int num_entries, i;
	X509_NAME_ENTRY *entry;
	ASN1_OBJECT *object;
	int nid;
	ASN1_STRING *str;
	char **out_str;

	x509 = load_x509(data, data_len);
	if (NULL == x509) {
		goto fail;
	}

	subject = X509_get_subject_name(x509);
	if (NULL == subject) {
		goto fail;
	}

	entries = subject->entries;
	num_entries = sk_X509_NAME_ENTRY_num(entries);

	for (i = 0; i < num_entries; ++i) {
		entry = sk_X509_NAME_ENTRY_value(entries, i);
		object = X509_NAME_ENTRY_get_object(entry);
		/* Reading object->nid doesn't work. */
		nid = OBJ_obj2nid(object);
		str = X509_NAME_ENTRY_get_data(entry);

		switch (nid) {
		case NID_organizationName:
			if (NULL != o) {
				out_str = o;
			}
			break;
		case NID_organizationalUnitName:
			if (NULL != ou) {
				out_str = ou;
			}
			break;
		case NID_commonName:
			if (NULL != n) {
				out_str = n;
			}
			break;
		case NID_countryName:
			if (NULL != c) {
				out_str = c;
			}
			break;
		default:
			out_str = NULL;
		}

		if (NULL != out_str) {
			*out_str = malloc(ASN1_STRING_length(str) + 1);
			if (NULL != *out_str) {
				memcpy(*out_str, ASN1_STRING_data(str),
				    ASN1_STRING_length(str));
				(*out_str)[ASN1_STRING_length(str)] = '\0';
			}
		}
	}

	X509_free(x509); x509 = NULL;

	return 0;

fail:
	if (NULL != x509) {
		X509_free(x509);
	}
	return -1;
}


/* ========================================================================= */
/*!
 * @brief Read certificate from der string.
 */
static
int X509_store_add_cert_der(X509_STORE *store, const char *der_str)
/* ========================================================================= */
{
	BIO *bio = NULL;
	X509 *x509 = NULL;
	unsigned long err;

	debug_func_call();

	assert(NULL != store);
	if (NULL == store) {
		goto fail;
	}
	assert(NULL != der_str);
	if (NULL == der_str) {
		goto fail;
	}

	bio = BIO_new_mem_buf((void *) der_str, strlen(der_str));
	if (NULL == bio) {
		log_error("%s\n", "Cannot create memory BIO.");
		while (0 != (err = ERR_get_error())) {
			log_error("openssl error: %s\n",
			    ERR_error_string(err, NULL));
		}
		goto fail;
	}

	x509 = PEM_read_bio_X509(bio, NULL, NULL, NULL);
	if (x509 == NULL) {
		log_error("%s\n", "Cannot parse certificate from BIO.");
		while (0 != (err = ERR_get_error())) {
			log_error("openssl error: %s\n",
			    ERR_error_string(err, NULL));
		}
		goto fail;
	}

	BIO_free(bio); bio = NULL;

	if (X509_STORE_add_cert(store, x509) == 0) {
		err = ERR_get_error();
		log_error("%s\n", "Cannot store certificate.");
		while (0 != (err = ERR_get_error())) {
			log_error("openssl error: %s\n",
			    ERR_error_string(err, NULL));
		}
		goto fail;
	}

#if defined PRINT_CERTS
	fprintf(stderr, ">>>\n");
	x509_printf(x509, stderr);
	fprintf(stderr, "<<<\n");
#endif /* PRINT_CERTS */

	X509_free(x509); x509 = NULL;

	return 0;

fail:
	if (NULL != bio) {
		BIO_free(bio);
	}
	if (NULL != x509) {
		X509_free(x509);
	}
	return -1;
}


/* ========================================================================= */
/*
 * Read certificate file.
 */
static
int X509_store_add_cert_file(X509_STORE *store, const char *fname,
    int *num_loaded)
/* ========================================================================= */
{
	BIO *bio = NULL;
	X509 *x509 = NULL;
	unsigned long err;
	int read_pems = 0, stored_pems = 0; /* PEM counters. */

	debug_func_call();

	/* TODO -- Does it store all certificates from a single file? */

	assert(NULL != store);
	if (NULL == store) {
		goto fail;
	}
	assert(NULL != fname);
	if (NULL == fname) {
		goto fail;
	}

	bio = BIO_new_file(fname, "r");
	if (NULL == bio) {
		log_error("Cannot open certificate file '%s'.\n", fname);
		goto fail;
	}

	for (;;) {
		x509 = PEM_read_bio_X509(bio, NULL, NULL, NULL);
		if (NULL == x509) {
			if (0 == read_pems) {
				/*
				 * Print error only if no certificate
				 * has been read.
				 */
				log_error("Cannot parse certificate (%d) "
				    "file '%s'.\n", read_pems, fname);
				while (0 != (err = ERR_get_error())) {
					log_error("openssl error: %s\n",
					    ERR_error_string(err, NULL));
				}
			}
			if (0 == stored_pems) {
				goto fail;
			} else {
				break;
			}
		}

		if (X509_STORE_add_cert(store, x509) == 0) {
			err = ERR_get_error();
			log_error(
			    "Cannot store certificate (%d) from file '%s'.\n",
			    read_pems, fname);
			while (0 != (err = ERR_get_error())) {
				log_error("openssl error: %s\n",
				    ERR_error_string(err, NULL));
			}
		} else {
			++stored_pems;
		}

		++read_pems;

#if defined PRINT_CERTS
		fprintf(stderr, ">>>\n");
		x509_printf(x509, stderr);
		fprintf(stderr, "<<<\n");
#endif /* PRINT_CERTS */

		X509_free(x509); x509 = NULL;
	}

	BIO_free(bio); bio = NULL;

	if (NULL != num_loaded) {
		*num_loaded = stored_pems;
	}

	if (0 == stored_pems) {
		goto fail;
	}

	return 0;

fail:
	if (NULL != bio) {
		BIO_free(bio);
	}
	if (NULL != x509) {
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
		log_error("%s\n", "Cannot create memory BIO.");
		while (0 != (err = ERR_get_error())) {
			log_error("openssl error: %s\n",
			    ERR_error_string(err, NULL));
		}
		goto fail;
	}

	cms = d2i_CMS_bio(bio, NULL);
	if (NULL == cms) {
		log_error("%s\n", "Cannot parse CMS.");
		while (0 != (err = ERR_get_error())) {
			log_error("openssl error: %s\n",
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
 * Load X509 from buffer.
 */
static
X509 * load_x509(const void *raw, size_t raw_len)
/* ========================================================================= */
{
	BIO *bio = NULL;
	X509 *x509 = NULL;
	unsigned long err;

	debug_func_call();

	bio = BIO_new_mem_buf((void *) raw, raw_len);
	if (NULL == bio) {
		log_error("%s\n", "Cannot create memory BIO.");
		while (0 != (err = ERR_get_error())) {
			log_error("openssl error: %s\n",
			    ERR_error_string(err, NULL));
		}
		goto fail;
	}

	x509 = d2i_X509_bio(bio, NULL);
	if (NULL == x509) {
		log_error("%s\n", "Cannot parse X509.");
		while (0 != (err = ERR_get_error())) {
			log_error("openssl error: %s\n",
			    ERR_error_string(err, NULL));
		}
		goto fail;
	}

	BIO_free(bio); bio = NULL;

	return x509;

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
int cms_verify_signature(CMS_ContentInfo *cms, X509_STORE *ca_store,
    int crl_check)
/* ========================================================================= */
{
	const ASN1_OBJECT *asn1;
	int nid;
	unsigned long err;
	int verify_flags = 0;

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

	if (NULL == ca_store) {
		verify_flags |= CMS_NO_SIGNER_CERT_VERIFY;
	}

	if (0 == crl_check) {
		verify_flags |= CMS_NOCRL;
	}

	if (!CMS_verify(cms, NULL, ca_store, NULL, NULL,
	        verify_flags)) {
		log_warning("%s\n", "Could not verify CMS.");
		while (0 != (err = ERR_get_error())) {
			log_error("openssl error: %s\n",
			    ERR_error_string(err, NULL));
		}
		return 0;
	}

	return 1;

fail:
	return -1;
}


/* ========================================================================= */
/*
 * Get RFC 3161 time-stamp value.
 */
static
int cms_get_timestamp_value(CMS_ContentInfo *cms, time_t *utc_time)
/* ========================================================================= */
{
	const ASN1_OBJECT *asn1;
	int nid;
	unsigned long err;
	ASN1_OCTET_STRING **pos;
	BIO *bio = NULL;
	TS_TST_INFO *ts_info = NULL;
	const ASN1_GENERALIZEDTIME *gen_time;

	/* ASN1_GENERALIZEDTIME is an alias for ASN1_STRING. */

	debug_func_call();

	assert(NULL != cms);
	assert(NULL != utc_time);
	if (NULL == utc_time) {
		goto fail;
	}

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

	pos = CMS_get0_content((CMS_ContentInfo *) cms);
	if ((NULL == pos) || (NULL == *pos)) {
		assert(0);
		goto fail;
	}

	bio = BIO_new_mem_buf((*pos)->data, (*pos)->length);
	if (NULL == bio) {
		log_error("%s\n", "Cannot create memory bio.");
		while (0 != (err = ERR_get_error())) {
			log_error("openssl error: %s\n",
			    ERR_error_string(err, NULL));
		}
		goto fail;
	}

	ts_info = d2i_TS_TST_INFO_bio(bio, NULL);
	if (NULL == ts_info) {
		log_error("%s\n", "Could not read ts info.");
		while (0 != (err = ERR_get_error())) {
			log_error("openssl error: %s\n",
			    ERR_error_string(err, NULL));
		}
		goto fail;
	}

	BIO_free(bio); bio = NULL;

	gen_time = TS_TST_INFO_get_time(ts_info);
	if (NULL == gen_time) {
		goto fail;
	}

	if (-1 == asn1_time_to_utc_time(gen_time, utc_time)) {
		goto fail;
	}

	TS_TST_INFO_free(ts_info); ts_info = NULL;

	return 0;
fail:
	if (NULL != bio) {
		BIO_free(bio);
	}
	if (NULL != ts_info) {
		TS_TST_INFO_free(ts_info);
	}
	return -1;
}


/* ========================================================================= */
/*
 * Check certificate date.
 */
static
int x509_check_date(const X509 *x509, time_t utc_time)
/* ========================================================================= */
{
	X509_CINF *cinf;
	X509_VAL *val;
	ASN1_TIME *notBefore, *notAfter;
	time_t tNotBef, tNotAft;

	/* ASN1_TIME is an alias for ASN1_STRING. */

	assert(NULL != x509);
	if (NULL == x509) {
		goto fail;
	}
	cinf = x509->cert_info;

	assert(NULL != cinf);
	if (NULL == cinf) {
		goto fail;
	}
	val = cinf->validity;

	notBefore = val->notBefore;
	assert(NULL != notBefore);
	if (NULL == notBefore) {
		goto fail;
	}
	notAfter = val->notAfter;
	assert(NULL != notAfter);
	if (NULL == notAfter) {
		goto fail;
	}
	asn1_time_to_utc_time(notBefore, &tNotBef);
	asn1_time_to_utc_time(notAfter, &tNotAft);

	return (tNotBef <= utc_time) && (utc_time <= tNotAft);

fail:
	return -1;
}


/* ========================================================================= */
/*
 * Converts ASN1_TIME into time_t UTC time.
 */
static
int asn1_time_to_utc_time(const ASN1_TIME *asn1_time, time_t *utc_time)
/* ========================================================================= */
{
	struct tm t;
	const char *str;
	unsigned int i = 0;
	char adjust_op, colon;
	int adj_hour = 0, adj_min = 0;
	int adj_secs;
	time_t ret_time;

//	fprintf(stderr, "TIME %s\n",
//	    ASN1_STRING_data((ASN1_STRING *) asn1_time));

	assert(NULL != asn1_time);
	assert(NULL != utc_time);

	memset(&t, 0, sizeof(t));

	str = (char *) asn1_time->data;
	assert(NULL != str);

	if (V_ASN1_UTCTIME == asn1_time->type) {
		/* two digit year */
		//t.tm_year = (str[i++] - '0') * 10 + (str[i++] - '0');
		t.tm_year =  (str[i++] - '0') * 10;
		t.tm_year += (str[i++] - '0');
		if (t.tm_year < 70) {
			t.tm_year += 100;
		}
	} else if (V_ASN1_GENERALIZEDTIME == asn1_time->type) {
		/* four digit year */
		//t.tm_year = (str[i++] - '0') * 1000 +
		//    (str[++i] - '0') * 100 + (str[++i] - '0') * 10 +
		//    (str[++i] - '0');
		t.tm_year =  (str[i++] - '0') * 1000;
		t.tm_year += (str[i++] - '0') * 100;
		t.tm_year += (str[i++] - '0') * 10;
		t.tm_year += (str[i++] - '0');
		t.tm_year -= 1900;
	}
	/* -1 since January is 0 not 1. */
	//t.tm_mon = ((str[i++] - '0') * 10 + (str[++i] - '0')) - 1;
	t.tm_mon =  (str[i++] - '0') * 10;
	t.tm_mon += (str[i++] - '0');
	t.tm_mon -= 1;
	//t.tm_mday = (str[i++] - '0') * 10 + (str[++i] - '0');
	t.tm_mday =  (str[i++] - '0') * 10;
	t.tm_mday += (str[i++] - '0');
	//t.tm_hour = (str[i++] - '0') * 10 + (str[++i] - '0');
	t.tm_hour =  (str[i++] - '0') * 10;
	t.tm_hour += (str[i++] - '0');
	//t.tm_min = (str[i++] - '0') * 10 + (str[++i] - '0');
	t.tm_min =  (str[i++] - '0') * 10;
	t.tm_min += (str[i++] - '0');
	//t.tm_sec = (str[i++] - '0') * 10 + (str[++i] - '0');
	t.tm_sec =  (str[i++] - '0') * 10;
	t.tm_sec += (str[i++] - '0');

	/* Ignore fractional part of time. */
	if ('.' == str[i]) {
		i++;
		while (isdigit(str[i])) {
			i++;
		}
	}

	adjust_op = str[i++];
	switch (adjust_op) {
	case 'z':
	case 'Z':
		*utc_time = timegm_utc(&t);
//		fprintf(stderr, "UTC %lu\n", *utc_time);
		return 0;
		break;
	case '-':
	case '+':
		/* Continue. */
		break;
	default:
		assert(0);
		return -1;
		break;
	}

	/* Adjust the time based on time-zone information. */

	assert(('+' == adjust_op) || ('-' == adjust_op));
	if (('+' != adjust_op) && ('-' != adjust_op)) {
		return -1;
	}

	adj_hour =  (str[i++] - '0') * 10;
	adj_hour += (str[i++] - '0');

	colon = str[i++];
	assert(':' == colon);
	if (':' != colon) {
		return -1;
	}

	adj_min =  (str[i++] - '0') * 10;
	adj_min += (str[i++] - '0');

	adj_secs = (adj_hour * 3600) + (adj_min * 60);

	ret_time = timegm_utc(&t);

	if ('+' == adjust_op) {
		*utc_time = ret_time - adj_secs;
	} else {
		*utc_time = ret_time + adj_secs;
	}

	return 0;
}


/* ========================================================================= */
/*
 * Portable version of timegm().
 */
static
time_t timegm_utc(struct tm *tm)
/* ========================================================================= */
{
#ifndef WIN32
	/* Code taken from man timegm(3). */

	time_t ret;
	char *tz;

	tz = getenv("TZ");
	if (NULL != tz) {
		tz = strdup(tz);
	}
	setenv("TZ", "", 1);
	tzset();
	ret = mktime(tm);
	if (NULL != tz) {
		setenv("TZ", tz, 1);
		free(tz);
	} else {
		unsetenv("TZ");
	}
	tzset();

	return ret;
#else /* WIN32 */
	return timegm_win(tm);
#endif /* !WIN32 */
}


/* ========================================================================= */
/*
 * Converts X509 certificate to DER.
 */
static
int x509_get_pem(X509 *x509, void **der, size_t *der_len)
/* ========================================================================= */
{
	BIO *bio = NULL;
	BUF_MEM *bptr = NULL;

	assert(NULL != x509);
	if (NULL == x509) {
		goto fail;
	}
	assert(NULL != der);
	if (NULL == der) {
		goto fail;
	}
	assert(NULL != der_len);
	if (NULL == der_len) {
		goto fail;
	}

	bio = BIO_new(BIO_s_mem());

	if (!i2d_X509_bio(bio, x509)) {
		log_error("%s\n", "Could not write X509 to bio.");
		goto fail;
	}

	BIO_get_mem_ptr(bio, &bptr);
	if (!BIO_set_close(bio, BIO_NOCLOSE)) {
		goto fail;
	}
	BIO_free(bio); bio = NULL;

	/* Extract DER from buffer. */
	*der = bptr->data; bptr->data = NULL;
	*der_len = bptr->length; bptr->length = 0;
	bptr->max = 0;

	BUF_MEM_free(bptr); bptr = NULL;

	return 0;

fail:
	if (NULL != bio) {
		BIO_free(bio);
	}
	if (NULL != bptr) {
		BUF_MEM_free(bptr);
	}
	return -1;
}


#if defined PRINT_CERTS
/* ========================================================================= */
/*
 * Prints x509.
 */
static
int x509_printf(X509 *x509, FILE *fout)
/* ========================================================================= */
{
	BIO *wbio = NULL;
	BUF_MEM *buf_mem;

	assert(NULL != x509);
	assert(NULL != fout);

	wbio = BIO_new(BIO_s_mem());
	if (NULL == wbio) {
		goto fail;
	}

	X509_print(wbio, x509);
	//i2d_X509_bio(wbio, x509);
	BIO_get_mem_ptr(wbio, &buf_mem);

	fwrite(buf_mem->data, buf_mem->length, 1, fout);

	BIO_free(wbio); wbio = NULL;

	return 0;
fail:
	if (NULL != wbio) {
		BIO_free(wbio);
	}
	return -1;
}
#endif /* PRINT_CERTS */


const char postsignum_qca_root_file[] = "postsignum_qca_root.pem";
const char postsignum_qca_root_name[] = "PostSignum Root QCA";
const char postsignum_qca_root_pem[] =
"-----BEGIN CERTIFICATE-----""\n"
"MIIGKjCCBRKgAwIBAgIBATANBgkqhkiG9w0BAQUFADBZMQswCQYDVQQGEwJDWjEs""\n"
"MCoGA1UECgwjxIxlc2vDoSBwb8WhdGEsIHMucC4gW0nEjCA0NzExNDk4M10xHDAa""\n"
"BgNVBAMTE1Bvc3RTaWdudW0gUm9vdCBRQ0EwHhcNMDUwNDA2MDk0NTExWhcNMzAw""\n"
"NDA2MDk0MjI3WjBZMQswCQYDVQQGEwJDWjEsMCoGA1UECgwjxIxlc2vDoSBwb8Wh""\n"
"dGEsIHMucC4gW0nEjCA0NzExNDk4M10xHDAaBgNVBAMTE1Bvc3RTaWdudW0gUm9v""\n"
"dCBRQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC4OrLAx+0mAPpc""\n"
"fvUNrOic7u6DJcokEJLoWSv0ZurD5pXVZG+zN9pKX5P3iH7DZtuZ2qwzg4tHReCe""\n"
"u6SR+aAn962eG2ZEw1uv411QrZUgVkOe8Tvfr0Cv1HOzgZn0AFZNZ8TnHS67SMP/""\n"
"z//VyFLqSBm44QtJDeiAvzwLXFAp5HYeIBMXVMfp1aY2t8RN7B0WSgO8aU1UgRvi""\n"
"KR4qCJao0iCuQV/4f0Exf1o4AyjXlTZ4wbKD5ZAwuI8a+aZKjtIW1Ucioa/0kyLx""\n"
"3DHLq0Lsll5OaVP2awfPkxXGyPOSYrEXxoNm32CfKeXjY1xTIwm0cIx5AEpNP8t7""\n"
"Ku5hPwY7AgMBAAGjggL7MIIC9zCCAYsGA1UdHwSCAYIwggF+MDCgLqAshipodHRw""\n"
"Oi8vd3d3LnBvc3RzaWdudW0uY3ovY3JsL3Bzcm9vdHFjYS5jcmwwMKAuoCyGKmh0""\n"
"dHA6Ly9wb3N0c2lnbnVtLnR0Yy5jei9jcmwvcHNyb290cWNhLmNybDCBiqCBh6CB""\n"
"hIaBgWxkYXA6Ly9xY2EucG9zdHNpZ251bS5jei9jbiUzZFBvc3RTaWdudW0lMjBS""\n"
"b290JTIwUUNBLG8lM2RDZXNrYSUyMHBvc3RhJTIwcy5wLiUyMFtJQyUyMDQ3MTE0""\n"
"OTgzXSxjJTNkQ1o/Y2VydGlmaWNhdGVSZXZvY2F0aW9uTGlzdDCBiqCBh6CBhIaB""\n"
"gWxkYXA6Ly9wb3N0c2lnbnVtLnR0Yy5jei9jbiUzZFBvc3RTaWdudW0lMjBSb290""\n"
"JTIwUUNBLG8lM2RDZXNrYSUyMHBvc3RhJTIwcy5wLiUyMFtJQyUyMDQ3MTE0OTgz""\n"
"XSxjJTNkQ1o/Y2VydGlmaWNhdGVSZXZvY2F0aW9uTGlzdDCBoQYDVR0gBIGZMIGW""\n"
"MIGTBgRVHSAAMIGKMIGHBggrBgEFBQcCAjB7GnlUZW50byBjZXJ0aWZpa2F0IGJ5""\n"
"bCB2eWRhbiBqYWtvIGt2YWxpZmlrb3Zhbnkgc3lzdGVtb3Z5IGNlcnRpZmlrYXQg""\n"
"dmUgc215c2x1IHpha29uYSAyMjcvMjAwMCBTYi4gYSBuYXZhenVqaWNpY2ggcHJl""\n"
"ZHBpc3UuMA8GA1UdEwQIMAYBAf8CAQEwDgYDVR0PAQH/BAQDAgEGMB0GA1UdDgQW""\n"
"BBQrHdGdefXVeB4CPIJK6N3uQ68pRDCBgQYDVR0jBHoweIAUKx3RnXn11XgeAjyC""\n"
"Sujd7kOvKUShXaRbMFkxCzAJBgNVBAYTAkNaMSwwKgYDVQQKDCPEjGVza8OhIHBv""\n"
"xaF0YSwgcy5wLiBbScSMIDQ3MTE0OTgzXTEcMBoGA1UEAxMTUG9zdFNpZ251bSBS""\n"
"b290IFFDQYIBATANBgkqhkiG9w0BAQUFAAOCAQEAsWkApNYzof7ZKmroU3aDOnR/""\n"
"2ObgD0SnE3N+/KYYSGCzLf4HQtGspMjUEDMULUqAWQF76ZbPRbv6FSVyk5YuAxkI""\n"
"DvNknsfTxz6mCnGNsL/qgTYaK2TLk8A1b6VEXMD0MjOXODm5ooa+sSNxzT3JBbTC""\n"
"AJbtJ6OrDmqVE9X+88M39L1z7YTHPaTt1i7HGrKfYf42TWp0oGD+o0lJQoqAwHOj""\n"
"ASVmDEs4iUUi6y3jboBJtZSoUDkzK5mRlJELWtdvANTpcrf/DLj7CbG9wKYIUH0D""\n"
"KQuvApdC79JbGojTzZiMOVBH9H+v/8suZgFdQqBwF82mwSZwxHmn149grQLkJg==""\n"
"-----END CERTIFICATE-----";

const char postsignum_qca_sub_file[] = "postsignum_qca_sub.pem";
const char postsignum_qca_sub_name[] = "PostSignum Qualified CA";
const char postsignum_qca_sub_pem[] =
"-----BEGIN CERTIFICATE-----""\n"
"MIIGLjCCBRagAwIBAgIBHDANBgkqhkiG9w0BAQUFADBZMQswCQYDVQQGEwJDWjEs""\n"
"MCoGA1UECgwjxIxlc2vDoSBwb8WhdGEsIHMucC4gW0nEjCA0NzExNDk4M10xHDAa""\n"
"BgNVBAMTE1Bvc3RTaWdudW0gUm9vdCBRQ0EwHhcNMDUwNDA3MDgzMzE3WhcNMjAw""\n"
"NDA3MDgzMjI2WjBdMQswCQYDVQQGEwJDWjEsMCoGA1UECgwjxIxlc2vDoSBwb8Wh""\n"
"dGEsIHMucC4gW0nEjCA0NzExNDk4M10xIDAeBgNVBAMTF1Bvc3RTaWdudW0gUXVh""\n"
"bGlmaWVkIENBMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAsWvwIoOT""\n"
"m8tAM8GOrDPt7532/qvT4rh7iDWPKP7PIHNnvIY7l1HJIenOWA0qPqtccu1VZxtg""\n"
"+PAIrSw2CbopaWOPiDG8W5WFZbxI0xPAHr1mPvla4rSW84Gk0iptmHtfMHBVTJxR""\n"
"tBAd6+WFThuELG8qoN1hqu/9Q77mb891WKlR5q1GBsbf0+WXuS3N5LU0sMPzSszY""\n"
"FpyX1wJO331Cubda3JFU69Jjnz+5l3bcKliZhcDhEhdDQFTDglwc5MB/hxR7J4pz""\n"
"PgvIlmUipQlHefVmvCdwnu3tm/oLLDpLUT773EFiBB0j5MoQPiP5DSzziWvGuJf5""\n"
"8VlFbY/QveyFKwIDAQABo4IC+zCCAvcwgaEGA1UdIASBmTCBljCBkwYEVR0gADCB""\n"
"ijCBhwYIKwYBBQUHAgIwexp5VGVudG8gY2VydGlmaWthdCBieWwgdnlkYW4gamFr""\n"
"byBrdmFsaWZpa292YW55IHN5c3RlbW92eSBjZXJ0aWZpa2F0IHZlIHNteXNsdSB6""\n"
"YWtvbmEgMjI3LzIwMDAgU2IuIGEgbmF2YXp1amljaWNoIHByZWRwaXN1LjAPBgNV""\n"
"HRMECDAGAQH/AgEAMA4GA1UdDwEB/wQEAwIBBjCBgQYDVR0jBHoweIAUKx3RnXn1""\n"
"1XgeAjyCSujd7kOvKUShXaRbMFkxCzAJBgNVBAYTAkNaMSwwKgYDVQQKDCPEjGVz""\n"
"a8OhIHBvxaF0YSwgcy5wLiBbScSMIDQ3MTE0OTgzXTEcMBoGA1UEAxMTUG9zdFNp""\n"
"Z251bSBSb290IFFDQYIBATCCAYsGA1UdHwSCAYIwggF+MDCgLqAshipodHRwOi8v""\n"
"d3d3LnBvc3RzaWdudW0uY3ovY3JsL3Bzcm9vdHFjYS5jcmwwMKAuoCyGKmh0dHA6""\n"
"Ly9wb3N0c2lnbnVtLnR0Yy5jei9jcmwvcHNyb290cWNhLmNybDCBiqCBh6CBhIaB""\n"
"gWxkYXA6Ly9xY2EucG9zdHNpZ251bS5jei9jbiUzZFBvc3RTaWdudW0lMjBSb290""\n"
"JTIwUUNBLG8lM2RDZXNrYSUyMHBvc3RhJTIwcy5wLiUyMFtJQyUyMDQ3MTE0OTgz""\n"
"XSxjJTNkQ1o/Y2VydGlmaWNhdGVSZXZvY2F0aW9uTGlzdDCBiqCBh6CBhIaBgWxk""\n"
"YXA6Ly9wb3N0c2lnbnVtLnR0Yy5jei9jbiUzZFBvc3RTaWdudW0lMjBSb290JTIw""\n"
"UUNBLG8lM2RDZXNrYSUyMHBvc3RhJTIwcy5wLiUyMFtJQyUyMDQ3MTE0OTgzXSxj""\n"
"JTNkQ1o/Y2VydGlmaWNhdGVSZXZvY2F0aW9uTGlzdDAdBgNVHQ4EFgQUp5+2jomT""\n"
"mmV2CZqV+ER+aYJq3gswDQYJKoZIhvcNAQEFBQADggEBACkRE++TGTBboYXi1Skh""\n"
"lR66Y9Eo4xvJctW4Pao6VCZJlU5M3cHy2dM1Du4OvHwlKHvcP/w66xo++ves30sC""\n"
"rg7OQaqLR8KfJsX4wkvBKYC6D2K7aZqUAfCVABcWZkVdr9fwMp+59Yl39UyCxRlP""\n"
"vH1unHylO/ibSxH9Lsl+N0ioFugmW50vYEFP4vy6blRPMW5Akwa+SP00vV2YejRn""\n"
"5I6RHxV/nq9A0gGxBZq4U4sSbg+oLs0szBqTWt4EEYLMleexttp+7H1eOX3spsn3""\n"
"WodbhcSWXDyVjR29Ezbhs5Lo6aAQSl5ZL38h8L1AiCSUFG/SmwjJmnpCGDrRwEsZ""\n"
"Xr0=""\n"
"-----END CERTIFICATE-----";

const char postsignum_qca2_root_file[] = "postsignum_qca2_root.pem";
const char postsignum_qca2_root_name[] = "PostSignum Root QCA 2";
const char postsignum_qca2_root_pem[] =
"-----BEGIN CERTIFICATE-----""\n"
"MIIFnDCCBISgAwIBAgIBZDANBgkqhkiG9w0BAQsFADBbMQswCQYDVQQGEwJDWjEs""\n"
"MCoGA1UECgwjxIxlc2vDoSBwb8WhdGEsIHMucC4gW0nEjCA0NzExNDk4M10xHjAc""\n"
"BgNVBAMTFVBvc3RTaWdudW0gUm9vdCBRQ0EgMjAeFw0xMDAxMTkwODA0MzFaFw0y""\n"
"NTAxMTkwODA0MzFaMFsxCzAJBgNVBAYTAkNaMSwwKgYDVQQKDCPEjGVza8OhIHBv""\n"
"xaF0YSwgcy5wLiBbScSMIDQ3MTE0OTgzXTEeMBwGA1UEAxMVUG9zdFNpZ251bSBS""\n"
"b290IFFDQSAyMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAoFz8yBxf""\n"
"2gf1uN0GGXknvGHwurpp4Lw3ZPWZB6nEBDGjSGIXK0Or6Xa3ZT+tVDTeUUjT133G""\n"
"7Vs51D6z/ShWy+9T7a1f6XInakewyFj8PT0EdZ4tAybNYdEUO/dShg2WvUyfZfXH""\n"
"0jmmZm6qUDy0VfKQfiyWchQRi/Ax6zXaU2+X3hXBfvRMr5l6zgxYVATEyxCfOLM9""\n"
"a5U6lhpyCDf2Gg6dPc5Cy6QwYGGpYER1fzLGsN9stdutkwlP13DHU1Sp6W5ywtfL""\n"
"owYaV1bqOOdARbAoJ7q8LO6EBjyIVr03mFusPaMCOzcEn3zL5XafknM36Vqtdmqz""\n"
"iWR+3URAUgqE0wIDAQABo4ICaTCCAmUwgaUGA1UdHwSBnTCBmjAxoC+gLYYraHR0""\n"
"cDovL3d3dy5wb3N0c2lnbnVtLmN6L2NybC9wc3Jvb3RxY2EyLmNybDAyoDCgLoYs""\n"
"aHR0cDovL3d3dzIucG9zdHNpZ251bS5jei9jcmwvcHNyb290cWNhMi5jcmwwMaAv""\n"
"oC2GK2h0dHA6Ly9wb3N0c2lnbnVtLnR0Yy5jei9jcmwvcHNyb290cWNhMi5jcmww""\n"
"gfEGA1UdIASB6TCB5jCB4wYEVR0gADCB2jCB1wYIKwYBBQUHAgIwgcoagcdUZW50""\n"
"byBrdmFsaWZpa292YW55IHN5c3RlbW92eSBjZXJ0aWZpa2F0IGJ5bCB2eWRhbiBw""\n"
"b2RsZSB6YWtvbmEgMjI3LzIwMDBTYi4gYSBuYXZhem55Y2ggcHJlZHBpc3UvVGhp""\n"
"cyBxdWFsaWZpZWQgc3lzdGVtIGNlcnRpZmljYXRlIHdhcyBpc3N1ZWQgYWNjb3Jk""\n"
"aW5nIHRvIExhdyBObyAyMjcvMjAwMENvbGwuIGFuZCByZWxhdGVkIHJlZ3VsYXRp""\n"
"b25zMBIGA1UdEwEB/wQIMAYBAf8CAQEwDgYDVR0PAQH/BAQDAgEGMB0GA1UdDgQW""\n"
"BBQVKYzFRWmruLPD6v5LuDHY3PDndjCBgwYDVR0jBHwweoAUFSmMxUVpq7izw+r+""\n"
"S7gx2Nzw53ahX6RdMFsxCzAJBgNVBAYTAkNaMSwwKgYDVQQKDCPEjGVza8OhIHBv""\n"
"xaF0YSwgcy5wLiBbScSMIDQ3MTE0OTgzXTEeMBwGA1UEAxMVUG9zdFNpZ251bSBS""\n"
"b290IFFDQSAyggFkMA0GCSqGSIb3DQEBCwUAA4IBAQBeKtoLQKFqWJEgLNxPbQNN""\n"
"5OTjbpOTEEkq2jFI0tUhtRx//6zwuqJCzfO/KqggUrHBca+GV/qXcNzNAlytyM71""\n"
"fMv/VwgL9gBHTN/IFIw100JbciI23yFQTdF/UoEfK/m+IFfirxSRi8LRERdXHTEb""\n"
"vwxMXIzZVXloWvX64UwWtf4Tvw5bAoPj0O1Z2ly4aMTAT2a+y+z184UhuZ/oGyMw""\n"
"eIakmFM7M7RrNki507jiSLTzuaFMCpyWOX7ULIhzY6xKdm5iQLjTvExn2JTvVChF""\n"
"Y+jUu/G0zAdLyeU4vaXdQm1A8AEiJPTd0Z9LAxL6Sq2iraLNN36+NyEK/ts3mPLL""\n"
"-----END CERTIFICATE-----";

const char postsignum_qca2_sub_file[] = "postsignum_qca2_sub.pem";
const char postsignum_qca2_sub_name[] = "PostSignum Qualified CA 2";
const char postsignum_qca2_sub_pem[] =
"-----BEGIN CERTIFICATE-----""\n"
"MIIGXzCCBUegAwIBAgIBcTANBgkqhkiG9w0BAQsFADBbMQswCQYDVQQGEwJDWjEs""\n"
"MCoGA1UECgwjxIxlc2vDoSBwb8WhdGEsIHMucC4gW0nEjCA0NzExNDk4M10xHjAc""\n"
"BgNVBAMTFVBvc3RTaWdudW0gUm9vdCBRQ0EgMjAeFw0xMDAxMTkxMTMxMjBaFw0y""\n"
"MDAxMTkxMTMwMjBaMF8xCzAJBgNVBAYTAkNaMSwwKgYDVQQKDCPEjGVza8OhIHBv""\n"
"xaF0YSwgcy5wLiBbScSMIDQ3MTE0OTgzXTEiMCAGA1UEAxMZUG9zdFNpZ251bSBR""\n"
"dWFsaWZpZWQgQ0EgMjCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKbR""\n"
"ReVFlmMooQD/ZzJA9M793LcZivHRvWEG8jsEpp2xTayR17ovs8OMeoYKjvGo6PDf""\n"
"kCJs+sBYS0q5WQFApdWkyl/tUOw1oZ2SPSq6uYLJUyOYSKPMOgKz4u3XuB4Ki1Z+""\n"
"i8Fb7zeRye6eqahK+tql3ZAJnrJKgC4X2Ta1RKkxK+Hu1bdhWJA3gwL+WkIZbL/P""\n"
"YIzjet++T8ssWK1PWdBXsSfKOTikNzZt2VPETAQDBpOYxqAgLfCRbcb9KU2WIMT3""\n"
"NNxILu3sNl+OM9gV/GWO943JHsOMAVyJSQREaZksG5KDzzNzQS/LsbYkFtnJAmmh""\n"
"7g9p9Ci6cEJ+pfBTtMECAwEAAaOCAygwggMkMIHxBgNVHSAEgekwgeYwgeMGBFUd""\n"
"IAAwgdowgdcGCCsGAQUFBwICMIHKGoHHVGVudG8ga3ZhbGlmaWtvdmFueSBzeXN0""\n"
"ZW1vdnkgY2VydGlmaWthdCBieWwgdnlkYW4gcG9kbGUgemFrb25hIDIyNy8yMDAw""\n"
"U2IuIGEgbmF2YXpueWNoIHByZWRwaXN1L1RoaXMgcXVhbGlmaWVkIHN5c3RlbSBj""\n"
"ZXJ0aWZpY2F0ZSB3YXMgaXNzdWVkIGFjY29yZGluZyB0byBMYXcgTm8gMjI3LzIw""\n"
"MDBDb2xsLiBhbmQgcmVsYXRlZCByZWd1bGF0aW9uczASBgNVHRMBAf8ECDAGAQH/""\n"
"AgEAMIG8BggrBgEFBQcBAQSBrzCBrDA3BggrBgEFBQcwAoYraHR0cDovL3d3dy5w""\n"
"b3N0c2lnbnVtLmN6L2NydC9wc3Jvb3RxY2EyLmNydDA4BggrBgEFBQcwAoYsaHR0""\n"
"cDovL3d3dzIucG9zdHNpZ251bS5jei9jcnQvcHNyb290cWNhMi5jcnQwNwYIKwYB""\n"
"BQUHMAKGK2h0dHA6Ly9wb3N0c2lnbnVtLnR0Yy5jei9jcnQvcHNyb290cWNhMi5j""\n"
"cnQwDgYDVR0PAQH/BAQDAgEGMIGDBgNVHSMEfDB6gBQVKYzFRWmruLPD6v5LuDHY""\n"
"3PDndqFfpF0wWzELMAkGA1UEBhMCQ1oxLDAqBgNVBAoMI8SMZXNrw6EgcG/FoXRh""\n"
"LCBzLnAuIFtJxIwgNDcxMTQ5ODNdMR4wHAYDVQQDExVQb3N0U2lnbnVtIFJvb3Qg""\n"
"UUNBIDKCAWQwgaUGA1UdHwSBnTCBmjAxoC+gLYYraHR0cDovL3d3dy5wb3N0c2ln""\n"
"bnVtLmN6L2NybC9wc3Jvb3RxY2EyLmNybDAyoDCgLoYsaHR0cDovL3d3dzIucG9z""\n"
"dHNpZ251bS5jei9jcmwvcHNyb290cWNhMi5jcmwwMaAvoC2GK2h0dHA6Ly9wb3N0""\n"
"c2lnbnVtLnR0Yy5jei9jcmwvcHNyb290cWNhMi5jcmwwHQYDVR0OBBYEFInoTN+L""\n"
"Jjk+1yQuEg565+Yn5daXMA0GCSqGSIb3DQEBCwUAA4IBAQB17M2VB48AXCVfVeeO""\n"
"Lo0LIJZcg5EyHUKurbnff6tQOmyT7gzpkJNY3I3ijW2ErBfUM/6HefMxYKKWSs4j""\n"
"XqGSK5QfxG0B0O3uGfHPS4WFftaPSAnWk1tiJZ4c43+zSJCcH33n9pDmvt8n0j+6""\n"
"cQAZIWh4PPpmkvUg3uN4E0bzZHnH2uKzMvpVnE6wKml6oV+PUfPASPIYQw9gFEAN""\n"
"cMzp10hXJHrnOo0alPklymZdTVssBXwdzhSBsFel1eVBSvVOx6+y8zdbrkRLOvTV""\n"
"nSMb6zH+fsygU40mimdo30rY/6N+tdQhbM/sTCxgdWAy2g0elAN1zi9Jx6aQ76wo""\n"
"Dcn+""\n"
"-----END CERTIFICATE-----";

const char postsignum_qca3_sub_file[] = "postsignum_qca3_sub.pem";
const char postsignum_qca3_sub_name[] = "PostSignum Qualified CA 3";
const char postsignum_qca3_sub_pem[] =
"-----BEGIN CERTIFICATE-----""\n"
"MIIGYDCCBUigAwIBAgICAKQwDQYJKoZIhvcNAQELBQAwWzELMAkGA1UEBhMCQ1ox""\n"
"LDAqBgNVBAoMI8SMZXNrw6EgcG/FoXRhLCBzLnAuIFtJxIwgNDcxMTQ5ODNdMR4w""\n"
"HAYDVQQDExVQb3N0U2lnbnVtIFJvb3QgUUNBIDIwHhcNMTQwMzI2MDgwMTMyWhcN""\n"
"MjQwMzI2MDcwMDM2WjBfMQswCQYDVQQGEwJDWjEsMCoGA1UECgwjxIxlc2vDoSBw""\n"
"b8WhdGEsIHMucC4gW0nEjCA0NzExNDk4M10xIjAgBgNVBAMTGVBvc3RTaWdudW0g""\n"
"UXVhbGlmaWVkIENBIDMwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCX""\n"
"Ou7d2frODVCZuo7IEWxoF5f1KE9aelb8FUyoZCL6iyvBe7YaL1pH4FJ5DPFbf3mz""\n"
"6rLnSiDY/YSpipstdNUHM2BZkhiEulb7ltvMC+v4gf+H9ApVkmNspEWcO8+Thj4b""\n"
"m0anXJ8oFKRCkPQYAPQQyRq0erqlXTkXS4NePI0TU4mvtaokZCqBBqzP6GnXOvZA""\n"
"zxo/KkK7nvgEwibZEXnrI3ZN20dzmvT/m+igHsPfBuTJsRXO1ytqxD+xz8L9eoAX""\n"
"yOWbQTLJI9FXE3utZ9fr0mhEUc0xcaQfVwdGahJ6/ex1asZH7XFD2VyHaTSqXomD""\n"
"iyo71Zp0EnGjdLACkUtdAgMBAAGjggMoMIIDJDCB8QYDVR0gBIHpMIHmMIHjBgRV""\n"
"HSAAMIHaMIHXBggrBgEFBQcCAjCByhqBx1RlbnRvIGt2YWxpZmlrb3Zhbnkgc3lz""\n"
"dGVtb3Z5IGNlcnRpZmlrYXQgYnlsIHZ5ZGFuIHBvZGxlIHpha29uYSAyMjcvMjAw""\n"
"MFNiLiBhIG5hdmF6bnljaCBwcmVkcGlzdS9UaGlzIHF1YWxpZmllZCBzeXN0ZW0g""\n"
"Y2VydGlmaWNhdGUgd2FzIGlzc3VlZCBhY2NvcmRpbmcgdG8gTGF3IE5vIDIyNy8y""\n"
"MDAwQ29sbC4gYW5kIHJlbGF0ZWQgcmVndWxhdGlvbnMwEgYDVR0TAQH/BAgwBgEB""\n"
"/wIBADCBvAYIKwYBBQUHAQEEga8wgawwNwYIKwYBBQUHMAKGK2h0dHA6Ly93d3cu""\n"
"cG9zdHNpZ251bS5jei9jcnQvcHNyb290cWNhMi5jcnQwOAYIKwYBBQUHMAKGLGh0""\n"
"dHA6Ly93d3cyLnBvc3RzaWdudW0uY3ovY3J0L3Bzcm9vdHFjYTIuY3J0MDcGCCsG""\n"
"AQUFBzAChitodHRwOi8vcG9zdHNpZ251bS50dGMuY3ovY3J0L3Bzcm9vdHFjYTIu""\n"
"Y3J0MA4GA1UdDwEB/wQEAwIBBjCBgwYDVR0jBHwweoAUFSmMxUVpq7izw+r+S7gx""\n"
"2Nzw53ahX6RdMFsxCzAJBgNVBAYTAkNaMSwwKgYDVQQKDCPEjGVza8OhIHBvxaF0""\n"
"YSwgcy5wLiBbScSMIDQ3MTE0OTgzXTEeMBwGA1UEAxMVUG9zdFNpZ251bSBSb290""\n"
"IFFDQSAyggFkMIGlBgNVHR8EgZ0wgZowMaAvoC2GK2h0dHA6Ly93d3cucG9zdHNp""\n"
"Z251bS5jei9jcmwvcHNyb290cWNhMi5jcmwwMqAwoC6GLGh0dHA6Ly93d3cyLnBv""\n"
"c3RzaWdudW0uY3ovY3JsL3Bzcm9vdHFjYTIuY3JsMDGgL6AthitodHRwOi8vcG9z""\n"
"dHNpZ251bS50dGMuY3ovY3JsL3Bzcm9vdHFjYTIuY3JsMB0GA1UdDgQWBBTy+Mwq""\n"
"V2HaKxczWeWCLewGHIpPSjANBgkqhkiG9w0BAQsFAAOCAQEAVHG9oYU7dATQI/yV""\n"
"gwhboNVX9Iat8Ji6PvVnoM6TQ8WjUQ5nErZG1fV5QQgN7slMBWnXKNjUSxMDpfht""\n"
"N2RbJHniaw/+vDqKtlmoKAnmIRzRaIqBLwGZs6RGHFrMPiol3La55fBoa4JPliRT""\n"
"Fw5xVOK5FdJh/5Pbfg+XNZ0RzO0/tk/oKRXfgRNb9ZBL2pe8sr9g9QywpsGKt2gP""\n"
"9t0q/+dhKAGc0+eimChM8Bmq4WNUxK4qdo4ARH6344uIVlIu+9Gq3H54noyZd/Oh""\n"
"RTnuoXuQOdx9DooTp6SPpPfZXj/djsseT22QVpYBP7v8AVK/paqphINL2XmQdiw6""\n"
"5KhDYA==""\n"
"-----END CERTIFICATE-----";

const char equifax_ca_file[] = "equifax_ca.pem";
const char equifax_ca_name[] = "Equifax Secure Certificate Authority";
const char equifax_ca_pem[] =
"-----BEGIN CERTIFICATE-----""\n"
"MIIDIDCCAomgAwIBAgIENd70zzANBgkqhkiG9w0BAQUFADBOMQswCQYDVQQGEwJV""\n"
"UzEQMA4GA1UEChMHRXF1aWZheDEtMCsGA1UECxMkRXF1aWZheCBTZWN1cmUgQ2Vy""\n"
"dGlmaWNhdGUgQXV0aG9yaXR5MB4XDTk4MDgyMjE2NDE1MVoXDTE4MDgyMjE2NDE1""\n"
"MVowTjELMAkGA1UEBhMCVVMxEDAOBgNVBAoTB0VxdWlmYXgxLTArBgNVBAsTJEVx""\n"
"dWlmYXggU2VjdXJlIENlcnRpZmljYXRlIEF1dGhvcml0eTCBnzANBgkqhkiG9w0B""\n"
"AQEFAAOBjQAwgYkCgYEAwV2xWGcIYu6gmi0fCG2RFGiYCh7+2gRvE4RiIcPRfM6f""\n"
"BeC4AfBONOziipUEZKzxa1NfBbPLZ4C/QgKO/t0BCezhABRP/PvwDN1Dulsr4R+A""\n"
"cJkVV5MW8Q+XarfCaCMczE1ZMKxRHjuvK9buY0V7xdlfUNLjUA86iOe/FP3gx7kC""\n"
"AwEAAaOCAQkwggEFMHAGA1UdHwRpMGcwZaBjoGGkXzBdMQswCQYDVQQGEwJVUzEQ""\n"
"MA4GA1UEChMHRXF1aWZheDEtMCsGA1UECxMkRXF1aWZheCBTZWN1cmUgQ2VydGlm""\n"
"aWNhdGUgQXV0aG9yaXR5MQ0wCwYDVQQDEwRDUkwxMBoGA1UdEAQTMBGBDzIwMTgw""\n"
"ODIyMTY0MTUxWjALBgNVHQ8EBAMCAQYwHwYDVR0jBBgwFoAUSOZo+SvSspXXR9gj""\n"
"IBBPM5iQn9QwHQYDVR0OBBYEFEjmaPkr0rKV10fYIyAQTzOYkJ/UMAwGA1UdEwQF""\n"
"MAMBAf8wGgYJKoZIhvZ9B0EABA0wCxsFVjMuMGMDAgbAMA0GCSqGSIb3DQEBBQUA""\n"
"A4GBAFjOKer89961zgK5F7WF0bnj4JXMJTENAKaSbn+2kmOeUJXRmm/kEd5jhW6Y""\n"
"7qj/WsjTVbJmcVfewCHrPSqnI0kBBIZCe/zuf6IWUrVnZ9NA2zsmWLIodz2uFHdh""\n"
"1voqZiegDfqnc1zqcPGUIWVEX/r87yloqaKHee9570+sB3c4""\n"
"-----END CERTIFICATE-----";

const char all_certs_file[] = "all_trusted.pem";


/*!
 * @brief Holds NULL-terminated list of PEM encoded certificate files.
 *
 * @note In C file names may be string literals or 'cost char str[]'.
 * C++ allows 'const char * str'.
 */
const char *pem_files[] = {
	NULL, /* Don't use this list. */
	postsignum_qca_root_file,
	postsignum_qca_sub_file,
	postsignum_qca2_root_file,
	postsignum_qca2_sub_file,
	postsignum_qca3_sub_file,
//	equifax_ca_file,
//	NULL,
//	all_certs_file,
	NULL
};

/*
 * Holds NULL-terminated list of PEM encoded certificates.
 */
const struct pem_str pem_strs[] = {
	{postsignum_qca_root_name, postsignum_qca_root_pem},
	{postsignum_qca_sub_name, postsignum_qca_sub_pem},
	{postsignum_qca2_root_name, postsignum_qca2_root_pem},
	{postsignum_qca2_sub_name, postsignum_qca2_sub_pem},
	{postsignum_qca3_sub_name, postsignum_qca3_sub_pem},
//	{equifax_ca_name, equifax_ca_pem},
	{NULL, NULL}
};
