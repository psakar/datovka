#include <stdlib.h>
#include <time.h>

#include "helper.h"
#include "src/crypto/crypto.h"
#include "src/crypto/crypto_nonthreadsafe.h"
#include "src/log/log_c.h"


#define P12_FILE "data/cert_and_key.p12"
#define P12_ENCRYPTED_FILE "data/cert_and_key_encrypted.p12"
#define PWD "Abcd0"


/* ========================================================================= */
/* ========================================================================= */
int main(void)
/* ========================================================================= */
/* ========================================================================= */
{
	void *p12_enc = NULL;
	size_t p12_enc_size;
	void *pem_enc = NULL;
	size_t pem_enc_size;

	void *p12 = NULL;
	size_t p12_size;
	void *pem = NULL;
	size_t pem_size;

	if (0 != crypto_init()) {
		goto fail;
	}

	p12_enc = internal_read_file(P12_ENCRYPTED_FILE, &p12_enc_size, "r");
	if (NULL == p12_enc) {
		goto fail;
	}

	if (0 != p12_to_pem(p12_enc, p12_enc_size, PWD,
	        &pem_enc, &pem_enc_size)) {
		log_error("%s\n", "Cannot convert encrypted PKCS12 to PEM.");
		goto fail;
	}
	free(p12_enc); p12_enc = NULL;

	log_info("%s", pem_enc);

	/*
	 * In order to check whether the private key has been correctly
	 * decrypted compare the output of these commands:
	 *
	 * openssl pkcs12 -in data/cert_and_key_encrypted.p12 -nocerts -nodes
	 * ./test_7 2>&1 | openssl pkey
	 */

	free(pem_enc); pem_enc = NULL;

	p12 = internal_read_file(P12_FILE, &p12_size, "r");
	if (NULL == p12) {
		goto fail;
	}

	if (0 != p12_to_pem(p12, p12_size, "", &pem, &pem_size)) {
		log_error("%s\n", "Cannot convert PKCS12 to PEM.");
		goto fail;
	}
	free(p12); p12 = NULL;

	log_info("%s", pem);

	free(pem); pem = NULL;

	return EXIT_SUCCESS;

fail:
	if (NULL != p12_enc) {
		free(p12_enc);
	}
	if (NULL != pem_enc) {
		free(pem_enc);
	}
	if (NULL != p12) {
		free(p12);
	}
	if (NULL != pem) {
		free(pem);
	}
	log_error("Error in file '%s'.\n", __FILE__);
	return EXIT_FAILURE;
}
