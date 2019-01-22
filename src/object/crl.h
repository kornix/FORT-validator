#ifndef SRC_OBJECT_CRL_H_
#define SRC_OBJECT_CRL_H_

#include <openssl/x509.h>
#include "uri.h"

int crl_load(struct rpki_uri const *uri, X509_CRL **);

#endif /* SRC_OBJECT_CRL_H_ */
