/*
 * Copyright (C) 2014-2016 CZ.NIC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
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

#include <cstdlib>
#include <QtTest/QtTest>

#include "src/crypto/crypto.h"
#include "src/crypto/crypto_funcs.h"
#include "src/log/log.h"
#include "tests/helper.h"
#include "tests/test_crypto.h"

class DerFileContent {
public:
	explicit DerFileContent(const QString &path);
	~DerFileContent(void);

	void loadContent(void);
	void unloadContent(void);

	const QString fPath; /*!< Path to file. */
	char *der; /*!< File content. */
	size_t der_size; /*!< Size of data portion. */
	struct x509_crt *x509_crt; /*!< Signing certificate. */
};

DerFileContent::DerFileContent(const QString &path)
    : fPath(path),
    der(NULL),
    der_size(0),
    x509_crt(NULL)
{
}

DerFileContent::~DerFileContent(void)
{
	unloadContent();
}

void DerFileContent::loadContent(void)
{
	der = internal_read_file(fPath.toUtf8().constData(), &der_size, "r");
	x509_crt = raw_cms_signing_cert(der, der_size);
}

void DerFileContent::unloadContent(void)
{
	free(der); der = NULL; der_size = 0;
	if (x509_crt != NULL) {
		x509_crt_destroy(x509_crt); x509_crt = NULL;
	}
}

class TestCrypto: public QObject {
	Q_OBJECT

public:
	TestCrypto(void);

private slots:
	void initTestCase(void);

	void cleanupTestCase(void);

	void x509CrtVerify(void);

	void verifySignature01(void);

	void cryptoAddCrl01(void);

	void verifySignature02(void);

	void cryptoAddCrl02(void);

	void crtAlgorithmInfo(void);

	void crtDateInfo(void);

	void rawTstVerify(void);

	void p12ToPem(void);

private:
	void verifySignature(void);

	void cryptoAddCrl(bool expectFail);

	DerFileContent ddz; /*!< Data message. */
	DerFileContent bin; /*!< Binary data message. */
	DerFileContent tst; /*!< Time stamp. */

	const QString p12FilePath; /*!< Unencrypted PKCS 12 file. */
	const QString p12EncryptedFilePath; /* !< Encrypted PKCS 12 file. */
	const QString pwd; /*!< Password for encrypted PKCS 12 file. */

	const QString crl1FilePath; /*!< Path to certificate revocation list. */
	const QString crl2FilePath; /*!< Path to certificate revocation list. */
};

TestCrypto::TestCrypto(void)
    : ddz("data/DDZ_3401761.zfo"),
    bin("data/message_binary.dat"),
    tst("data/timestamp_binary.dat"),
    p12FilePath("data/cert_and_key.p12"),
    p12EncryptedFilePath("data/cert_and_key_encrypted.p12"),
    pwd("Abcd0"),
    crl1FilePath("../trusted_certs/crl/psrootqca.crl"),
    crl2FilePath("../trusted_certs/crl/psrootqca2.crl")
{
}

void TestCrypto::initTestCase(void)
{
	/* Initialise cryptographic context. */
	int ret = crypto_init();
	QVERIFY(ret == 0);

	ddz.loadContent();
	QVERIFY(ddz.der != NULL && ddz.der_size > 0);
	QVERIFY(ddz.x509_crt != NULL);

	bin.loadContent();
	QVERIFY(bin.der != NULL && bin.der_size > 0);
	QVERIFY(bin.x509_crt != NULL);

	tst.loadContent();
	QVERIFY(tst.der != NULL && tst.der_size > 0);
	QVERIFY(tst.x509_crt != NULL);
}

void TestCrypto::cleanupTestCase(void)
{
	ddz.unloadContent();
	bin.unloadContent();
	tst.unloadContent();
}

void TestCrypto::x509CrtVerify(void)
{
	int ret;

	ret = x509_crt_verify(ddz.x509_crt);
	QVERIFY2(ret == 0, "Got invalid result at 1st attempt. Expected 0.");

	ret = x509_crt_verify(ddz.x509_crt);
	QVERIFY2(ret == 0, "Got invalid result at 2nd attempt. Expected 0.");

	ret = x509_crt_verify(ddz.x509_crt);
	QVERIFY2(ret == 0, "Got invalid result at 3rd attempt. Expected 0.");

	ret = x509_crt_verify(bin.x509_crt);
	QVERIFY2(ret == 0, "Got invalid result at 4th attempt. Expected 0.");

	ret = x509_crt_verify(bin.x509_crt);
	QVERIFY2(ret == 0, "Got invalid result at 5th attempt. Expected 0.");

	ret = x509_crt_verify(bin.x509_crt);
	QVERIFY2(ret == 0, "Got invalid result at 6th attempt. Expected 0.");

	ret = x509_crt_verify(tst.x509_crt);
	QVERIFY2(ret == 1, "Got invalid result at 7th attempt. Expected 1.");

	ret = x509_crt_verify(tst.x509_crt);
	QVERIFY2(ret == 1, "Got invalid result at 8th attempt. Expected 1.");

	ret = x509_crt_verify(tst.x509_crt);
	QVERIFY2(ret == 1, "Got invalid result at 9th attempt. Expected 1.");
}

void TestCrypto::verifySignature01(void)
{
	verifySignature();
}

void TestCrypto::cryptoAddCrl01(void)
{
	cryptoAddCrl(false);
}

void TestCrypto::verifySignature02(void)
{
	verifySignature();
}

void TestCrypto::cryptoAddCrl02(void)
{
	cryptoAddCrl(true);
}

void TestCrypto::crtAlgorithmInfo(void)
{
	char *sa_id = NULL, *sa_name = NULL;
	int ret;

	ret = x509_crt_algorithm_info(ddz.x509_crt, &sa_id, &sa_name);
	QVERIFY2(ret == 0, "Could not obtain signature algorithm details.");
	QVERIFY2(sa_id != NULL &&
	         sa_id == QStringLiteral("1.2.840.113549.1.1.11"),
	         "Could not obtain signature algorithm identifier string.");
	QVERIFY2(sa_name != NULL &&
	         sa_name == QStringLiteral("sha256WithRSAEncryption"),
	         "Could not obtain signature algorithm name.");
	free(sa_id); sa_id = NULL;
	free(sa_name); sa_name = NULL;

	ret = x509_crt_algorithm_info(bin.x509_crt, &sa_id, &sa_name);
	QVERIFY2(ret == 0, "Could not obtain signature algorithm details.");
	QVERIFY2(sa_id != NULL &&
	         sa_id == QStringLiteral("1.2.840.113549.1.1.11"),
	         "Could not obtain signature algorithm identifier string.");
	QVERIFY2(sa_name != NULL &&
	         sa_name == QStringLiteral("sha256WithRSAEncryption"),
	         "Could not obtain signature algorithm name.");

	free(sa_id); sa_id = NULL;
	free(sa_name); sa_name = NULL;

	ret = x509_crt_algorithm_info(tst.x509_crt, &sa_id, &sa_name);
	QVERIFY2(ret == 0, "Could not obtain signature algorithm details.");
	QVERIFY2(sa_id != NULL &&
	         sa_id == QStringLiteral("1.2.840.113549.1.1.11"),
	         "Could not obtain signature algorithm identifier string.");
	QVERIFY2(sa_name != NULL &&
	         sa_name == QStringLiteral("sha256WithRSAEncryption"),
	         "Could not obtain signature algorithm name.");

	free(sa_id); sa_id = NULL;
	free(sa_name); sa_name = NULL;
}

void TestCrypto::crtDateInfo(void)
{
	time_t inception = 0, expiration = 0;
	int ret;

	ret = x509_crt_date_info(ddz.x509_crt, &inception, &expiration);
	QVERIFY2(ret == 0, "Could not obtain date details.");
	QVERIFY2(inception == 1412923874, "Got wrong inception time.");
	QVERIFY2(expiration == 1446187874, "Got wrong expiration time.");

	ret = x509_crt_date_info(bin.x509_crt, &inception, &expiration);
	QVERIFY2(ret == 0, "Could not obtain date details.");
	QVERIFY2(inception == 1412923874, "Got wrong inception time.");
	QVERIFY2(expiration == 1446187874, "Got wrong expiration time.");

	ret = x509_crt_date_info(tst.x509_crt, &inception, &expiration);
	QVERIFY2(ret == 0, "Could not obtain date details.");
	QVERIFY2(inception == 1377511700, "Got wrong inception time.");
	QVERIFY2(expiration == 1570183700, "Got wrong expiration time.");
}

void TestCrypto::rawTstVerify(void)
{
	time_t timestamp;
	int ret;

	ret = raw_tst_verify(tst.der, tst.der_size, &timestamp);
	QVERIFY2(ret == 1, "Time stamp signature verification failed.");
}

void TestCrypto::p12ToPem(void)
{
	void *p12_enc = NULL;
	size_t p12_enc_size = 0;
	void *pem_enc = NULL;
	size_t pem_enc_size = 0;

	void *p12 = NULL;
	size_t p12_size = 0;
	void *pem = NULL;
	size_t pem_size = 0;

	int ret;

	p12_enc = internal_read_file(p12EncryptedFilePath.toUtf8().constData(),
	    &p12_enc_size, "r");
	QVERIFY(p12_enc != NULL && p12_enc_size > 0);

	ret = p12_to_pem(p12_enc, p12_enc_size, pwd.toUtf8().constData(),
	    &pem_enc, &pem_enc_size);
	QVERIFY(ret == 0);
	QVERIFY(pem_enc != NULL && pem_enc_size > 0);
	free(p12_enc); p12_enc = NULL;

//	fprintf(stderr, "XXX %s\n", pem_enc);

	/*
	 * In order to check whether the private key has been correctly
	 * decrypted compare the output of these commands:
	 *
	 * openssl pkcs12 -in data/cert_and_key_encrypted.p12 -nocerts -nodes
	 * ./test_7 2>&1 | openssl pkey
	 */

	free(pem_enc); pem_enc = NULL;

	p12 = internal_read_file(p12FilePath.toUtf8().constData(),
	    &p12_size, "r");
	QVERIFY(p12 != NULL && p12_size > 0);

	ret = p12_to_pem(p12, p12_size, "", &pem, &pem_size);
	QVERIFY(ret == 0);
	QVERIFY(pem != NULL && pem_size > 0);

	free(p12); p12 = NULL;

//	fprintf(stderr, "XXX %s\n", pem);

	free(pem); pem = NULL;
}

void TestCrypto::verifySignature(void)
{
	int ret;

	ret = raw_msg_verify_signature(ddz.der, ddz.der_size, 0, 0);
	QVERIFY2(ret == 1, "Expected valid signature result.");

	ret = raw_msg_verify_signature(ddz.der, ddz.der_size, 1, 0);
	QVERIFY2(ret == 0, "Expected invalid signature result.");

	ret = raw_msg_verify_signature(ddz.der, ddz.der_size, 0, 1);
	QVERIFY2(ret == 1, "Expected valid signature result.");

	ret = raw_msg_verify_signature(ddz.der, ddz.der_size, 1, 1);
	QVERIFY2(ret == 0, "Expected invalid signature result.");

	ret = raw_msg_verify_signature(bin.der, bin.der_size, 0, 0);
	QVERIFY2(ret == 1, "Expected valid signature result.");

	ret = raw_msg_verify_signature(bin.der, bin.der_size, 1, 0);
	QVERIFY2(ret == 0, "Expected invalid signature result.");

	ret = raw_msg_verify_signature(bin.der, bin.der_size, 0, 1);
	QVERIFY2(ret == 1, "Expected valid signature result.");

	ret = raw_msg_verify_signature(bin.der, bin.der_size, 1, 1);
	QVERIFY2(ret == 0, "Expected invalid signature result.");

	ret = raw_msg_verify_signature(tst.der, tst.der_size, 0, 0);
	QVERIFY2(ret == 1, "Expected valid signature result.");

	ret = raw_msg_verify_signature(tst.der, tst.der_size, 1, 0);
	QVERIFY2(ret == 0, "Expected invalid signature result.");

	ret = raw_msg_verify_signature(tst.der, tst.der_size, 0, 1);
	QVERIFY2(ret == 1, "Expected valid signature result.");

	ret = raw_msg_verify_signature(tst.der, tst.der_size, 1, 1);
	QVERIFY2(ret == 0, "Expected invalid signature result.");
}

void TestCrypto::cryptoAddCrl(bool expectFail)
{
	char *der = NULL;
	size_t der_size = 0;
	int ret;

	der = internal_read_file(crl1FilePath.toUtf8().constData(),
	    &der_size, "r");
	QVERIFY(der != NULL && der_size > 0);
	ret = crypto_add_crl(der, der_size);
	if (expectFail) {
		QEXPECT_FAIL("",
		    "This CRL file was probably added in previous test.",
		    Continue);
	}
	QVERIFY(ret == 0);
	free(der); der = NULL;

	der = internal_read_file(crl2FilePath.toUtf8().constData(),
	    &der_size, "r");
	QVERIFY(der != NULL && der_size > 0);
	ret = crypto_add_crl(der, der_size);
	if (expectFail) {
		QEXPECT_FAIL("",
		    "This CRL file was probably added in previous test.",
		    Continue);
	}
	QVERIFY(ret == 0);
	free(der); der = NULL;
}

QObject *newTestCrypto(void)
{
	return new (std::nothrow) TestCrypto();
}

//QTEST_MAIN(TestCrypto)
#include "test_crypto.moc"
