#ifndef SODIUM_H
#define SODIUM_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
#define crypto_secretbox_KEYBYTES 32
#define crypto_secretbox_NONCEBYTES 24
#define crypto_secretbox_MACBYTES 16
int crypto_secretbox_easy(unsigned char *c,const unsigned char *m,unsigned long long mlen,const unsigned char *n,const unsigned char *k);
int crypto_secretbox_open_easy(unsigned char *m,const unsigned char *c,unsigned long long clen,const unsigned char *n,const unsigned char *k);
#ifdef __cplusplus
}
#endif
#endif
