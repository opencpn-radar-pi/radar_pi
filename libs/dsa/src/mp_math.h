/* LibTomMath, multiple-precision integer library -- Tom St Denis
 *
 * LibTomMath is a library that provides multiple-precision
 * integer arithmetic as well as number theoretic functionality.
 *
 * The library was designed directly after the MPI library by
 * Michael Fromberger but has been written from scratch with
 * additional optimizations in place.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://math.libtomcrypt.com
 */

#ifndef MP_MATH_H
#define MP_MATH_H

#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <ctype.h>

#define XMALLOC  malloc
#define XFREE    free
#define XREALLOC realloc
#define XCALLOC  calloc

#ifdef __cplusplus
/* C++ compilers don't like assigning void * to mp_digit * */
	#define  OPT_CAST(x)  (x *)
#else
	#define  OPT_CAST(x)
#endif

/* detect 64-bit mode if possible */
#if defined(__x86_64__) 
   #if !(defined(MP_64BIT) && defined(MP_16BIT) && defined(MP_8BIT))
      #define MP_64BIT
   #endif
#endif

/* some default configurations.
 *
 * A "mp_digit" must be able to hold DIGIT_BIT + 1 bits
 * A "mp_word" must be able to hold 2*DIGIT_BIT + 1 bits
 *
 * At the very least a mp_digit must be able to hold 7 bits
 * [any size beyond that is ok provided it doesn't overflow the data type]
 */
#ifdef MP_8BIT
   typedef unsigned char      mp_digit;
   typedef unsigned short     mp_word;
#elif defined(MP_16BIT)
   typedef unsigned short     mp_digit;
   typedef unsigned long      mp_word;
#elif defined(MP_64BIT)
   /* for GCC only on supported platforms */
   typedef unsigned long long ulong64;
   typedef signed long long   long64;

   typedef unsigned long      mp_digit;
   typedef unsigned long      mp_word __attribute__ ((mode(TI)));

   #define DIGIT_BIT          60

#else
   /* this is the default case, 28-bit digits */
   
   #if defined(_MSC_VER) || defined(__BORLANDC__) 
      typedef unsigned __int64   ulong64;
      typedef signed __int64     long64;
   #else
      typedef unsigned long long ulong64;
      typedef signed long long   long64;
   #endif

   typedef unsigned long      mp_digit;
   typedef ulong64            mp_word;

   #define DIGIT_BIT          28
   #define MP_28BIT
#endif

#ifndef DIGIT_BIT
   #define DIGIT_BIT     ((int)((CHAR_BIT * sizeof(mp_digit) - 1)))  /* bits per digit */
#endif

#define MP_DIGIT_BIT     DIGIT_BIT
#define MP_MASK          ((((mp_digit)1)<<((mp_digit)DIGIT_BIT))-((mp_digit)1))
#define MP_DIGIT_MAX     MP_MASK

// equalities
#define MP_LT         -1  /* less than */
#define MP_EQ         0   /* equal to */
#define MP_GT         1   /* greater than */

#define MP_ZPOS       0   /* positive integer */
#define MP_NEG        1   /* negative */

#define MP_OKAY       0   /* ok result */
#define MP_MEM        -2  /* out of mem */
#define MP_VAL        -3  /* invalid input */
#define MP_RANGE      MP_VAL

#define MP_YES        1   /* yes response */
#define MP_NO         0   /* no response */

/* Primality generation flags */
#define LTM_PRIME_BBS      0x0001 /* BBS style prime */
#define LTM_PRIME_SAFE     0x0002 /* Safe prime (p-1)/2 == prime */
#define LTM_PRIME_2MSB_ON  0x0008 /* force 2nd MSB to 1 */

typedef int mp_err;

#define MP_PREC		32     /* default digits of precision */

/* size of comba arrays, should be at least 2 * 2**(BITS_PER_WORD - BITS_PER_DIGIT*2) */
#define MP_WARRAY               (1 << (sizeof(mp_word) * CHAR_BIT - 2 * DIGIT_BIT + 1))

/* the infamous mp_int structure */
typedef struct  {
    int used, alloc, sign;
    mp_digit *dp;
} mp_int;

/* callback for mp_prime_random, should fill dst with random bytes and return how many read [upto len] */
typedef int ltm_prime_callback(unsigned char *dst, int len, void *dat);

#define MP_MIN(x,y) ((x)<(y)?(x):(y))
#define MP_MAX(x,y) ((x)>(y)?(x):(y))

#define USED(m)    ((m)->used)
#define DIGIT(m,k) ((m)->dp[(k)])
#define SIGN(m)    ((m)->sign)

// Init and deinit {{{
//int mp_init(mp_int *a);
void mp_clear(mp_int *a);
//int mp_init_multi(mp_int *mp, ...);
//void mp_clear_multi(mp_int *mp, ...);
void mp_exch(mp_int *a, mp_int *b);
//int mp_shrink(mp_int *a);
int mp_grow(mp_int *a, int size);
int mp_init_size(mp_int *a, int size);
// }}}

// Basic Manipulations {{{
#define mp_iszero(a) (((a)->used == 0) ? MP_YES : MP_NO)
#define mp_iseven(a) (((a)->used > 0 && (((a)->dp[0] & 1) == 0)) ? MP_YES : MP_NO)
#define mp_isodd(a)  (((a)->used > 0 && (((a)->dp[0] & 1) == 1)) ? MP_YES : MP_NO)
void mp_zero(mp_int *a);
void mp_set(mp_int *a, mp_digit b);
//int mp_set_int(mp_int *a, unsigned long b);
//unsigned long mp_get_int(mp_int * a);
//int mp_init_set (mp_int * a, mp_digit b);
//int mp_init_set_int (mp_int * a, unsigned long b);
int mp_copy(mp_int *a, mp_int *b);
int mp_init_copy(mp_int *a, mp_int *b);
void mp_clamp(mp_int *a);
// }}}

// Digit manipulation {{{ 
void mp_rshd(mp_int *a, int b);
int mp_lshd(mp_int *a, int b);
int mp_div_2d(mp_int *a, int b, mp_int *c, mp_int *d);
int mp_div_2(mp_int *a, mp_int *b);
int mp_mul_2d(mp_int *a, int b, mp_int *c);
int mp_mul_2(mp_int *a, mp_int *b);
int mp_mod_2d(mp_int *a, int b, mp_int *c);
int mp_2expt(mp_int *a, int b);
//int mp_cnt_lsb(mp_int *a);
//int mp_rand(mp_int *a, int digits);

// Binary operations {{{
//int mp_xor(mp_int *a, mp_int *b, mp_int *c);
//int mp_or(mp_int *a, mp_int *b, mp_int *c);
//int mp_and(mp_int *a, mp_int *b, mp_int *c);
// }}}

// Basic arithmetic {{{
int mp_neg(mp_int *a, mp_int *b);
int mp_abs(mp_int *a, mp_int *b);
//int mp_cmp(mp_int *a, mp_int *b);
int mp_cmp_mag(mp_int *a, mp_int *b);
int mp_add(mp_int *a, mp_int *b, mp_int *c);
int mp_sub(mp_int *a, mp_int *b, mp_int *c);
int mp_mul(mp_int *a, mp_int *b, mp_int *c);
int mp_sqr(mp_int *a, mp_int *b);
int mp_div(mp_int *a, mp_int *b, mp_int *c, mp_int *d);
//int mp_mod(mp_int *a, mp_int *b, mp_int *c);
// }}}

// Single digit functions {{{
int mp_cmp_d(mp_int *a, mp_digit b);
int mp_add_d(mp_int *a, mp_digit b, mp_int *c);
int mp_sub_d(mp_int *a, mp_digit b, mp_int *c);
int mp_mul_d(mp_int *a, mp_digit b, mp_int *c);
int mp_div_d(mp_int *a, mp_digit b, mp_int *c, mp_digit *d);
int mp_div_3(mp_int *a, mp_int *c, mp_digit *d);
//int mp_expt_d(mp_int *a, mp_digit b, mp_int *c);
int mp_mod_d(mp_int *a, mp_digit b, mp_digit *c);
// }}}

// Number theory {{{
//int mp_addmod(mp_int *a, mp_int *b, mp_int *c, mp_int *d);
//int mp_submod(mp_int *a, mp_int *b, mp_int *c, mp_int *d);
//int mp_mulmod(mp_int *a, mp_int *b, mp_int *c, mp_int *d);
//int mp_sqrmod(mp_int *a, mp_int *b, mp_int *c);
//int mp_invmod(mp_int *a, mp_int *b, mp_int *c);
//int mp_gcd(mp_int *a, mp_int *b, mp_int *c);
//int mp_exteuclid(mp_int *a, mp_int *b, mp_int *U1, mp_int *U2, mp_int *U3);
//int mp_lcm(mp_int *a, mp_int *b, mp_int *c);
//int mp_n_root(mp_int *a, mp_digit b, mp_int *c);
//int mp_sqrt(mp_int *arg, mp_int *ret);
//int mp_is_square(mp_int *arg, int *ret);
//int mp_jacobi(mp_int *a, mp_int *n, int *c);
int mp_reduce_setup(mp_int *a, mp_int *b);
int mp_reduce(mp_int *a, mp_int *b, mp_int *c);
int mp_montgomery_setup(mp_int *a, mp_digit *mp);
int mp_montgomery_calc_normalization(mp_int *a, mp_int *b);
int mp_montgomery_reduce(mp_int *a, mp_int *m, mp_digit mp);
int mp_dr_is_modulus(mp_int *a);
void mp_dr_setup(mp_int *a, mp_digit *d);
int mp_dr_reduce(mp_int *a, mp_int *b, mp_digit mp);
int mp_reduce_is_2k(mp_int *a);
int mp_reduce_2k_setup(mp_int *a, mp_digit *d);
int mp_reduce_2k(mp_int *a, mp_int *n, mp_digit d);
int mp_reduce_is_2k_l(mp_int *a);
int mp_reduce_2k_setup_l(mp_int *a, mp_int *d);
int mp_reduce_2k_l(mp_int *a, mp_int *n, mp_int *d);
//int mp_exptmod(mp_int *a, mp_int *b, mp_int *c, mp_int *d);
// }}}

// Primes {{{
//#ifdef MP_8BIT
//   #define PRIME_SIZE      31
//#else
//   #define PRIME_SIZE      256
//#endif
//extern const mp_digit ltm_prime_tab[];
//int mp_prime_is_divisible(mp_int *a, int *result);
//int mp_prime_fermat(mp_int *a, mp_int *b, int *result);
//int mp_prime_miller_rabin(mp_int *a, mp_int *b, int *result);
//int mp_prime_rabin_miller_trials(int size);
//int mp_prime_is_prime(mp_int *a, int t, int *result);
//int mp_prime_next_prime(mp_int *a, int t, int bbs_style);
//#define mp_prime_random(a, t, size, bbs, cb, dat) mp_prime_random_ex(a, t, ((size) * 8) + 1, (bbs==1)?LTM_PRIME_BBS:0, cb, dat)
//int mp_prime_random_ex(mp_int *a, int t, int size, int flags, ltm_prime_callback cb, void *dat);
// }}}

// Radix conversion {{{
int mp_count_bits(mp_int *a);
//int mp_unsigned_bin_size(mp_int *a);
//int mp_read_unsigned_bin(mp_int *a, const unsigned char *b, int c);
//int mp_to_unsigned_bin(mp_int *a, unsigned char *b);
//int mp_to_unsigned_bin_n (mp_int * a, unsigned char *b, unsigned long *outlen);
//int mp_signed_bin_size(mp_int *a);
//int mp_read_signed_bin(mp_int *a, const unsigned char *b, int c);
//int mp_to_signed_bin(mp_int *a,  unsigned char *b);
//int mp_to_signed_bin_n (mp_int * a, unsigned char *b, unsigned long *outlen);
//int mp_read_radix(mp_int *a, const char *str, int radix);
int mp_toradix(mp_int *a, char *str, int radix);
//int mp_toradix_n(mp_int * a, char *str, int radix, int maxlen);
//int mp_radix_size(mp_int *a, int radix, int *size);
//int mp_fread(mp_int *a, int radix, FILE *stream);
//int mp_fwrite(mp_int *a, int radix, FILE *stream);
//#define mp_read_raw(mp, str, len) mp_read_signed_bin((mp), (str), (len))
//#define mp_raw_size(mp)           mp_signed_bin_size(mp)
//#define mp_toraw(mp, str)         mp_to_signed_bin((mp), (str))
//#define mp_read_mag(mp, str, len) mp_read_unsigned_bin((mp), (str), (len))
//#define mp_mag_size(mp)           mp_unsigned_bin_size(mp)
//#define mp_tomag(mp, str)         mp_to_unsigned_bin((mp), (str))
#define mp_tobinary(M, S)  mp_toradix((M), (S), 2)
#define mp_tooctal(M, S)   mp_toradix((M), (S), 8)
#define mp_todecimal(M, S) mp_toradix((M), (S), 10)
#define mp_tohex(M, S)     mp_toradix((M), (S), 16)
// }}}

#ifdef __cplusplus
extern "C" int mp_read_radix(mp_int *a, const char *str, int radix);
extern "C" int mp_init(mp_int *a);
extern "C" int mp_init_multi(mp_int *mp, ...);
extern "C" void mp_clear_multi(mp_int *mp, ...);
extern "C" int mp_cmp(mp_int *a, mp_int *b);
extern "C" int mp_mulmod(mp_int *a, mp_int *b, mp_int *c, mp_int *d);
extern "C" int mp_invmod(mp_int *a, mp_int *b, mp_int *c);
extern "C" int mp_exptmod(mp_int *a, mp_int *b, mp_int *c, mp_int *d);
extern "C" int mp_mod(mp_int *a, mp_int *b, mp_int *c);
extern "C" int mp_read_unsigned_bin(mp_int *a, const unsigned char *b, int c);

#else
int mp_read_radix(mp_int *a, const char *str, int radix);
int mp_init(mp_int *a);
int mp_init_multi(mp_int *mp, ...);
void mp_clear_multi(mp_int *mp, ...);
int mp_cmp(mp_int *a, mp_int *b);
int mp_mulmod(mp_int *a, mp_int *b, mp_int *c, mp_int *d);
int mp_invmod(mp_int *a, mp_int *b, mp_int *c);
int mp_exptmod(mp_int *a, mp_int *b, mp_int *c, mp_int *d);
int mp_mod(mp_int *a, mp_int *b, mp_int *c);
int mp_read_unsigned_bin(mp_int *a, const unsigned char *b, int c);

#endif





// Low level {{{
int s_mp_add(mp_int *a, mp_int *b, mp_int *c);
int s_mp_sub(mp_int *a, mp_int *b, mp_int *c);
#define s_mp_mul(a, b, c) s_mp_mul_digs(a, b, c, (a)->used + (b)->used + 1)
int fast_s_mp_mul_digs(mp_int *a, mp_int *b, mp_int *c, int digs);
int s_mp_mul_digs(mp_int *a, mp_int *b, mp_int *c, int digs);
//int fast_s_mp_mul_high_digs(mp_int *a, mp_int *b, mp_int *c, int digs);
int s_mp_mul_high_digs(mp_int *a, mp_int *b, mp_int *c, int digs);
//int fast_s_mp_sqr(mp_int *a, mp_int *b);
int s_mp_sqr(mp_int *a, mp_int *b);
//int mp_karatsuba_mul(mp_int *a, mp_int *b, mp_int *c);
//int mp_toom_mul(mp_int *a, mp_int *b, mp_int *c);
//int mp_karatsuba_sqr(mp_int *a, mp_int *b);
//int mp_toom_sqr(mp_int *a, mp_int *b);
//int fast_mp_invmod(mp_int *a, mp_int *b, mp_int *c);
//int mp_invmod_slow (mp_int * a, mp_int * b, mp_int * c);
int fast_mp_montgomery_reduce(mp_int *a, mp_int *m, mp_digit mp);
int mp_exptmod_fast(mp_int *G, mp_int *X, mp_int *P, mp_int *Y, int mode);
int s_mp_exptmod (mp_int * G, mp_int * X, mp_int * P, mp_int * Y, int mode);
void bn_reverse(unsigned char *s, int len);	
// }}}

#endif

