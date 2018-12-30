/*
 * Copyright (C) 2014-2018 CZ.NIC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations including
 * the two.
 */

#include <assert.h>
#include <ctype.h> /* isdigit(3) */
#include <openssl/bio.h>
#include "cms/cms.h"
#include <openssl/err.h>
#include <openssl/evp.h> /* OpenSSL_add_all_algorithms() */
#include <openssl/pem.h>
#include <openssl/ts.h>
#include <openssl/x509_vfy.h>
#include <openssl/x509v3.h>
#include <openssl/pkcs12.h>
#include <string.h>

#include "src/compat/compat_win.h"
#include "src/crypto/crypto.h"
#include "src/crypto/crypto_funcs.h"
#include "src/datovka_shared/crypto/crypto_trusted_certs.h"
#include "src/datovka_shared/log/log_c.h"

#if OPENSSL_VERSION_NUMBER < 0x10100000L

  #define _ASN1_STRING_get0_data(x) \
	ASN1_STRING_data((x))

  #define _X509_NAME_entry_count(name) \
	sk_X509_NAME_ENTRY_num((name)->entries)

  #define _X509_NAME_get_entry(name, loc) \
	sk_X509_NAME_ENTRY_value((name)->entries, (loc))

  #define _X509_get0_tbs_sigalg(x) \
	((x)->sig_alg)

  #define _X509_get0_notBefore(x) \
	(((x) && (x)->cert_info && (x)->cert_info->validity) ? (x)->cert_info->validity->notBefore : NULL)

  #define _X509_get0_notAfter(x) \
	(((x) && (x)->cert_info && (x)->cert_info->validity) ? (x)->cert_info->validity->notAfter : NULL)

#else /* OPENSSL_VERSION_NUMBER >= 0x10100000L */

  #define _ASN1_STRING_get0_data(x) \
	ASN1_STRING_get0_data((x))

  #define _X509_NAME_entry_count(name) \
	X509_NAME_entry_count((name))

  #define _X509_NAME_get_entry(name, loc) \
	X509_NAME_get_entry((name), (loc))

  #define _X509_get0_tbs_sigalg(x) \
	X509_get0_tbs_sigalg((x))

  #define _X509_get0_notBefore(x) \
	X509_get0_notBefore((x))

  #define _X509_get0_notAfter(x) \
	X509_get0_notAfter((x))

#endif /* OPENSSL_VERSION_NUMBER < 0x10100000L */

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
 * @brief Certificates used for CMS validation.
 */
static
X509_STORE *ca_certs = NULL;

/*!
 * @brief Number of certificates loaded.
 */
static
int loaded_ca_certs = 0;

/*!
 * @brief Read certificate from a PEM string.
 *
 * @param[in,out] store   Certificate storage.
 * @param[in]     pem_str Certificate string.
 * @return 0 certificate was parsed and was stored, -1 else.
 */
static
int x509_store_add_cert_pem(X509_STORE *store, const char *pem_str);

/*!
 * @brief Read certificate file.
 *
 * @param[in,out] store      Certificate storage.
 * @param[in]     fname      Full name of file containing certificates.
 * @param[out]    num_loaded Number of loaded certificates.
 * @return 0 if at least one certificate could be loaded, -1 else.
 */
static
int x509_store_add_cert_file(X509_STORE *store, const char *fname,
    int *num_loaded);

/*!
 * @brief Load CMS from buffer.
 *
 * @return NULL on failure.
 */
static
CMS_ContentInfo *cms_load_der(const void *raw, size_t raw_len);

/*!
 * @brief Load X509 from buffer.
 *
 * @return NULL on failure.
 */
static
X509 *x509_load_der(const void *raw, size_t raw_len);

/*!
 * @brief Converts X509 certificate to DER.
 *
 * @param[in]  x509    Certificate.
 * @param[out] der     Pointer to allocated buffer. Use free() to free memory.
 * @param[out] der_len Size of the allocate buffer.
 */
static
int x509_store_der(X509 *x509, void **der, size_t *der_len);

/*!
 * @brief Load X509 from buffer.
 *
 * @return NULL on failure.
 */
static
X509 *x509_load_pem(const char *pem_str);

/*!
 * @brief Converts X509 certificate to PEM.
 *
 * @param[in]  x509     Certificate.
 * @param[out] pem      Pointer to allocated buffer. Use free() to free memory.
 * @param[out] pem_len  Size of the allocated buffer.
 */
static
int x509_store_pem(X509 *x509, void **pem, size_t *pem_len);

/*!
 * @brief Converts the private key to PEM.
 *
 * @param[in]  pkey     Private key.
 * @param[out] pem      Pointer to allocated buffer. Use free() to free memory.
 * @param[out] pem_len  Size of the allocated buffer.
 * @param[in]  pwd      Password to encrypt the private key in PEM format.
 */
static
int evp_pkey_store_pem(EVP_PKEY *pkey, void **pem, size_t *pem_len,
    const char *pwd);

/*!
 * @brief Load X509_CRL from buffer.
 *
 * @return NULL on failure.
 */
static
X509_CRL *x509_crl_load_der(const void *raw, size_t raw_len);

/*!
 * @brief Load PKCS12 from buffer.
 *
 * @return NULL on failure.
 */
static
PKCS12 *pkcs12_load_p12(const void *raw, size_t raw_len);

/*!
 * @brief Verify the signature of the CRL.
 *
 * @param[in] x509_clr CRL to be checked.
 *
 * @return  1 if signature valid,
 *          0 if signature invalid,
 *         -1 on error.
 */
static
int x509_crl_verify_signature(X509_CRL *x509_crl);

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

//#define PRINT_CERTS 1
//#define CUSTOM_VERIFY_CB 1

#if defined PRINT_CERTS
/*!
 * @brief Prints x509.
 */
static
int x509_printf(X509 *x509, FILE *fout);
#endif /* PRINT_CERTS */

#if defined CUSTOM_VERIFY_CB
/*!
 * @brief Custom certificate verification callback.
 *
 * @note Can be used to override some verification errors.
 */
static
int cert_verify_cb(int ok, X509_STORE_CTX *ctx);
#endif /* CUSTOM_VERIFY_CB */

/*!
 * @brief Forces verification success and sets various variables according to
 *     verification progress.
 */
static
int cert_force_success(int ok, X509_STORE_CTX *ctx);

int crypto_init(void)
{
	const char **pem_file;
	const struct pem_str *pem_desc;
	int loaded_at_once;
#define MAX_PATH_LEN 256
	size_t file_name_len;
	char file_path[MAX_PATH_LEN];

	debug_func_call();

	/* Needed for CMS validation and PKCS #12. */
	OpenSSL_add_all_algorithms();

	ERR_load_crypto_strings();
	ERR_load_CMS_strings();
	//ERR_free_strings();

	if (NULL != ca_certs) {
		X509_STORE_free(ca_certs); ca_certs = NULL;
		loaded_ca_certs = 0;
	}

	ca_certs = X509_STORE_new();
	if (NULL == ca_certs) {
		goto fail;
	}

	/* Load from files. */
	pem_file = pem_files;
	assert(NULL != pem_file);
	/* The prefix doesn't change. */
	const char *crt_dir = CERT_PATH_PREFIX CERT_DIR;
	const size_t crt_dir_path_len = strlen(crt_dir);
	if (crt_dir_path_len >= MAX_PATH_LEN) {
		log_error("File path buffer is too short for '%s'.\n",
		    crt_dir);
		goto skip_files;
	}
	memcpy(file_path, crt_dir, crt_dir_path_len);
	file_path[crt_dir_path_len] = '\0';
	while (NULL != *pem_file) {
		/* Append file name to construct full file name. */
		file_name_len = strlen(*pem_file);
		if ((crt_dir_path_len + file_name_len) >= MAX_PATH_LEN) {
			log_error("File path buffer is too short for '%s%s'.\n",
			    crt_dir, *pem_file);
			continue;
		}
		memcpy(file_path + crt_dir_path_len, *pem_file, file_name_len);
		file_path[crt_dir_path_len + file_name_len] = '\0';

		if (0 != x509_store_add_cert_file(ca_certs, file_path,
		             &loaded_at_once)) {
			log_warning("Could not load certificate file '%s'.\n",
			    file_path);
		} else {
			loaded_ca_certs += loaded_at_once;
		}
		++pem_file;
	}
skip_files:

	/* Load from built-in certificates. */
	pem_desc = all_pem_strs;
	assert(NULL != pem_desc);
	while ((NULL != pem_desc->name) && (NULL != pem_desc->pem)) {
		if (0 != x509_store_add_cert_pem(ca_certs, pem_desc->pem)) {
			log_warning("Could not load certificate '%s'.\n",
			    pem_desc->name);
		} else {
			log_info("Loaded certificate '%s'.\n", pem_desc->name);
			++loaded_ca_certs;
		}
		++pem_desc;
	}

	if (0 == loaded_ca_certs) {
		log_error("%s\n", "Did not load any certificate.");
	}

#ifdef CUSTOM_VERIFY_CB
	X509_STORE_set_verify_cb(ca_certs, cert_verify_cb);
#else /* !CUSTOM_VERIFY_CB */
	X509_STORE_set_verify_cb(ca_certs, NULL);
#endif /* CUSTOM_VERIFY_CB */

	return 0;

fail:
	if (NULL != ca_certs) {
		X509_STORE_free(ca_certs); ca_certs = NULL;
	}
	return -1;
#undef MAX_PATH_LEN
}

int crypto_certificates_loaded(void)
{
	return (NULL != ca_certs) && (0 < loaded_ca_certs);
}

//#define USE_VERIFY_PARAM
/*
 * Using of X509_V_FLAG_CRL_CHECK causes the X509_V_ERR_UNABLE_TO_GET_CRL error
 * to be generated.
 */
#define VERIFY_FLAGS (X509_V_FLAG_CRL_CHECK_ALL)

int crypto_add_crl(const void *der, size_t der_size)
{
#ifdef USE_VERIFY_PARAM
	X509_VERIFY_PARAM *param = NULL;
#endif /* USE_VERIFY_PARAM */
	X509_CRL *x509_crl = NULL;

	if (NULL == ca_certs) {
		assert(0);
		log_error("%s\n",
		    "Cryptographic context has not been initialised.");
		goto fail;
	}

	x509_crl = x509_crl_load_der(der, der_size);
	if (NULL == x509_crl) {
		log_error("%s\n", "Could not load CRL.");
		goto fail;
	}

	if (1 != x509_crl_verify_signature(x509_crl)) {
		log_error("%s\n", "Could not verify CRL signature.");
		goto fail;
	}

	if (!X509_STORE_add_crl(ca_certs, x509_crl)) {
		log_error("%s\n", "Could not store CRL.");
		goto fail;
	}
	x509_crl = NULL;

#ifndef USE_VERIFY_PARAM
	if (!X509_STORE_set_flags(ca_certs, VERIFY_FLAGS)) {
		log_error("%s\n", "Error setting CRL check flags.");
		goto fail;
	}
#else /* !USE_VERIFY_PARAM */
	param = X509_VERIFY_PARAM_new();
	if (NULL == param) {
		goto fail;
	}

	if (!X509_VERIFY_PARAM_set_flags(param, VERIFY_FLAGS)) {
		goto fail;
	}

	if (!X509_STORE_set1_param(ca_certs, param)) {
		goto fail;
	}

	X509_VERIFY_PARAM_free(param); param = NULL;
#endif /* USE_VERIFY_PARAM */

	return 0;

fail:
#ifdef USE_VERIFY_PARAM
	if (NULL != param) {
		X509_VERIFY_PARAM_free(param);
	}
#endif /* USE_VERIFY_PARAM */
	if (NULL != x509_crl) {
		X509_CRL_free(x509_crl);
	}
	return -1;
}

int raw_msg_verify_signature(const void *der, size_t der_size, int verify_cert,
    int crl_check)
{
//#define PRINT_HANDLED_CERT

	int ret;
	CMS_ContentInfo *cms = NULL;
#if defined PRINT_CERTS && defined PRINT_HANDLED_CERT
	STACK_OF(X509) *signers = NULL;
#endif /* PRINT_CERTS */

	debug_func_call();

	cms = cms_load_der(der, der_size);
	if (NULL == cms) {
		log_error("%s\n", "Could not load CMS.");
		return -1;
	}

	ret = cms_verify_signature(cms,
	    (0 == verify_cert) ? NULL : ca_certs, crl_check);
#if defined PRINT_CERTS && defined PRINT_HANDLED_CERT
	signers = CMS_get0_signers(cms);
	fprintf(stderr, ">>>\n");
	x509_printf(sk_X509_value(signers, 0), stderr);
	fprintf(stderr, "<<<\n");
	sk_X509_free(signers); signers = NULL;
#endif /* PRINT_CERTS */

	CMS_ContentInfo_free(cms); cms = NULL;

	return ret;

#ifdef PRINT_HANDLED_CERT
#  undef PRINT_HANDLED_CERT
#endif /* PRINT_HANDLED_CERT */
}

int raw_msg_verify_signature_date(const void *der, size_t der_size,
    time_t utc_time, int crl_check)
{
	int ret;
	CMS_ContentInfo *cms = NULL;
	STACK_OF(X509) *signers = NULL;
	unsigned long err;
	int num_signers;

	debug_func_call();

	cms = cms_load_der(der, der_size);
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

int raw_tst_verify(const void *der, size_t der_size, time_t *utc_time)
{
//#define PRINT_HANDLED_CERT

	int ret;
	CMS_ContentInfo *cms = NULL;
	STACK_OF(X509) *signers = NULL;
	X509 *x509;
	int num_signers;

	debug_func_call();

	cms = cms_load_der(der, der_size);
	if (NULL == cms) {
		log_error("%s\n", "Could not load CMS.");
		goto fail;
	}

	/*
	 * The timestamps are provided by the postsignum.cz certification
	 * authority.
	 * TODO -- Currently the signing certificate is not checked against
	 * any other certificate here. This is because the check fails
	 * signalling invalid certificate purpose. Therefore an explicit
	 * certificate check is performed.
	 */
	ret = cms_verify_signature(cms, NULL, 0);
	if (-1 == ret) {
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

	x509 = sk_X509_value(signers, 0);

#if defined PRINT_CERTS && defined PRINT_HANDLED_CERT
	fprintf(stderr, ">>>\n");
	x509_printf(x509, stderr);
	fprintf(stderr, "<<<\n");
#endif /* PRINT_CERTS */

	/* Explicitly verify signature. */
	ret = x509_crt_verify((struct x509_crt *) x509);
	if (-1 == ret) {
		goto fail;
	}

	if (1 == ret) {
		/* Check certificate purpose. */
		ret = X509_check_purpose(x509, X509_PURPOSE_TIMESTAMP_SIGN,
		    0) ? 1 : 0;
	}

	x509 = NULL;
	sk_X509_free(signers); signers = NULL;

	if (NULL == utc_time) {
		assert(0);
		goto fail;
	}

	if (-1 == cms_get_timestamp_value(cms, utc_time)) {
		goto fail;
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

#ifdef PRINT_HANDLED_CERT
#  undef PRINT_HANDLED_CERT
#endif /* PRINT_HANDLED_CERT */
}

struct x509_crt *raw_cms_signing_cert(const void *der, size_t der_size)
{
	int ret;
	CMS_ContentInfo *cms = NULL;
	STACK_OF(X509) *signers = NULL;
	X509 *x509 = NULL;
	int num_signers;

	debug_func_call();

	cms = cms_load_der(der, der_size);
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

	x509 = X509_dup(sk_X509_value(signers, 0));
	if (NULL == x509) {
		log_error("%s\n", "Cannot copy X509.");
		goto fail;
	}

	sk_X509_free(signers); signers = NULL;

	CMS_ContentInfo_free(cms); cms = NULL;

	return (struct x509_crt *) x509;
fail:
	if (NULL != cms) {
		CMS_ContentInfo_free(cms);
	}
	if (NULL != signers) {
		sk_X509_free(signers);
	}
	return NULL;
}

void x509_crt_destroy(struct x509_crt *x509_crt)
{
	debug_func_call();

	assert(NULL != x509_crt);

	X509_free((X509 *) x509_crt);
}

int x509_crt_to_der(struct x509_crt *x509_crt, void **der_out,
    size_t *out_size)
{
	debug_func_call();

	return x509_store_der((X509 *) x509_crt, der_out, out_size);
}

struct x509_crt *x509_crt_from_der(const void *der, size_t der_size)
{
	debug_func_call();

	return (struct x509_crt *) x509_load_der(der, der_size);
}

void crt_issuer_info_init(struct crt_issuer_info *cii)
{
	if (NULL == cii) {
		assert(0);
		return;
	}

	memset(cii, 0, sizeof(*cii));
}

void crt_issuer_info_clear(struct crt_issuer_info *cii)
{
	if (NULL == cii) {
		assert(0);
		return;
	}

	if (NULL != cii->o) {
		free(cii->o); cii->o = NULL;
	}
	if (NULL != cii->ou) {
		free(cii->ou); cii->ou = NULL;
	}
	if (NULL != cii->n) {
		free(cii->n); cii->n = NULL;
	}
	if (NULL != cii->c) {
		free(cii->c); cii->c = NULL;
	}
}

int x509_crt_issuer_info(struct x509_crt *x509_crt,
    struct crt_issuer_info *cii)
{
	const X509_NAME *subject;
	int num_entries, i;
	X509_NAME_ENTRY *entry;
	ASN1_OBJECT *object;
	int nid;
	ASN1_STRING *str;
	char **out_str;

	debug_func_call();

	if (NULL == x509_crt) {
		assert(0);
		goto fail;
	}
	if (NULL == cii) {
		assert(0);
		goto fail;
	}

	subject = X509_get_subject_name((X509 *) x509_crt);
	if (NULL == subject) {
		goto fail;
	}

	num_entries = _X509_NAME_entry_count(subject);

	for (i = 0; i < num_entries; ++i) {
		entry = _X509_NAME_get_entry(subject, i);
		object = X509_NAME_ENTRY_get_object(entry);
		/* Reading object->nid doesn't work. */
		nid = OBJ_obj2nid(object);
		str = X509_NAME_ENTRY_get_data(entry);

		out_str = NULL;

		switch (nid) {
		case NID_organizationName:
			out_str = &cii->o;
			break;
		case NID_organizationalUnitName:
			out_str = &cii->ou;
			break;
		case NID_commonName:
			out_str = &cii->n;
			break;
		case NID_countryName:
			out_str = &cii->c;
			break;
		default:
			break;
		}

		if (NULL != out_str) {
			*out_str = malloc(ASN1_STRING_length(str) + 1);
			if (NULL != *out_str) {
				memcpy(*out_str, _ASN1_STRING_get0_data(str),
				    ASN1_STRING_length(str));
				(*out_str)[ASN1_STRING_length(str)] = '\0';
			}
		}
	}

	return 0;

fail:
	return -1;
}

int x509_crt_algorithm_info(struct x509_crt *x509_crt, char **sa_id,
    char **sa_name)
{
//#define PRINT_HANDLED_CERT

/* According to documentation of OBJ_obj2txt 80 should be enough. */
#define MAX_ID_LEN 80
	const X509_ALGOR *sa;
	const ASN1_OBJECT *alg;
	int sig_nid;
	const char *ln;
	char id[MAX_ID_LEN];
	int id_len;

	debug_func_call();

	if (NULL == x509_crt) {
		assert(0);
		goto fail;
	}

#if defined PRINT_CERTS && defined PRINT_HANDLED_CERT
	fprintf(stderr, ">>>\n");
	x509_printf((X509 *) x509_crt, stderr);
	fprintf(stderr, "<<<\n");
#endif /* PRINT_CERTS */

	sa = _X509_get0_tbs_sigalg((X509 *)x509_crt);
	alg = sa->algorithm;

	if (NULL != sa_id) {
		id_len = OBJ_obj2txt(id, MAX_ID_LEN, alg, 1);
		id[MAX_ID_LEN - 1] = '\0';
		*sa_id = malloc(id_len + 1);
		if (NULL != *sa_id) {
			memcpy(*sa_id, id, id_len);
			(*sa_id)[id_len] = '\0';
		}
	}

	if (NULL != sa_name) {
		sig_nid = OBJ_obj2nid(alg);
		ln = OBJ_nid2ln(sig_nid);
		*sa_name = malloc(strlen(ln) + 1);
		if (NULL != *sa_name) {
			memcpy(*sa_name, ln, strlen(ln));
			(*sa_name)[strlen(ln)] = '\0';
		}
	}

	return 0;

fail:
	return -1;

#undef MAX_ID_LEN

#ifdef PRINT_HANDLED_CERT
#  undef PRINT_HANDLED_CERT
#endif /* PRINT_HANDLED_CERT */
}

int x509_crt_date_info(struct x509_crt *x509_crt, time_t *utc_inception,
    time_t *utc_expiration)
{
	const ASN1_TIME *notBefore, *notAfter;

	/* ASN1_TIME is an alias for ASN1_STRING. */

	debug_func_call();

	if (NULL == x509_crt) {
		assert(0);
		goto fail;
	}

	notBefore = _X509_get0_notBefore((X509 *)x509_crt);
	if (NULL == notBefore) {
		assert(0);
		goto fail;
	}
	notAfter = _X509_get0_notAfter((X509 *)x509_crt);
	if (NULL == notAfter) {
		assert(0);
		goto fail;
	}

	if (NULL != utc_inception) {
		asn1_time_to_utc_time(notBefore, utc_inception);
	}
	if (NULL != utc_expiration) {
		asn1_time_to_utc_time(notAfter, utc_expiration);
	}

	return 0;

fail:
	return -1;
}

int x509_crt_verify(struct x509_crt *x509_crt)
{
//#define PRINT_HANDLED_CERT

	int ret = 0;
	X509_STORE_CTX *csc = NULL;
	unsigned long err;

	debug_func_call();

	if (NULL == ca_certs) {
		assert(0);
		log_error("%s\n",
		    "Cryptographic context has not been initialised.");
		goto fail;
	}

#if defined PRINT_CERTS && defined PRINT_HANDLED_CERT
	fprintf(stderr, ">>>\n");
	x509_printf((X509 *) x509_crt, stderr);
	fprintf(stderr, "<<<\n");
#endif /* PRINT_CERTS */

	csc = X509_STORE_CTX_new();
	if (NULL == csc) {
		while (0 != (err = ERR_get_error())) {
			log_error("openssl error: %s\n",
			    ERR_error_string(err, NULL));
		}
		ret = -1;
		goto fail;
	}

	if (!X509_STORE_CTX_init(csc, ca_certs, (X509 *) x509_crt, NULL)) {
		while (0 != (err = ERR_get_error())) {
			log_error("openssl error: %s\n",
			    ERR_error_string(err, NULL));
		}
		ret = -1;
		goto fail;
	}

	/* TODO -- CRL */

	if (0 < X509_verify_cert(csc)) {
		ret = 1;
	} else {
		while (0 != (err = ERR_get_error())) {
			log_error("openssl error: %s\n",
			    ERR_error_string(err, NULL));
		}
		ret = 0;
	}

	X509_STORE_CTX_free(csc); csc = NULL;

	return ret;

fail:
	if (NULL != csc) {
		X509_STORE_CTX_free(csc);
	}
	return -1;

#ifdef PRINT_HANDLED_CERT
#  undef PRINT_HANDLED_CERT
#endif /* PRINT_HANDLED_CERT */
}

static
struct crt_verif_outcome glob_cvo;

int x509_crt_track_verification(struct x509_crt *x509_crt,
    struct crt_verif_outcome *cvo)
{
//#define PRINT_HANDLED_CERT

	/* TODO -- This function is nearly the same as x509_crt_verify(). */

	int ret = 0;
	X509_STORE_CTX *csc = NULL;
	unsigned long err;

	debug_func_call();

	glob_cvo.parent_crt_not_found = 0;
	glob_cvo.time_validity_fail = 0;
	glob_cvo.crt_revoked = 0;
	glob_cvo.crt_signature_invalid = 0;

	if (NULL == ca_certs) {
		assert(0);
		log_error("%s\n",
		    "Cryptographic context has not been initialised.");
		goto fail;
	}

#if defined PRINT_CERTS && defined PRINT_HANDLED_CERT
	fprintf(stderr, ">>>\n");
	x509_printf((X509 *) x509_crt, stderr);
	fprintf(stderr, "<<<\n");
#endif /* PRINT_CERTS */

	csc = X509_STORE_CTX_new();
	if (NULL == csc) {
		while (0 != (err = ERR_get_error())) {
			log_error("openssl error: %s\n",
			    ERR_error_string(err, NULL));
		}
		ret = -1;
		goto fail;
	}

	if (!X509_STORE_CTX_init(csc, ca_certs, (X509 *) x509_crt, NULL)) {
		while (0 != (err = ERR_get_error())) {
			log_error("openssl error: %s\n",
			    ERR_error_string(err, NULL));
		}
		ret = -1;
		goto fail;
	}

	/* Force verification tracking. */
	X509_STORE_CTX_set_verify_cb(csc, cert_force_success);

	/* TODO -- CRL */

	if (0 < X509_verify_cert(csc)) {
		ret = 1;
	} else {
		while (0 != (err = ERR_get_error())) {
			log_error("openssl error: %s\n",
			    ERR_error_string(err, NULL));
		}
		ret = 0;
	}

	/* Restore default behaviour. */
#if defined CUSTOM_VERIFY_CB
	X509_STORE_CTX_set_verify_cb(csc, cert_verify_cb);
#else /* !CUSTOM_VERIFY_CB */
	X509_STORE_CTX_set_verify_cb(csc, NULL);
#endif /* CUSTOM_VERIFY_CB */

	if (NULL != cvo) {
		*cvo = glob_cvo;
	}

	X509_STORE_CTX_free(csc); csc = NULL;

	return ret;

fail:
	if (NULL != csc) {
		X509_STORE_CTX_free(csc);
	}
	return -1;

#ifdef PRINT_HANDLED_CERT
#  undef PRINT_HANDLED_CERT
#endif /* PRINT_HANDLED_CERT */
}

int p12_to_pem(const void *p12, size_t p12_size, const char *pwd,
    void **pem_out, size_t *out_size)
{
	PKCS12 *pkcs12 = NULL;
	unsigned long err;
	X509 *cert = NULL;
	EVP_PKEY *pkey = NULL;
	void *cert_pem = NULL;
	size_t cert_pem_len;
	void *pkey_pem = NULL;
	size_t pkey_pem_len;

	pkcs12 = pkcs12_load_p12(p12, p12_size);
	if (NULL == pkcs12) {
		goto fail;
	}

	if (0 == PKCS12_verify_mac(pkcs12, pwd, -1)) {
		while (0 != (err = ERR_get_error())) {
			log_error("openssl error: %s\n",
			    ERR_error_string(err, NULL));
		}
		goto fail;
	}

	/*
	 * In openssl pkcs12 application they've created dump_certs_keys_p12.
	 * This should apparently be a workaround around the bug in
	 * PKCS12_parse() whose main disadvantage is that it returns only one
	 * private key and certificate. Hope that PKCS12_parse() will suffice
	 * because I don't wan to program the whole stuff.
	 */

	int ret = PKCS12_parse(pkcs12, pwd, &pkey, &cert, NULL);
	if (0 == ret) {
		while (0 != (err = ERR_get_error())) {
			log_error("openssl error: %s\n",
			    ERR_error_string(err, NULL));
		}
		goto fail;
	}
	PKCS12_free(pkcs12); pkcs12 = NULL;

	if (0 != x509_store_pem(cert, &cert_pem, &cert_pem_len)) {
		goto fail;
	}
	X509_free(cert); cert = NULL;

	if (0 != evp_pkey_store_pem(pkey, &pkey_pem, &pkey_pem_len, pwd)) {
		goto fail;
	}
	EVP_PKEY_free(pkey); pkey = NULL;

	/* A trailing zero will be added to the sting. */

	if (NULL != pem_out) {
		*pem_out = malloc(cert_pem_len + pkey_pem_len + 1);
		if (NULL == *pem_out) {
			goto fail;
		}

		memcpy(*pem_out, cert_pem, cert_pem_len);
		memcpy((char *) *pem_out + cert_pem_len, pkey_pem,
		    pkey_pem_len);
		((char *) *pem_out)[cert_pem_len + pkey_pem_len] = '\0';
	}

	if (NULL != out_size) {
		*out_size = cert_pem_len + pkey_pem_len;
	}

	free(cert_pem); cert_pem = NULL;
	memset(pkey_pem, 0, pkey_pem_len);
	free(pkey_pem); pkey_pem = NULL;

	return 0;

fail:
	if (NULL != pkcs12) {
		PKCS12_free(pkcs12);
	}
	if (NULL != cert) {
		X509_free(cert);
	}
	if (NULL != pkey) {
		EVP_PKEY_free(pkey);
	}
	if (NULL != cert_pem) {
		free(cert_pem);
	}
	if (NULL != pkey_pem) {
		free(pkey_pem);
	}
	return -1;
}

static
int x509_store_add_cert_pem(X509_STORE *store, const char *pem_str)
{
//#define PRINT_HANDLED_CERT

	X509 *x509 = NULL;
	unsigned long err;

	debug_func_call();

	if (NULL == store) {
		assert(0);
		goto fail;
	}
	if (NULL == pem_str) {
		assert(0);
		goto fail;
	}

	x509 = x509_load_pem(pem_str);
	if (NULL == x509) {
		goto fail;
	}

	if (X509_STORE_add_cert(store, x509) == 0) {
		err = ERR_get_error();
		log_error("%s\n", "Cannot store certificate.");
		while (0 != (err = ERR_get_error())) {
			log_error("openssl error: %s\n",
			    ERR_error_string(err, NULL));
		}
		goto fail;
	}

#if defined PRINT_CERTS && defined PRINT_HANDLED_CERT
	fprintf(stderr, ">>>\n");
	x509_printf(x509, stderr);
	fprintf(stderr, "<<<\n");
#endif /* PRINT_CERTS */

	X509_free(x509); x509 = NULL;

	return 0;

fail:
	if (NULL != x509) {
		X509_free(x509);
	}
	return -1;

#ifdef PRINT_HANDLED_CERT
#  undef PRINT_HANDLED_CERT
#endif /* PRINT_HANDLED_CERT */
}

static
int x509_store_add_cert_file(X509_STORE *store, const char *fname,
    int *num_loaded)
{
//#define PRINT_HANDLED_CERT

	BIO *bio = NULL;
	X509 *x509 = NULL;
	unsigned long err;
	int read_pems = 0, stored_pems = 0; /* PEM counters. */

	debug_func_call();

	/* TODO -- Does it store all certificates from a single file? */

	if (NULL == store) {
		assert(0);
		goto fail;
	}
	if (NULL == fname) {
		assert(0);
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

#if defined PRINT_CERTS && defined PRINT_HANDLED_CERT
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

	/* stored_pems cannot be 0 here
	if (0 == stored_pems) {
		goto fail;
	}
	*/

	return 0;

fail:
	if (NULL != bio) {
		BIO_free(bio);
	}
	/* x509 cannot be NULL here
	if (NULL != x509) {
		X509_free(x509);
	}
	*/
	return -1;

#ifdef PRINT_HANDLED_CERT
#  undef PRINT_HANDLED_CERT
#endif /* PRINT_HANDLED_CERT */
}

static
CMS_ContentInfo * cms_load_der(const void *raw, size_t raw_len)
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

static
X509 *x509_load_der(const void *raw, size_t raw_len)
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

static
int x509_store_der(X509 *x509, void **der, size_t *der_len)
{
	BIO *bio = NULL;
	BUF_MEM *bptr = NULL;

	if (NULL == x509) {
		assert(0);
		goto fail;
	}
	if (NULL == der) {
		assert(0);
		goto fail;
	}
	if (NULL == der_len) {
		assert(0);
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

static
X509 *x509_load_pem(const char *pem_str)
{
	BIO *bio = NULL;
	X509 *x509 = NULL;
	unsigned long err;

	bio = BIO_new_mem_buf((void *) pem_str, strlen(pem_str));
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

	return x509;

fail:
	if (NULL != bio) {
		BIO_free(bio);
	}
	return NULL;
}

static
int x509_store_pem(X509 *x509, void **pem, size_t *pem_len)
{
	BIO *bio = NULL;
	BUF_MEM *bptr = NULL;

	if (NULL == x509) {
		assert(0);
		goto fail;
	}
	if (NULL == pem) {
		assert(0);
		goto fail;
	}
	if (NULL == pem_len) {
		assert(0);
		goto fail;
	}

	bio = BIO_new(BIO_s_mem());

	if (!PEM_write_bio_X509(bio, x509)) {
		log_error("%s\n", "Could not write X509 to bio.");
		goto fail;
	}

	BIO_get_mem_ptr(bio, &bptr);
	if (!BIO_set_close(bio, BIO_NOCLOSE)) {
		goto fail;
	}
	BIO_free(bio); bio = NULL;

	/* Extract DER from buffer. */
	*pem = bptr->data; bptr->data = NULL;
	*pem_len = bptr->length; bptr->length = 0;
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

static
int evp_pkey_store_pem(EVP_PKEY *pkey, void **pem, size_t *pem_len,
    const char *pwd)
{
	const EVP_CIPHER *enc = EVP_des_ede3_cbc();
	BIO *bio = NULL;
	BUF_MEM *bptr = NULL;

	if (NULL == pkey) {
		assert(0);
		goto fail;
	}
	if (NULL == pem) {
		assert(0);
		goto fail;
	}
	if (NULL == pem_len) {
		assert(0);
		goto fail;
	}

	bio = BIO_new(BIO_s_mem());

	if ((NULL == pwd) || ('\0' == pwd[0])) {
		/* Don't encrypt when having empty password. */
		pwd = NULL;
		enc = NULL;
	}

	if (!PEM_write_bio_PrivateKey(bio, pkey, enc, NULL, 0, NULL,
	        (char *) pwd)) {
		log_error("%s\n", "Could not write X509 to bio.");
		goto fail;
	}

	BIO_get_mem_ptr(bio, &bptr);
	if (!BIO_set_close(bio, BIO_NOCLOSE)) {
		goto fail;
	}
	BIO_free(bio); bio = NULL;

	/* Extract DER from buffer. */
	*pem = bptr->data; bptr->data = NULL;
	*pem_len = bptr->length; bptr->length = 0;
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

static
X509_CRL *x509_crl_load_der(const void *raw, size_t raw_len)
{
	BIO *bio = NULL;
	X509_CRL *x509_crl = NULL;
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

	x509_crl = d2i_X509_CRL_bio(bio, NULL);
	if (NULL == x509_crl) {
		log_error("%s\n", "Cannot parse X509_CRL.");
		while (0 != (err = ERR_get_error())) {
			log_error("openssl error: %s\n",
			    ERR_error_string(err, NULL));
		}
		goto fail;
	}

	BIO_free(bio); bio = NULL;

	return x509_crl;

fail:
	if (NULL != bio) {
		BIO_free(bio);
	}
	return NULL;
}

static
PKCS12 *pkcs12_load_p12(const void *raw, size_t raw_len)
{
	BIO *bio = NULL;
	PKCS12 *pkcs12 = NULL;
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

	pkcs12 = d2i_PKCS12_bio(bio, NULL);
	if (NULL == pkcs12) {
		log_error("%s\n", "Cannot parse PKCS12.");
		while (0 != (err = ERR_get_error())) {
			log_error("openssl error: %s\n",
			    ERR_error_string(err, NULL));
		}
		goto fail;
	}

	BIO_free(bio); bio = NULL;

	return pkcs12;

fail:
	if (NULL != bio) {
		BIO_free(bio);
	}
	return NULL;
}

static
int x509_crl_verify_signature(X509_CRL *x509_crl)
{
	X509 *x509 = NULL;
	const struct pem_str *pem_desc;
	EVP_PKEY *pkey = NULL;
	int verified = -1;

	pem_desc = msg_pem_strs;
	assert(NULL != pem_desc);
	while ((1 != verified) &&
	       (NULL != pem_desc->name) && (NULL != pem_desc->pem)) {
		x509 = x509_load_pem(pem_desc->pem);
		if (NULL == x509) {
			/* Don't generate an error. */
			goto next;
		}

		pkey = X509_get_pubkey(x509);
		if (NULL == pkey) {
			goto next;
		}

		if (X509_CRL_verify(x509_crl, pkey)) {
			verified = 1;
		} else if (-1 == verified) {
			verified = 0;
		}

next:
		if (NULL != x509) {
			X509_free(x509); x509 = NULL;
		}
		if (NULL != pkey) {
			EVP_PKEY_free(pkey); pkey = NULL;
		}
		++pem_desc;
	}

	return verified;
}

static
int cms_verify_signature(CMS_ContentInfo *cms, X509_STORE *ca_store,
    int crl_check)
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

static
int cms_get_timestamp_value(CMS_ContentInfo *cms, time_t *utc_time)
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

	if (NULL == utc_time) {
		assert(0);
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

static
int x509_check_date(const X509 *x509, time_t utc_time)
{
	const ASN1_TIME *notBefore, *notAfter;
	time_t tNotBef, tNotAft;

	/* ASN1_TIME is an alias for ASN1_STRING. */

	debug_func_call();

	if (NULL == x509) {
		assert(0);
		goto fail;
	}

	notBefore = _X509_get0_notBefore(x509);
	if (NULL == notBefore) {
		assert(0);
		goto fail;
	}
	notAfter = _X509_get0_notAfter(x509);
	if (NULL == notAfter) {
		assert(0);
		goto fail;
	}
	asn1_time_to_utc_time(notBefore, &tNotBef);
	asn1_time_to_utc_time(notAfter, &tNotAft);

	return (tNotBef <= utc_time) && (utc_time <= tNotAft);

fail:
	return -1;
}

static
int asn1_time_to_utc_time(const ASN1_TIME *asn1_time, time_t *utc_time)
{
	struct tm t;
	const char *str;
	unsigned int i = 0;
	char adjust_op, colon;
	int adj_hour = 0, adj_min = 0;
	int adj_secs;
	time_t ret_time;

//	fprintf(stderr, "TIME %s\n",
//	    _ASN1_STRING_get0_data((const ASN1_STRING *) asn1_time));

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

	if (('+' != adjust_op) && ('-' != adjust_op)) {
		assert(0);
		return -1;
	}

	adj_hour =  (str[i++] - '0') * 10;
	adj_hour += (str[i++] - '0');

	colon = str[i++];
	if (':' != colon) {
		assert(0);
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

#ifndef WIN32
/*!
 * @brief This function should stop Coverity from keeping complaining about
 *     a tainted string.
 *
 * @param[in] tz_val Time zone variable value.
 * @return Copy of value or NULL when NULL passed.
 */
char *sanitised_tz_val(const char *tz_val)
{
	if (NULL != tz_val) {
		return strdup(tz_val);
	}

	return NULL;
}
#endif /* !WIN32 */

static
time_t timegm_utc(struct tm *tm)
{
#ifndef WIN32
	/* Code taken from man timegm(3). */
	/* Modified so that Coverity won't complain about tainted strings. */

	time_t ret = 0;
	char *tz = NULL;

	/*
	 * We only store a copy of the original value in order to restore it
	 * afterwards. We don't care about it's actual value.
	 */

	tz = sanitised_tz_val(getenv("TZ"));

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


#if defined PRINT_CERTS
static
int x509_printf(X509 *x509, FILE *fout)
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


#if defined CUSTOM_VERIFY_CB
static
int cert_verify_cb(int ok, X509_STORE_CTX *ctx)
{
	int error;

	error = X509_STORE_CTX_get_error(ctx);

	log_info("X509_V_ERR %d, ok %d\n", error, ok);

	/*
	 * Override certificate purpose check. Especially the validation of
	 * time stamp CMS containers fails with this error.
	 */
	if (X509_V_ERR_INVALID_PURPOSE == error) {
		return 1;
	}

	return ok;

}
#endif /* CUSTOM_VERIFY_CB */

static
int cert_force_success(int ok, X509_STORE_CTX *ctx)
{
//	X509 *err_cert;
	int err;
//	int depth;

//	err_cert = X509_STORE_CTX_get_current_cert(ctx);
	err = X509_STORE_CTX_get_error(ctx);
//	depth = X509_STORE_CTX_get_error_depth(ctx);

	if (0 == ok) {
		switch (err) {
		case X509_V_OK:
			break;
		case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
			glob_cvo.parent_crt_not_found = 1;
			break;
		case X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE:
		case X509_V_ERR_CERT_SIGNATURE_FAILURE:
			glob_cvo.crt_signature_invalid = 1;
			break;
		case X509_V_ERR_CERT_NOT_YET_VALID:
		case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
		case X509_V_ERR_CERT_HAS_EXPIRED:
		case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
			glob_cvo.time_validity_fail = 1;
			break;
		case X509_V_ERR_CERT_REVOKED:
			glob_cvo.crt_revoked = 1;
			break;
		default:
			log_error("%s\n",
			    "Unknown certificate verification error.");
			break;
		}
	}

	if ((X509_V_OK == err) && (2 == ok)) {
		/* Print out policies. */
	}

	return 1;
}
