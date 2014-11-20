

#ifndef _CRYPTO_THREADSAFE_H_
#define _CRYPTO_THREADSAFE_H_


#include <ctime>


/*
 * For function documentation see C sources.
 */


int cryptoInit(void);


int cryptoCertificatesLoaded(void);


int cryptoAddCrl(const void *der, size_t der_size);


int rawMsgVerifySignature(const void *der, size_t der_size, int verify_cert,
    int crl_check);


int rawMsgVerifySignatureDate(const void *der, size_t der_size,
    time_t utc_time, int crl_check);


int rawTstVerify(const void *der, size_t der_size, time_t *utc_time);


struct x509_crt * rawCmsSigningCert(const void *der, size_t der_size);


void x509CrtDestroy(struct x509_crt *x509_crt);


int x509CrtToDer(struct x509_crt *x509_crt, void **der_out,
    size_t *out_size);


struct x509_crt * x509CrtFromDer(const void *der, size_t der_size);


void crtIssuerInfoInit(struct crt_issuer_info *cii);


void crtIssuerInfoClear(struct crt_issuer_info *cii);


int x509CrtIssuerInfo(struct x509_crt *x509_crt,
    struct crt_issuer_info *cii);


int x509CrtAlgorithmInfo(struct x509_crt *x509_crt, char **sa_id,
    char **sa_name);


int x509CrtDateInfo(struct x509_crt *x509_crt, time_t *utc_inception,
    time_t *utc_expiration);


int x509CrtVerify(struct x509_crt *x509_crt);


int x509CrtTrackVerification(struct x509_crt *x509_crt,
    struct crt_verif_outcome *cvo);


#endif /* _CRYPTO_THREADSAFE_H_ */
