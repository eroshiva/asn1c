#ifndef	ASN1PRINT_H
#define	ASN1PRINT_H

enum asn1print_flags {
	APF_NOFLAGS,
	APF_NOINDENT		= 0x01,	/* Disable indentation */
	APF_LINE_COMMENTS	= 0x02, /* Include line comments */
	APF_PRINT_XML_DTD	= 0x04,	/* Generate XML DTD */
	APF_PRINT_CONSTRAINTS	= 0x08,	/* Explain constraints */
	APF_PRINT_CLASS_MATRIX	= 0x10,	/* Dump class matrix */
	APF_PRINT_PROTOBUF	= 0x20,	/* Generate Protobuf */
};

/*
 * Print the contents of the parsed ASN.1 syntax tree.
 */
int asn1print(asn1p_t *asn, enum asn1print_flags flags);
int asn1print_ref(const asn1p_ref_t *ref, enum asn1print_flags flags);
int asn1print_value(const asn1p_value_t *val, enum asn1print_flags flags);
int asn1print_constraint(const asn1p_constraint_t *, enum asn1print_flags);
int asn1print_expr(asn1p_t *asn, asn1p_module_t *mod, asn1p_expr_t *tc, enum asn1print_flags flags, int level);

const char *asn1p_constraint_string(const asn1p_constraint_t *ct);

#endif	/* ASN1PRINT_H */
