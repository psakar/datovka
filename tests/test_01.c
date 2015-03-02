

#include <stdlib.h>

#include "helper.h"
#include "src/crypto/crypto.h"
#include "src/crypto/crypto_nonthreadsafe.h"
#include "src/log/log_c.h"


//#define MESS_FILE "data/message_binary.dat"
#define MESS_FILE "data/DDZ_3401761.zfo"


/* ========================================================================= */
/* ========================================================================= */
int main(void)
/* ========================================================================= */
/* ========================================================================= */
{
	char *der = NULL;
	size_t der_len;

	struct x509_crt *x509_crt = NULL;

	int ret;

	if (0 != crypto_init()) {
		goto fail;
	}

	der = internal_read_file(MESS_FILE, &der_len, "r");
	if (NULL == der) {
		goto fail;
	}

	x509_crt = raw_cms_signing_cert(der, der_len);
	if (NULL == x509_crt) {
		goto fail;
	}

	free(der); der = NULL;

	ret = x509_crt_verify(x509_crt);
	log_info("Verification 1 result %d.\n", ret);
	if (-1 == ret) {
		goto fail;
	}

	ret = x509_crt_verify(x509_crt);
	log_info("Verification 2 result %d.\n", ret);
	if (-1 == ret) {
		goto fail;
	}

	ret = x509_crt_verify(x509_crt);
	log_info("Verification 3 result %d.\n", ret);
	if (-1 == ret) {
		goto fail;
	}

	x509_crt_destroy(x509_crt); x509_crt = NULL;

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
