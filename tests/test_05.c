

#include <stdlib.h>
#include <time.h>

#include "helper.h"
#include "src/crypto/crypto.h"
#include "src/crypto/crypto_nonthreadsafe.h"
#include "src/log/log_c.h"


#define MESS_FILE "data/message_binary.dat"
//#define MESS_FILE "data/DDZ_3401761.zfo"
#define CRL1_FILE "../trusted_certs/crl/psrootqca.crl"
#define CRL2_FILE "../trusted_certs/crl/psrootqca2.crl"


/* ========================================================================= */
/* ========================================================================= */
int main(void)
/* ========================================================================= */
/* ========================================================================= */
{
	char *der = NULL;
	size_t der_len;
	struct x509_crt *x509_crt = NULL;
	time_t inception, expiration;
	int ret;

	if (0 != crypto_init()) {
		goto fail;
	}

#if 1
	der = internal_read_file(CRL1_FILE, &der_len, "r");
	if (NULL == der) {
		goto fail;
	}
	if (0 != crypto_add_crl(der, der_len)) {
		goto fail;
	}
	free(der); der = NULL;
#endif

#if 1
	der = internal_read_file(CRL2_FILE, &der_len, "r");
	if (NULL == der) {
		goto fail;
	}
	if (0 != crypto_add_crl(der, der_len)) {
		goto fail;
	}
	free(der); der = NULL;
#endif

	der = internal_read_file(MESS_FILE, &der_len, "r");
	if (NULL == der) {
		goto fail;
	}

	ret = raw_msg_verify_signature(der, der_len, 0, 0);
	log_info("Raw verification %d.\n", ret);

	ret = raw_msg_verify_signature(der, der_len, 1, 0);
	log_info("Raw verification %d.\n", ret);

	ret = raw_msg_verify_signature(der, der_len, 0, 1);
	log_info("Raw verification %d.\n", ret);

	ret = raw_msg_verify_signature(der, der_len, 1, 1);
	log_info("Raw verification %d.\n", ret);

	x509_crt = raw_cms_signing_cert(der, der_len);
	if (NULL == x509_crt) {
		goto fail;
	}
	free(der); der = NULL;

	if (0 != x509_crt_date_info(x509_crt, &inception, &expiration)) {
		log_error("%s\n", "Cannot obtain date details.");
		goto fail;
	}
	x509_crt_destroy(x509_crt); x509_crt = NULL;

	log_info("%d %d\n", inception, expiration);

	return EXIT_SUCCESS;

fail:
	if (NULL != der) {
		free(der);
	}
	if (NULL != x509_crt) {
		x509_crt_destroy(x509_crt);
	}
	log_error("Error in file '%s'.\n", __FILE__);
	return EXIT_FAILURE;
}
