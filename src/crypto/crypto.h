

#ifndef _CRYPTO_H_
#define _CRYPTO_H_


#ifdef __cplusplus
#  include <ctime>
#else
#  include <time.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif


/* Opaque structures to hide cryptographic implementation details. */
//struct cms_msg;
//struct cms_tst;
struct x509_crt;


/*!
 * @brief Holds locations of the CRL files.
 */
struct crl_location {
	const char *file_name; /*!< CRL file name. */
	const char **urls; /*!< NULL-terminated list of URLs. */
};


/*!
 * @brief NULL-terminated list of CRL files.
 */
extern const struct crl_location crl_locations[];


/*!
 * @brief Certificate issuer information.
 */
struct crt_issuer_info {
	char *o; /*!< Organisation name. */
	char *ou; /*!< Organisation unit name. */
	char *n; /*!< Common name. */
	char *c; /*!< Country name. */
};


#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* _CRYPTO_H_ */
