#ifndef SRC_OBJECT_ROA_H_
#define SRC_OBJECT_ROA_H_

#include <openssl/x509.h>
#include "uri.h"

int handle_roa(struct rpki_uri const *uri, STACK_OF(X509_CRL) *);

#endif /* SRC_OBJECT_ROA_H_ */
