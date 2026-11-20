/*
 * Minimal libcrypto.so.1.1 compatibility shim for the proprietary ELAN TOD
 * fingerprint plugin. Exports only the OPENSSL_1_1_0 symbols it imports and
 * forwards them to the system OpenSSL (libcrypto.so.4).
 */
typedef struct evp_cipher_st EVP_CIPHER;
typedef struct evp_cipher_ctx_st EVP_CIPHER_CTX;
typedef struct evp_md_st EVP_MD;
typedef struct engine_st ENGINE;

#define REAL(ret, name, args) \
	ret real_##name args; \
	__asm__(".symver real_" #name "," #name "@OPENSSL_4.0.0");

REAL(const EVP_CIPHER *, EVP_aes_256_cbc, (void))
REAL(const EVP_MD *, EVP_sha256, (void))
REAL(int, EVP_BytesToKey, (const EVP_CIPHER *, const EVP_MD *, const unsigned char *,
			   const unsigned char *, int, int, unsigned char *, unsigned char *))
REAL(int, EVP_CIPHER_get_block_size, (const EVP_CIPHER *))
REAL(EVP_CIPHER_CTX *, EVP_CIPHER_CTX_new, (void))
REAL(void, EVP_CIPHER_CTX_free, (EVP_CIPHER_CTX *))
REAL(int, EVP_CipherInit_ex, (EVP_CIPHER_CTX *, const EVP_CIPHER *, ENGINE *,
			      const unsigned char *, const unsigned char *, int))
REAL(int, EVP_CipherUpdate, (EVP_CIPHER_CTX *, unsigned char *, int *, const unsigned char *, int))
REAL(int, EVP_CipherFinal_ex, (EVP_CIPHER_CTX *, unsigned char *, int *))

const EVP_CIPHER *EVP_aes_256_cbc(void) { return real_EVP_aes_256_cbc(); }
const EVP_MD *EVP_sha256(void) { return real_EVP_sha256(); }

int EVP_BytesToKey(const EVP_CIPHER *type, const EVP_MD *md, const unsigned char *salt,
		   const unsigned char *data, int datal, int count,
		   unsigned char *key, unsigned char *iv)
{
	return real_EVP_BytesToKey(type, md, salt, data, datal, count, key, iv);
}

int EVP_CIPHER_block_size(const EVP_CIPHER *cipher) { return real_EVP_CIPHER_get_block_size(cipher); }
EVP_CIPHER_CTX *EVP_CIPHER_CTX_new(void) { return real_EVP_CIPHER_CTX_new(); }
void EVP_CIPHER_CTX_free(EVP_CIPHER_CTX *ctx) { real_EVP_CIPHER_CTX_free(ctx); }

int EVP_CipherInit_ex(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *cipher, ENGINE *impl,
		      const unsigned char *key, const unsigned char *iv, int enc)
{
	if (impl)	/* engines no longer exist in OpenSSL 4 */
		return 0;
	return real_EVP_CipherInit_ex(ctx, cipher, 0, key, iv, enc);
}

int EVP_CipherUpdate(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl,
		     const unsigned char *in, int inl)
{
	return real_EVP_CipherUpdate(ctx, out, outl, in, inl);
}

int EVP_CipherFinal_ex(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl)
{
	return real_EVP_CipherFinal_ex(ctx, out, outl);
}
