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

#include <stddef.h> /* NULL */

#include "src/datovka_shared/crypto/crypto_trusted_certs.h"

static const char postsignum_qca_root_file[] = "postsignum_qca_root.pem";
static const char postsignum_qca_root_name[] = "PostSignum Root QCA";
static const char postsignum_qca_root_pem[] =
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

static const char postsignum_qca_sub_file[] = "postsignum_qca_sub.pem";
static const char postsignum_qca_sub_name[] = "PostSignum Qualified CA";
static const char postsignum_qca_sub_pem[] =
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

static const char postsignum_qca2_root_file[] = "postsignum_qca2_root.pem";
static const char postsignum_qca2_root_name[] = "PostSignum Root QCA 2";
static const char postsignum_qca2_root_pem[] =
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

static const char postsignum_qca2_sub_file[] = "postsignum_qca2_sub.pem";
static const char postsignum_qca2_sub_name[] = "PostSignum Qualified CA 2";
static const char postsignum_qca2_sub_pem[] =
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

static const char postsignum_qca3_sub_file[] = "postsignum_qca3_sub.pem";
static const char postsignum_qca3_sub_name[] = "PostSignum Qualified CA 3";
static const char postsignum_qca3_sub_pem[] =
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

#if 0
static const char equifax_ca_file[] = "equifax_ca.pem";
static const char equifax_ca_name[] = "Equifax Secure Certificate Authority";
static const char equifax_ca_pem[] =
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
#endif

static const char digicert_ca_root_file[] = "digicert_global_root_g2.pem";
static const char digicert_ca_root_name[] = "DigiCert Global Root G2";
static const char digicert_ca_root_pem[] =
"-----BEGIN CERTIFICATE-----""\n"
"MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh""\n"
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3""\n"
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH""\n"
"MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT""\n"
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j""\n"
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG""\n"
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI""\n"
"2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx""\n"
"1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ""\n"
"q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz""\n"
"tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ""\n"
"vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP""\n"
"BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV""\n"
"5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY""\n"
"1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4""\n"
"NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG""\n"
"Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91""\n"
"8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe""\n"
"pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl""\n"
"MrY=""\n"
"-----END CERTIFICATE-----";

#if 0
static const char all_certs_file[] = "all_trusted.pem";
#endif

/*!
 * @brief Holds NULL-terminated list of PEM encoded certificate files.
 *
 * @note In C file names may be string literals or 'cost char str[]'.
 * C++ allows 'const char *str'.
 */
const char *pem_files[] = {
	NULL, /* Don't use this list. */
	postsignum_qca_root_file,
	postsignum_qca_sub_file,
	postsignum_qca2_root_file,
	postsignum_qca2_sub_file,
	postsignum_qca3_sub_file,
//	equifax_ca_file,
	digicert_ca_root_file,
//	NULL,
//	all_certs_file,
	NULL
};

/*
 * Holds NULL-terminated list of PEM encoded certificates.
 */
const struct pem_str all_pem_strs[] = {
	{postsignum_qca_root_name, postsignum_qca_root_pem},
	{postsignum_qca_sub_name, postsignum_qca_sub_pem},
	{postsignum_qca2_root_name, postsignum_qca2_root_pem},
	{postsignum_qca2_sub_name, postsignum_qca2_sub_pem},
	{postsignum_qca3_sub_name, postsignum_qca3_sub_pem},
//	{equifax_ca_name, equifax_ca_pem},
	{digicert_ca_root_name, digicert_ca_root_pem},
	{NULL, NULL}
};

/*
 * Holds NULL-terminated list of PEM encoded root certificates.
 */
const struct pem_str msg_pem_strs[] = {
	{postsignum_qca_root_name, postsignum_qca_root_pem},
	{postsignum_qca2_root_name, postsignum_qca2_root_pem},
	{NULL, NULL}
};

const struct pem_str conn_pem_strs[] = {
	//	{equifax_ca_name, equifax_ca_pem},
	{digicert_ca_root_name, digicert_ca_root_pem},
	{NULL, NULL}
};

static const char psrootqca_file[] = "psrootqca.crl";
static const char *psrootqca_urls[] = {
	"http://www.postsignum.cz/crl/psrootqca.crl",
	"http://postsignum.ttc.cz/crl/psrootqca.crl",
	NULL
};

static const char psrootqca2_file[] = "psrootqca2.crl";
static const char *psrootqca2_urls[] = {
	"http://www.postsignum.cz/crl/psrootqca2.crl",
	"http://www2.postsignum.cz/crl/psrootqca2.crl",
	"http://postsignum.ttc.cz/crl/psrootqca2.crl",
	NULL
};

/*
 * NULL-terminated list of CRL files.
 */
const struct crl_location crl_locations[] = {
	{psrootqca_file, psrootqca_urls},
	{psrootqca2_file, psrootqca2_urls},
	{NULL, NULL}
};
