#include "decode.h"

#include <errno.h>
#include "common.h"
#include "log.h"

#define COND_LOG(log, pr) (log ? pr : -EINVAL)

/* Decoded BER data */
struct ber_data {
	const void *src;
	size_t src_size;
	size_t consumed;
};

static int
validate(asn_TYPE_descriptor_t const *descriptor, void *result, bool log)
{
	char error_msg[256];
	size_t error_msg_size;
	int error;

	/* The lib's inbuilt validations. (Probably not much.) */
	error_msg_size = sizeof(error_msg);
	error = asn_check_constraints(descriptor, result, error_msg,
	    &error_msg_size);
	if (error == -1)
		return COND_LOG(log,
		    pr_err("Error validating ASN.1 object: %s", error_msg));

	return 0;
}

static void
der_debug(size_t size, const void *der, const void *orig, size_t consumed)
{
	size_t i;

	for (i = 0; i < size; i++)
		if (((uint8_t *)der)[i] != ((uint8_t *)orig)[consumed + i]) {
			pr_debug("Diff starts at byte %ld: BER '%02x', DER '%02x'",
			    consumed + i,
			    ((uint8_t *)der)[i],
			    ((uint8_t *)orig)[consumed + i]);
			return;
		}
}

static int
der_coder(const void *buf, size_t size, void *app_key)
{
	struct ber_data *data = app_key;

	if (data->consumed + size > data->src_size) {
		pr_debug("DER encoding will consume more bytes than expected (expected %lu, will get %lu)",
		    data->consumed + size, data->src_size);
		return -1;
	}

	if (memcmp(data->src + data->consumed, buf, size) != 0) {
		if (log_debug_enabled())
			der_debug(size, buf, data->src, data->consumed);
		return -1;
	}
	data->consumed += size;
	return 0;
}

static int
validate_der(size_t ber_consumed, asn_TYPE_descriptor_t const *descriptor,
    const void *original, void *result)
{
	struct ber_data data;
	asn_enc_rval_t eval;

	data.src = original;
	data.src_size = ber_consumed;
	data.consumed = 0;

	eval = der_encode(descriptor, result, der_coder, &data);
	if (eval.encoded == -1)
		return incidence(INID_OBJ_NOT_DER,
		    "'%s' isn't DER encoded", eval.failed_type->name);

	if (ber_consumed != eval.encoded) {
		pr_debug("DER encoding consumed less bytes than expected (expected %lu, got %lu)",
		    ber_consumed, eval.encoded);
		return incidence(INID_OBJ_NOT_DER, "'%s' isn't DER encoded",
		    descriptor->name);
	}

	return 0;
}

int
asn1_decode(const void *buffer, size_t buffer_size,
    asn_TYPE_descriptor_t const *descriptor, void **result, bool log,
    bool dec_as_der)
{
	asn_dec_rval_t rval;
	int error;

	*result = NULL;

	/* TODO (next iteration) first argument is more or less important. */
	rval = ber_decode(0, descriptor, result, buffer, buffer_size);
	if (rval.code != RC_OK) {
		/* Must free partial object according to API contracts. */
		ASN_STRUCT_FREE(*descriptor, *result);
		/* We expect the data to be complete; RC_WMORE is an error. */
		return COND_LOG(log,
		    pr_err("Error '%u' decoding ASN.1 object around byte %zu",
		        rval.code, rval.consumed));
	}

	/* Validate DER encoding */
	if (dec_as_der) {
		error = validate_der(rval.consumed, descriptor, buffer,
		    *result);
		if (error) {
			ASN_STRUCT_FREE(*descriptor, *result);
			return error;
		}
	}

	error = validate(descriptor, *result, log);
	if (error) {
		ASN_STRUCT_FREE(*descriptor, *result);
		return error;
	}

	return 0;
}

int
asn1_decode_any(ANY_t *any, asn_TYPE_descriptor_t const *descriptor,
    void **result, bool log, bool dec_as_der)
{
	return asn1_decode(any->buf, any->size, descriptor, result, log,
	    dec_as_der);
}

int
asn1_decode_octet_string(OCTET_STRING_t *string,
    asn_TYPE_descriptor_t const *descriptor, void **result, bool log,
    bool dec_as_der)
{
	return asn1_decode(string->buf, string->size, descriptor, result, log,
	    dec_as_der);
}

/*
 * TODO (next iteration) There's no need to load the entire file into memory.
 * ber_decode() can take an incomplete buffer, in which case it returns
 * RC_WMORE.
 */
int
asn1_decode_fc(struct file_contents *fc,
    asn_TYPE_descriptor_t const *descriptor, void **result, bool log,
    bool dec_as_der)
{
	return asn1_decode(fc->buffer, fc->buffer_size, descriptor, result,
	    log, dec_as_der);
}
