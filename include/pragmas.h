// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)


#ifndef __pragmas_h__
#define __pragmas_h__

#ifdef __cplusplus
extern "C" {
#endif

extern int dmval;

#if defined(__GNUC__) && defined(__i386__) && USE_ASM

//
// GCC Inline Assembler version
//

//{{{

// maybe one day I'll make these into macros
int boundmulscale(int a, int b, int c);
void clearbufbyte(void *D, int c, int a);
void copybufbyte(void *S, void *D, int c);
void copybufreverse(void *S, void *D, int c);


#define sqr(a) \
	({ int __a=(a); \
	   __asm__ __volatile__ ("imull %0, %0" \
		: "=q" (__a) \
		: "0" (__a) \
		: "cc"); \
	 __a; })

#define scale(a,d,c) \
	({ int __a=(a), __d=(d), __c=(c); \
	   __asm__ __volatile__ ("imull %%edx; idivl %%ecx" \
		: "=a" (__a), "=d" (__d) \
		: "0" (__a), "1" (__d), "c" (__c) : "cc"); \
	 __a; })

#define mulscale(a,d,c) \
	({ int __a=(a), __d=(d), __c=(c); \
	   __asm__ __volatile__ ("imull %%edx; shrdl %%cl, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d), "c" (__c) : "cc"); \
	 __a; })
#define mulscale1(a,d) \
	({ int __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $1, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale2(a,d) \
	({ int __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $2, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale3(a,d) \
	({ int __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $3, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale4(a,d) \
	({ int __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $4, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale5(a,d) \
	({ int __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $5, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale6(a,d) \
	({ int __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $6, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale7(a,d) \
	({ int __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $7, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale8(a,d) \
	({ int __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $8, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale9(a,d) \
	({ int __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $9, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale10(a,d) \
	({ int __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $10, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale11(a,d) \
	({ int __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $11, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale12(a,d) \
	({ int __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $12, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale13(a,d) \
	({ int __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $13, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale14(a,d) \
	({ int __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $14, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale15(a,d) \
	({ int __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $15, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale16(a,d) \
	({ int __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $16, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale17(a,d) \
	({ int __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $17, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale18(a,d) \
	({ int __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $18, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale19(a,d) \
	({ int __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $19, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale20(a,d) \
	({ int __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $20, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale21(a,d) \
	({ int __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $21, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale22(a,d) \
	({ int __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $22, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale23(a,d) \
	({ int __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $23, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale24(a,d) \
	({ int __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $24, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale25(a,d) \
	({ int __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $25, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale26(a,d) \
	({ int __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $26, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale27(a,d) \
	({ int __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $27, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale28(a,d) \
	({ int __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $28, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale29(a,d) \
	({ int __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $29, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale30(a,d) \
	({ int __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $30, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale31(a,d) \
	({ int __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $31, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale32(a,d) \
	({ int __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __d; })

#define dmulscale(a,d,S,D,c) \
	({ int __a=(a), __d=(d), __S=(S), __D=(D), __c=(c); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl %%cl, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D), "c" (__c) : "ebx", "cc"); \
	 __a; })
#define dmulscale1(a,d,S,D) \
	({ int __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $1, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale2(a,d,S,D) \
	({ int __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $2, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale3(a,d,S,D) \
	({ int __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $3, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale4(a,d,S,D) \
	({ int __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $4, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale5(a,d,S,D) \
	({ int __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $5, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale6(a,d,S,D) \
	({ int __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $6, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale7(a,d,S,D) \
	({ int __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $7, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale8(a,d,S,D) \
	({ int __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $8, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale9(a,d,S,D) \
	({ int __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $9, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale10(a,d,S,D) \
	({ int __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $10, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale11(a,d,S,D) \
	({ int __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $11, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale12(a,d,S,D) \
	({ int __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $12, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale13(a,d,S,D) \
	({ int __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $13, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale14(a,d,S,D) \
	({ int __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $14, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale15(a,d,S,D) \
	({ int __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $15, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale16(a,d,S,D) \
	({ int __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $16, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale17(a,d,S,D) \
	({ int __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $17, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale18(a,d,S,D) \
	({ int __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $18, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale19(a,d,S,D) \
	({ int __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $19, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale20(a,d,S,D) \
	({ int __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $20, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale21(a,d,S,D) \
	({ int __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $21, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale22(a,d,S,D) \
	({ int __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $22, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale23(a,d,S,D) \
	({ int __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $23, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale24(a,d,S,D) \
	({ int __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $24, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale25(a,d,S,D) \
	({ int __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $25, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale26(a,d,S,D) \
	({ int __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $26, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale27(a,d,S,D) \
	({ int __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $27, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale28(a,d,S,D) \
	({ int __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $28, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale29(a,d,S,D) \
	({ int __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $29, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale30(a,d,S,D) \
	({ int __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $30, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale31(a,d,S,D) \
	({ int __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $31, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale32(a,d,S,D) \
	({ int __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __d; })

#define tmulscale1(a,d,b,c,S,D) \
	({ int __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $1, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale2(a,d,b,c,S,D) \
	({ int __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $2, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale3(a,d,b,c,S,D) \
	({ int __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $3, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale4(a,d,b,c,S,D) \
	({ int __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $4, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale5(a,d,b,c,S,D) \
	({ int __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $5, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale6(a,d,b,c,S,D) \
	({ int __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $6, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale7(a,d,b,c,S,D) \
	({ int __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $7, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale8(a,d,b,c,S,D) \
	({ int __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $8, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale9(a,d,b,c,S,D) \
	({ int __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $9, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale10(a,d,b,c,S,D) \
	({ int __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $10, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale11(a,d,b,c,S,D) \
	({ int __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $11, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale12(a,d,b,c,S,D) \
	({ int __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $12, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale13(a,d,b,c,S,D) \
	({ int __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $13, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale14(a,d,b,c,S,D) \
	({ int __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $14, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale15(a,d,b,c,S,D) \
	({ int __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $15, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale16(a,d,b,c,S,D) \
	({ int __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $16, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale17(a,d,b,c,S,D) \
	({ int __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $17, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale18(a,d,b,c,S,D) \
	({ int __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $18, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale19(a,d,b,c,S,D) \
	({ int __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $19, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale20(a,d,b,c,S,D) \
	({ int __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $20, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale21(a,d,b,c,S,D) \
	({ int __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $21, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale22(a,d,b,c,S,D) \
	({ int __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $22, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale23(a,d,b,c,S,D) \
	({ int __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $23, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale24(a,d,b,c,S,D) \
	({ int __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $24, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale25(a,d,b,c,S,D) \
	({ int __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $25, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale26(a,d,b,c,S,D) \
	({ int __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $26, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale27(a,d,b,c,S,D) \
	({ int __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $27, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale28(a,d,b,c,S,D) \
	({ int __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $28, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale29(a,d,b,c,S,D) \
	({ int __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $29, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale30(a,d,b,c,S,D) \
	({ int __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $30, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale31(a,d,b,c,S,D) \
	({ int __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $31, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale32(a,d,b,c,S,D) \
	({ int __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __d; })

#define divscale(a,b,c) \
	({ int __a=(a), __b=(b), __c=(c); \
	   __asm__ __volatile__ ("movl %%eax, %%edx; shll %%cl, %%eax; negb %%cl; sarl %%cl, %%edx; idivl %%ebx" \
		: "=a" (__a) : "a" (__a), "c" (__c), "b" (__b) : "edx", "cc"); \
	 __a; })
#define divscale1(a,b) \
	({ int __a=(a), __b=(b); \
	   __asm__ __volatile__ ("addl %%eax, %%eax; sbbl %%edx, %%edx; idivl %%ebx" \
		: "=a" (__a) : "a" (__a), "b" (__b) : "edx", "cc"); \
	 __a; })
#define divscale2(a,b) \
	({ int __a=(a), __b=(b); \
	   __asm__ __volatile__ ("movl %%eax, %%edx; sarl $30, %%edx; leal (,%%eax,4), %%eax; idivl %%ebx" \
		: "=a" (__a) : "a" (__a), "b" (__b) : "edx", "cc"); \
	 __a; })
#define divscale3(a,b) \
	({ int __a=(a), __b=(b); \
	   __asm__ __volatile__ ("movl %%eax, %%edx; sarl $29, %%edx; leal (,%%eax,8), %%eax; idivl %%ebx" \
		: "=a" (__a) : "a" (__a), "b" (__b) : "edx", "cc"); \
	 __a; })
#define divscale4(a,b) \
	({ int __a=(a), __b=(b); \
	   __asm__ __volatile__ ("movl %%eax, %%edx; sarl $28, %%edx; shll $4, %%eax; idivl %%ebx" \
		: "=a" (__a) : "a" (__a), "b" (__b) : "edx", "cc"); \
	 __a; })
#define divscale5(a,b) \
	({ int __a=(a), __b=(b); \
	   __asm__ __volatile__ ("movl %%eax, %%edx; sarl $27, %%edx; shll $5, %%eax; idivl %%ebx" \
		: "=a" (__a) : "a" (__a), "b" (__b) : "edx", "cc"); \
	 __a; })
#define divscale6(a,b) \
	({ int __a=(a), __b=(b); \
	   __asm__ __volatile__ ("movl %%eax, %%edx; sarl $26, %%edx; shll $6, %%eax; idivl %%ebx" \
		: "=a" (__a) : "a" (__a), "b" (__b) : "edx", "cc"); \
	 __a; })
#define divscale7(a,b) \
	({ int __a=(a), __b=(b); \
	   __asm__ __volatile__ ("movl %%eax, %%edx; sarl $25, %%edx; shll $7, %%eax; idivl %%ebx" \
		: "=a" (__a) : "a" (__a), "b" (__b) : "edx", "cc"); \
	 __a; })
#define divscale8(a,b) \
	({ int __a=(a), __b=(b); \
	   __asm__ __volatile__ ("movl %%eax, %%edx; sarl $24, %%edx; shll $8, %%eax; idivl %%ebx" \
		: "=a" (__a) : "a" (__a), "b" (__b) : "edx", "cc"); \
	 __a; })
#define divscale9(a,b) \
	({ int __a=(a), __b=(b); \
	   __asm__ __volatile__ ("movl %%eax, %%edx; sarl $23, %%edx; shll $9, %%eax; idivl %%ebx" \
		: "=a" (__a) : "a" (__a), "b" (__b) : "edx", "cc"); \
	 __a; })
#define divscale10(a,b) \
	({ int __a=(a), __b=(b); \
	   __asm__ __volatile__ ("movl %%eax, %%edx; sarl $22, %%edx; shll $10, %%eax; idivl %%ebx" \
		: "=a" (__a) : "a" (__a), "b" (__b) : "edx", "cc"); \
	 __a; })
#define divscale11(a,b) \
	({ int __a=(a), __b=(b); \
	   __asm__ __volatile__ ("movl %%eax, %%edx; sarl $21, %%edx; shll $11, %%eax; idivl %%ebx" \
		: "=a" (__a) : "a" (__a), "b" (__b) : "edx", "cc"); \
	 __a; })
#define divscale12(a,b) \
	({ int __a=(a), __b=(b); \
	   __asm__ __volatile__ ("movl %%eax, %%edx; sarl $20, %%edx; shll $12, %%eax; idivl %%ebx" \
		: "=a" (__a) : "a" (__a), "b" (__b) : "edx", "cc"); \
	 __a; })
#define divscale13(a,b) \
	({ int __a=(a), __b=(b); \
	   __asm__ __volatile__ ("movl %%eax, %%edx; sarl $19, %%edx; shll $13, %%eax; idivl %%ebx" \
		: "=a" (__a) : "a" (__a), "b" (__b) : "edx", "cc"); \
	 __a; })
#define divscale14(a,b) \
	({ int __a=(a), __b=(b); \
	   __asm__ __volatile__ ("movl %%eax, %%edx; sarl $18, %%edx; shll $14, %%eax; idivl %%ebx" \
		: "=a" (__a) : "a" (__a), "b" (__b) : "edx", "cc"); \
	 __a; })
#define divscale15(a,b) \
	({ int __a=(a), __b=(b); \
	   __asm__ __volatile__ ("movl %%eax, %%edx; sarl $17, %%edx; shll $15, %%eax; idivl %%ebx" \
		: "=a" (__a) : "a" (__a), "b" (__b) : "edx", "cc"); \
	 __a; })
#define divscale16(a,b) \
	({ int __a=(a), __b=(b); \
	   __asm__ __volatile__ ("movl %%eax, %%edx; sarl $16, %%edx; shll $16, %%eax; idivl %%ebx" \
		: "=a" (__a) : "a" (__a), "b" (__b) : "edx", "cc"); \
	 __a; })
#define divscale17(a,b) \
	({ int __a=(a), __b=(b); \
	   __asm__ __volatile__ ("movl %%eax, %%edx; sarl $15, %%edx; shll $17, %%eax; idivl %%ebx" \
		: "=a" (__a) : "a" (__a), "b" (__b) : "edx", "cc"); \
	 __a; })
#define divscale18(a,b) \
	({ int __a=(a), __b=(b); \
	   __asm__ __volatile__ ("movl %%eax, %%edx; sarl $14, %%edx; shll $18, %%eax; idivl %%ebx" \
		: "=a" (__a) : "a" (__a), "b" (__b) : "edx", "cc"); \
	 __a; })
#define divscale19(a,b) \
	({ int __a=(a), __b=(b); \
	   __asm__ __volatile__ ("movl %%eax, %%edx; sarl $13, %%edx; shll $19, %%eax; idivl %%ebx" \
		: "=a" (__a) : "a" (__a), "b" (__b) : "edx", "cc"); \
	 __a; })
#define divscale20(a,b) \
	({ int __a=(a), __b=(b); \
	   __asm__ __volatile__ ("movl %%eax, %%edx; sarl $12, %%edx; shll $20, %%eax; idivl %%ebx" \
		: "=a" (__a) : "a" (__a), "b" (__b) : "edx", "cc"); \
	 __a; })
#define divscale21(a,b) \
	({ int __a=(a), __b=(b); \
	   __asm__ __volatile__ ("movl %%eax, %%edx; sarl $11, %%edx; shll $21, %%eax; idivl %%ebx" \
		: "=a" (__a) : "a" (__a), "b" (__b) : "edx", "cc"); \
	 __a; })
#define divscale22(a,b) \
	({ int __a=(a), __b=(b); \
	   __asm__ __volatile__ ("movl %%eax, %%edx; sarl $10, %%edx; shll $22, %%eax; idivl %%ebx" \
		: "=a" (__a) : "a" (__a), "b" (__b) : "edx", "cc"); \
	 __a; })
#define divscale23(a,b) \
	({ int __a=(a), __b=(b); \
	   __asm__ __volatile__ ("movl %%eax, %%edx; sarl $9, %%edx; shll $23, %%eax; idivl %%ebx" \
		: "=a" (__a) : "a" (__a), "b" (__b) : "edx", "cc"); \
	 __a; })
#define divscale24(a,b) \
	({ int __a=(a), __b=(b); \
	   __asm__ __volatile__ ("movl %%eax, %%edx; sarl $8, %%edx; shll $24, %%eax; idivl %%ebx" \
		: "=a" (__a) : "a" (__a), "b" (__b) : "edx", "cc"); \
	 __a; })
#define divscale25(a,b) \
	({ int __a=(a), __b=(b); \
	   __asm__ __volatile__ ("movl %%eax, %%edx; sarl $7, %%edx; shll $25, %%eax; idivl %%ebx" \
		: "=a" (__a) : "a" (__a), "b" (__b) : "edx", "cc"); \
	 __a; })
#define divscale26(a,b) \
	({ int __a=(a), __b=(b); \
	   __asm__ __volatile__ ("movl %%eax, %%edx; sarl $6, %%edx; shll $26, %%eax; idivl %%ebx" \
		: "=a" (__a) : "a" (__a), "b" (__b) : "edx", "cc"); \
	 __a; })
#define divscale27(a,b) \
	({ int __a=(a), __b=(b); \
	   __asm__ __volatile__ ("movl %%eax, %%edx; sarl $5, %%edx; shll $27, %%eax; idivl %%ebx" \
		: "=a" (__a) : "a" (__a), "b" (__b) : "edx", "cc"); \
	 __a; })
#define divscale28(a,b) \
	({ int __a=(a), __b=(b); \
	   __asm__ __volatile__ ("movl %%eax, %%edx; sarl $4, %%edx; shll $28, %%eax; idivl %%ebx" \
		: "=a" (__a) : "a" (__a), "b" (__b) : "edx", "cc"); \
	 __a; })
#define divscale29(a,b) \
	({ int __a=(a), __b=(b); \
	   __asm__ __volatile__ ("movl %%eax, %%edx; sarl $3, %%edx; shll $29, %%eax; idivl %%ebx" \
		: "=a" (__a) : "a" (__a), "b" (__b) : "edx", "cc"); \
	 __a; })
#define divscale30(a,b) \
	({ int __a=(a), __b=(b); \
	   __asm__ __volatile__ ("movl %%eax, %%edx; sarl $2, %%edx; shll $30, %%eax; idivl %%ebx" \
		: "=a" (__a) : "a" (__a), "b" (__b) : "edx", "cc"); \
	 __a; })
#define divscale31(a,b) \
	({ int __a=(a), __b=(b); \
	   __asm__ __volatile__ ("movl %%eax, %%edx; sarl $1, %%edx; shll $31, %%eax; idivl %%ebx" \
		: "=a" (__a) : "a" (__a), "b" (__b) : "edx", "cc"); \
	 __a; })
#define divscale32(d,b) \
	({ int __d=(d), __b=(b), __r; \
	   __asm__ __volatile__ ("xorl %%eax, %%eax; idivl %%ebx" \
		: "=a" (__r), "=d" (__d) : "d" (__d), "b" (__b) : "cc"); \
	 __r; })

#define readpixel(D) \
	({ void *__D=(D); int __a; \
	   __asm__ __volatile__ ("movb (%%edi), %%al" \
		: "=a" (__a): "D" (__D) : "cc"); \
	 __a; })
#define drawpixel(D,a) \
	({ void *__D=(D); int __a=(a); \
	   __asm__ __volatile__ ("movb %%al, (%%edi)" \
		: : "D" (__D), "a" (__a) : "memory", "cc"); \
	 0; })
#define drawpixels(D,a) \
	({ void *__D=(D); int __a=(a); \
	   __asm__ __volatile__ ("movw %%ax, (%%edi)" \
		: : "D" (__D), "a" (__a) : "memory", "cc"); \
	 0; })
#define drawpixelses(D,a) \
	({ void *__D=(D); int __a=(a); \
	   __asm__ __volatile__ ("movl %%eax, (%%edi)" \
		: : "D" (__D), "a" (__a) : "memory", "cc"); \
	 0; })
#define clearbuf(D,c,a) \
	({ void *__D=(D); int __c=(c), __a=(a); \
	   __asm__ __volatile__ ("rep; stosl" \
		: "=&D" (__D), "=&c" (__c) : "0" (__D), "1" (__c), "a" (__a) : "memory", "cc"); \
	 0; })
#define copybuf(S,D,c) \
	({ void *__S=(S), *__D=(D); int __c=(c); \
	   __asm__ __volatile__ ("rep; movsl" \
		: "=&S" (__S), "=&D" (__D), "=&c" (__c) : "0" (__S), "1" (__D), "2" (__c) : "memory", "cc"); \
	 0; })

#define mul3(a) \
	({ int __a=(a), __r; \
	   __asm__ __volatile__ ("lea (%1,%1,2), %0" \
		: "=r" (__r) : "0" (__a) : "cc"); \
	 __r; })
#define mul5(a) \
	({ int __a=(a), __r; \
	   __asm__ __volatile__ ("lea (%1,%1,4), %0" \
		: "=r" (__r) : "0" (__a) : "cc"); \
	 __r; })
#define mul9(a) \
	({ int __a=(a), __r; \
	   __asm__ __volatile__ ("lea (%1,%1,8), %0" \
		: "=r" (__r) : "0" (__a) : "cc"); \
	 __r; })

//returns eax/ebx, dmval = eax%edx;
#define divmod(a,b) \
	({ int __a=(a), __b=(b); \
	   __asm__ __volatile__ ("xorl %%edx, %%edx; divl %%ebx; movl %%edx, %[dmval]" \
		: "+a" (__a) : "b" (__b), [dmval] "m" (dmval) : "edx", "memory", "cc"); \
	 __a; })
//returns eax%ebx, dmval = eax/edx;
#define moddiv(a,b) \
	({ int __a=(a), __b=(b), __d; \
	   __asm__ __volatile__ ("xorl %%edx, %%edx; divl %%ebx; movl %%eax, %[dmval]" \
		: "=d" (__d) : "a" (__a), "b" (__b), [dmval] "m" (dmval) : "eax", "memory", "cc"); \
	 __d; })

#define klabs(a) \
	({ int __a=(a); \
	   __asm__ __volatile__ ("testl %%eax, %%eax; jns 0f; negl %%eax; 0:" \
		: "=a" (__a) : "a" (__a) : "cc"); \
	 __a; })
#define ksgn(b) \
	({ int __b=(b), __r; \
	   __asm__ __volatile__ ("addl %%ebx, %%ebx; sbbl %%eax, %%eax; cmpl %%ebx, %%eax; adcb $0, %%al" \
		: "=a" (__r) : "b" (__b) : "cc"); \
	 __r; })

#define umin(a,b) \
	({ int __a=(a), __b=(b); \
	   __asm__ __volatile__ ("subl %%ebx, %%eax; sbbl %%ecx, %%ecx; andl %%ecx, %%eax; addl %%ebx, %%eax" \
	   	: "=a" (__a) : "a" (__a), "b" (__b) : "ecx", "cc"); \
	 __a; })
#define umax(a,b) \
	({ int __a=(a), __b=(b); \
	   __asm__ __volatile__ ("subl %%ebx, %%eax; sbbl %%ecx, %%ecx; xorl $0xffffffff, %%ecx; andl %%ecx, %%eax; addl %%ebx, %%eax" \
		: "=a" (__a) : "a" (__a), "b" (__b) : "ecx", "cc"); \
	 __a; })

#define kmin(a,b) \
	({ int __a=(a), __b=(b); \
	   __asm__ __volatile__ ("cmpl %%ebx, %%eax; jl 0f; movl %%ebx, %%eax; 0:" \
		: "=a" (__a) : "a" (__a), "b" (__b) : "cc"); \
	 __a; })
#define kmax(a,b) \
	({ int __a=(a), __b=(b); \
	   __asm__ __volatile__ ("cmpl %%ebx, %%eax; jg 0f; movl %%ebx, %%eax; 0:" \
		: "=a" (__a) : "a" (__a), "b" (__b) : "cc"); \
	 __a; })

#define swapchar(a,b) \
	({ void *__a=(a), *__b=(b); \
	   __asm__ __volatile__ ("movb (%%eax), %%cl; movb (%%ebx), %%ch; movb %%cl, (%%ebx); movb %%ch, (%%eax)" \
		: : "a" (__a), "b" (__b) : "ecx", "memory", "cc"); \
	 0; })
#define swapshort(a,b) \
	({ void *__a=(a), *__b=(b); \
	   __asm__ __volatile__ ("movw (%%eax), %%cx; movw (%%ebx), %%dx; movw %%cx, (%%ebx); movw %%dx, (%%eax)" \
		: : "a" (__a), "b" (__b) : "ecx", "edx", "memory", "cc"); \
	 0; })
#define swaplong(a,b) \
	({ void *__a=(a), *__b=(b); \
	   __asm__ __volatile__ ("movl (%%eax), %%ecx; movl (%%ebx), %%edx; movl %%ecx, (%%ebx); movl %%edx, (%%eax)" \
		: : "a" (__a), "b" (__b) : "ecx", "edx", "memory", "cc"); \
	 0; })
#define swapbuf4(a,b,c) \
	({ void *__a=(a), *__b=(b); int __c=(c); \
	   __asm__ __volatile__ ("0: movl (%%eax), %%esi; movl (%%ebx), %%edi; movl %%esi, (%%ebx); " \
				"movl %%edi, (%%eax); addl $4, %%eax; addl $4, %%ebx; decl %%ecx; jnz 0b" \
		: : "a" (__a), "b" (__b), "c" (__c) : "esi", "edi", "memory", "cc"); \
	 0; })
#define swap64bit(a,b) \
	({ void *__a=(a), *__b=(b); \
	   __asm__ __volatile__ ("movl (%%eax), %%ecx; movl (%%ebx), %%edx; movl %%ecx, (%%ebx); " \
				"movl 4(%%eax), %%ecx; movl %%edx, (%%eax); movl 4(%%ebx), %%edx; " \
				"movl %%ecx, 4(%%ebx); movl %%edx, 4(%%eax)" \
		: : "a" (__a), "b" (__b) : "ecx", "edx", "memory", "cc"); \
	 0; })

//swapchar2(ptr1,ptr2,xsiz); is the same as:
//swapchar(ptr1,ptr2); swapchar(ptr1+1,ptr2+xsiz);
#define swapchar2(a,b,S) \
	({ void *__a=(a), *__b=(b); int __S=(S); \
	   __asm__ __volatile__ ("addl %%ebx, %%esi; movw (%%eax), %%cx; movb (%%ebx), %%dl; " \
				"movb %%cl, (%%ebx); movb (%%esi), %%dh; movb %%ch, (%%esi); " \
				"movw %%dx, (%%eax)" \
		: "=S" (__S) : "a" (__a), "b" (__b), "S" (__S) : "ecx", "edx", "memory", "cc"); \
	 0; })


#define qinterpolatedown16(a,c,d,S) \
	({ void *__a=(void*)(a); int __c=(c), __d=(d), __S=(S); \
	   __asm__ __volatile__ ("movl %%ecx, %%ebx; shrl $1, %%ecx; jz 1f; " \
				"0: leal (%%edx,%%esi,), %%edi; sarl $16, %%edx; movl %%edx, (%%eax); " \
				"leal (%%edi,%%esi,), %%edx; sarl $16, %%edi; movl %%edi, 4(%%eax); " \
				"addl $8, %%eax; decl %%ecx; jnz 0b; testl $1, %%ebx; jz 2f; " \
				"1: sarl $16, %%edx; movl %%edx, (%%eax); 2:" \
		: "=a" (__a), "=c" (__c), "=d" (__d) : "a" (__a), "c" (__c), "d" (__d), "S" (__S) \
		: "ebx", "edi", "memory", "cc"); \
	 0; })

#define qinterpolatedown16short(a,c,d,S) \
	({ void *__a=(void*)(a); int __c=(c), __d=(d), __S=(S); \
	   __asm__ __volatile__ ("testl %%ecx, %%ecx; jz 3f; testb $2, %%al; jz 0f; movl %%edx, %%ebx; " \
				"sarl $16, %%ebx; movw %%bx, (%%eax); addl %%esi, %%edx; addl $2, %%eax; " \
				"decl %%ecx; jz 3f; " \
				"0: subl $2, %%ecx; jc 2f; " \
				"1: movl %%edx, %%ebx; addl %%esi, %%edx; sarl $16, %%ebx; movl %%edx, %%edi; " \
				"andl $0xffff0000, %%edi; addl %%esi, %%edx; addl %%edi, %%ebx; " \
				"movl %%ebx, (%%eax); addl $4, %%eax; subl $2, %%ecx; jnc 1b; testb $1, %%cl; " \
				"jz 3f; " \
				"2: movl %%edx, %%ebx; sarl $16, %%ebx; movw %%bx, (%%eax); 3:" \
		: "=a" (__a), "=c" (__c), "=d" (__d) : "a" (__a), "c" (__c), "d" (__d), "S" (__S) \
		: "ebx", "edi", "memory", "cc"); \
	 0; })


//}}}

#elif defined(__WATCOMC__) && USE_ASM	// __GNUC__ && __i386__

//
// Watcom C inline assembler
//

//{{{
int sqr(int);
#pragma aux sqr =\
	"imul eax, eax",\
	parm nomemory [eax]\
	modify exact [eax]\
	value [eax]

int scale(int,int,int);
#pragma aux scale =\
	"imul edx",\
	"idiv ecx",\
	parm nomemory [eax][edx][ecx]\
	modify exact [eax edx]

int mulscale(int,int,int);
#pragma aux mulscale =\
	"imul edx",\
	"shrd eax, edx, cl",\
	parm nomemory [eax][edx][ecx]\
	modify exact [eax edx]

int mulscale1(int,int);
#pragma aux mulscale1 =\
	"imul edx",\
	"shrd eax, edx, 1",\
	parm nomemory [eax][edx]\
	modify exact [eax edx]

int mulscale2(int,int);
#pragma aux mulscale2 =\
	"imul edx",\
	"shrd eax, edx, 2",\
	parm nomemory [eax][edx]\
	modify exact [eax edx]

int mulscale3(int,int);
#pragma aux mulscale3 =\
	"imul edx",\
	"shrd eax, edx, 3",\
	parm nomemory [eax][edx]\
	modify exact [eax edx]

int mulscale4(int,int);
#pragma aux mulscale4 =\
	"imul edx",\
	"shrd eax, edx, 4",\
	parm nomemory [eax][edx]\
	modify exact [eax edx]

int mulscale5(int,int);
#pragma aux mulscale5 =\
	"imul edx",\
	"shrd eax, edx, 5",\
	parm nomemory [eax][edx]\
	modify exact [eax edx]

int mulscale6(int,int);
#pragma aux mulscale6 =\
	"imul edx",\
	"shrd eax, edx, 6",\
	parm nomemory [eax][edx]\
	modify exact [eax edx]

int mulscale7(int,int);
#pragma aux mulscale7 =\
	"imul edx",\
	"shrd eax, edx, 7",\
	parm nomemory [eax][edx]\
	modify exact [eax edx]

int mulscale8(int,int);
#pragma aux mulscale8 =\
	"imul edx",\
	"shrd eax, edx, 8",\
	parm nomemory [eax][edx]\
	modify exact [eax edx]

int mulscale9(int,int);
#pragma aux mulscale9 =\
	"imul edx",\
	"shrd eax, edx, 9",\
	parm nomemory [eax][edx]\
	modify exact [eax edx]

int mulscale10(int,int);
#pragma aux mulscale10 =\
	"imul edx",\
	"shrd eax, edx, 10",\
	parm nomemory [eax][edx]\
	modify exact [eax edx]

int mulscale11(int,int);
#pragma aux mulscale11 =\
	"imul edx",\
	"shrd eax, edx, 11",\
	parm nomemory [eax][edx]\
	modify exact [eax edx]

int mulscale12(int,int);
#pragma aux mulscale12 =\
	"imul edx",\
	"shrd eax, edx, 12",\
	parm nomemory [eax][edx]\
	modify exact [eax edx]

int mulscale13(int,int);
#pragma aux mulscale13 =\
	"imul edx",\
	"shrd eax, edx, 13",\
	parm nomemory [eax][edx]\
	modify exact [eax edx]

int mulscale14(int,int);
#pragma aux mulscale14 =\
	"imul edx",\
	"shrd eax, edx, 14",\
	parm nomemory [eax][edx]\
	modify exact [eax edx]

int mulscale15(int,int);
#pragma aux mulscale15 =\
	"imul edx",\
	"shrd eax, edx, 15",\
	parm nomemory [eax][edx]\
	modify exact [eax edx]

int mulscale16(int,int);
#pragma aux mulscale16 =\
	"imul edx",\
	"shrd eax, edx, 16",\
	parm nomemory [eax][edx]\
	modify exact [eax edx]

int mulscale17(int,int);
#pragma aux mulscale17 =\
	"imul edx",\
	"shrd eax, edx, 17",\
	parm nomemory [eax][edx]\
	modify exact [eax edx]

int mulscale18(int,int);
#pragma aux mulscale18 =\
	"imul edx",\
	"shrd eax, edx, 18",\
	parm nomemory [eax][edx]\
	modify exact [eax edx]

int mulscale19(int,int);
#pragma aux mulscale19 =\
	"imul edx",\
	"shrd eax, edx, 19",\
	parm nomemory [eax][edx]\
	modify exact [eax edx]

int mulscale20(int,int);
#pragma aux mulscale20 =\
	"imul edx",\
	"shrd eax, edx, 20",\
	parm nomemory [eax][edx]\
	modify exact [eax edx]

int mulscale21(int,int);
#pragma aux mulscale21 =\
	"imul edx",\
	"shrd eax, edx, 21",\
	parm nomemory [eax][edx]\
	modify exact [eax edx]

int mulscale22(int,int);
#pragma aux mulscale22 =\
	"imul edx",\
	"shrd eax, edx, 22",\
	parm nomemory [eax][edx]\
	modify exact [eax edx]

int mulscale23(int,int);
#pragma aux mulscale23 =\
	"imul edx",\
	"shrd eax, edx, 23",\
	parm nomemory [eax][edx]\
	modify exact [eax edx]

int mulscale24(int,int);
#pragma aux mulscale24 =\
	"imul edx",\
	"shrd eax, edx, 24",\
	parm nomemory [eax][edx]\
	modify exact [eax edx]

int mulscale25(int,int);
#pragma aux mulscale25 =\
	"imul edx",\
	"shrd eax, edx, 25",\
	parm nomemory [eax][edx]\
	modify exact [eax edx]

int mulscale26(int,int);
#pragma aux mulscale26 =\
	"imul edx",\
	"shrd eax, edx, 26",\
	parm nomemory [eax][edx]\
	modify exact [eax edx]

int mulscale27(int,int);
#pragma aux mulscale27 =\
	"imul edx",\
	"shrd eax, edx, 27",\
	parm nomemory [eax][edx]\
	modify exact [eax edx]

int mulscale28(int,int);
#pragma aux mulscale28 =\
	"imul edx",\
	"shrd eax, edx, 28",\
	parm nomemory [eax][edx]\
	modify exact [eax edx]

int mulscale29(int,int);
#pragma aux mulscale29 =\
	"imul edx",\
	"shrd eax, edx, 29",\
	parm nomemory [eax][edx]\
	modify exact [eax edx]

int mulscale30(int,int);
#pragma aux mulscale30 =\
	"imul edx",\
	"shrd eax, edx, 30",\
	parm nomemory [eax][edx]\
	modify exact [eax edx]

int mulscale31(int,int);
#pragma aux mulscale31 =\
	"imul edx",\
	"shrd eax, edx, 31",\
	parm nomemory [eax][edx]\
	modify exact [eax edx]

int mulscale32(int,int);
#pragma aux mulscale32 =\
	"imul edx",\
	parm nomemory [eax][edx]\
	modify exact [eax edx]\
	value [edx]

int dmulscale(int,int,int,int,int);
#pragma aux dmulscale =\
	"imul edx",\
	"mov ebx, eax",\
	"mov eax, esi",\
	"mov esi, edx",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, esi",\
	"shrd eax, edx, cl",\
	parm nomemory [eax][edx][esi][edi][ecx]\
	modify exact [eax ebx edx esi]

int dmulscale1(int,int,int,int);
#pragma aux dmulscale1 =\
	"imul edx",\
	"mov ebx, eax",\
	"mov eax, esi",\
	"mov esi, edx",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, esi",\
	"shrd eax, edx, 1",\
	parm nomemory [eax][edx][esi][edi]\
	modify exact [eax ebx edx esi]

int dmulscale2(int,int,int,int);
#pragma aux dmulscale2 =\
	"imul edx",\
	"mov ebx, eax",\
	"mov eax, esi",\
	"mov esi, edx",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, esi",\
	"shrd eax, edx, 2",\
	parm nomemory [eax][edx][esi][edi]\
	modify exact [eax ebx edx esi]

int dmulscale3(int,int,int,int);
#pragma aux dmulscale3 =\
	"imul edx",\
	"mov ebx, eax",\
	"mov eax, esi",\
	"mov esi, edx",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, esi",\
	"shrd eax, edx, 3",\
	parm nomemory [eax][edx][esi][edi]\
	modify exact [eax ebx edx esi]

int dmulscale4(int,int,int,int);
#pragma aux dmulscale4 =\
	"imul edx",\
	"mov ebx, eax",\
	"mov eax, esi",\
	"mov esi, edx",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, esi",\
	"shrd eax, edx, 4",\
	parm nomemory [eax][edx][esi][edi]\
	modify exact [eax ebx edx esi]

int dmulscale5(int,int,int,int);
#pragma aux dmulscale5 =\
	"imul edx",\
	"mov ebx, eax",\
	"mov eax, esi",\
	"mov esi, edx",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, esi",\
	"shrd eax, edx, 5",\
	parm nomemory [eax][edx][esi][edi]\
	modify exact [eax ebx edx esi]

int dmulscale6(int,int,int,int);
#pragma aux dmulscale6 =\
	"imul edx",\
	"mov ebx, eax",\
	"mov eax, esi",\
	"mov esi, edx",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, esi",\
	"shrd eax, edx, 6",\
	parm nomemory [eax][edx][esi][edi]\
	modify exact [eax ebx edx esi]

int dmulscale7(int,int,int,int);
#pragma aux dmulscale7 =\
	"imul edx",\
	"mov ebx, eax",\
	"mov eax, esi",\
	"mov esi, edx",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, esi",\
	"shrd eax, edx, 7",\
	parm nomemory [eax][edx][esi][edi]\
	modify exact [eax ebx edx esi]

int dmulscale8(int,int,int,int);
#pragma aux dmulscale8 =\
	"imul edx",\
	"mov ebx, eax",\
	"mov eax, esi",\
	"mov esi, edx",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, esi",\
	"shrd eax, edx, 8",\
	parm nomemory [eax][edx][esi][edi]\
	modify exact [eax ebx edx esi]

int dmulscale9(int,int,int,int);
#pragma aux dmulscale9 =\
	"imul edx",\
	"mov ebx, eax",\
	"mov eax, esi",\
	"mov esi, edx",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, esi",\
	"shrd eax, edx, 9",\
	parm nomemory [eax][edx][esi][edi]\
	modify exact [eax ebx edx esi]

int dmulscale10(int,int,int,int);
#pragma aux dmulscale10 =\
	"imul edx",\
	"mov ebx, eax",\
	"mov eax, esi",\
	"mov esi, edx",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, esi",\
	"shrd eax, edx, 10",\
	parm nomemory [eax][edx][esi][edi]\
	modify exact [eax ebx edx esi]

int dmulscale11(int,int,int,int);
#pragma aux dmulscale11 =\
	"imul edx",\
	"mov ebx, eax",\
	"mov eax, esi",\
	"mov esi, edx",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, esi",\
	"shrd eax, edx, 11",\
	parm nomemory [eax][edx][esi][edi]\
	modify exact [eax ebx edx esi]

int dmulscale12(int,int,int,int);
#pragma aux dmulscale12 =\
	"imul edx",\
	"mov ebx, eax",\
	"mov eax, esi",\
	"mov esi, edx",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, esi",\
	"shrd eax, edx, 12",\
	parm nomemory [eax][edx][esi][edi]\
	modify exact [eax ebx edx esi]

int dmulscale13(int,int,int,int);
#pragma aux dmulscale13 =\
	"imul edx",\
	"mov ebx, eax",\
	"mov eax, esi",\
	"mov esi, edx",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, esi",\
	"shrd eax, edx, 13",\
	parm nomemory [eax][edx][esi][edi]\
	modify exact [eax ebx edx esi]

int dmulscale14(int,int,int,int);
#pragma aux dmulscale14 =\
	"imul edx",\
	"mov ebx, eax",\
	"mov eax, esi",\
	"mov esi, edx",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, esi",\
	"shrd eax, edx, 14",\
	parm nomemory [eax][edx][esi][edi]\
	modify exact [eax ebx edx esi]

int dmulscale15(int,int,int,int);
#pragma aux dmulscale15 =\
	"imul edx",\
	"mov ebx, eax",\
	"mov eax, esi",\
	"mov esi, edx",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, esi",\
	"shrd eax, edx, 15",\
	parm nomemory [eax][edx][esi][edi]\
	modify exact [eax ebx edx esi]

int dmulscale16(int,int,int,int);
#pragma aux dmulscale16 =\
	"imul edx",\
	"mov ebx, eax",\
	"mov eax, esi",\
	"mov esi, edx",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, esi",\
	"shrd eax, edx, 16",\
	parm nomemory [eax][edx][esi][edi]\
	modify exact [eax ebx edx esi]

int dmulscale17(int,int,int,int);
#pragma aux dmulscale17 =\
	"imul edx",\
	"mov ebx, eax",\
	"mov eax, esi",\
	"mov esi, edx",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, esi",\
	"shrd eax, edx, 17",\
	parm nomemory [eax][edx][esi][edi]\
	modify exact [eax ebx edx esi]

int dmulscale18(int,int,int,int);
#pragma aux dmulscale18 =\
	"imul edx",\
	"mov ebx, eax",\
	"mov eax, esi",\
	"mov esi, edx",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, esi",\
	"shrd eax, edx, 18",\
	parm nomemory [eax][edx][esi][edi]\
	modify exact [eax ebx edx esi]

int dmulscale19(int,int,int,int);
#pragma aux dmulscale19 =\
	"imul edx",\
	"mov ebx, eax",\
	"mov eax, esi",\
	"mov esi, edx",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, esi",\
	"shrd eax, edx, 19",\
	parm nomemory [eax][edx][esi][edi]\
	modify exact [eax ebx edx esi]

int dmulscale20(int,int,int,int);
#pragma aux dmulscale20 =\
	"imul edx",\
	"mov ebx, eax",\
	"mov eax, esi",\
	"mov esi, edx",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, esi",\
	"shrd eax, edx, 20",\
	parm nomemory [eax][edx][esi][edi]\
	modify exact [eax ebx edx esi]

int dmulscale21(int,int,int,int);
#pragma aux dmulscale21 =\
	"imul edx",\
	"mov ebx, eax",\
	"mov eax, esi",\
	"mov esi, edx",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, esi",\
	"shrd eax, edx, 21",\
	parm nomemory [eax][edx][esi][edi]\
	modify exact [eax ebx edx esi]

int dmulscale22(int,int,int,int);
#pragma aux dmulscale22 =\
	"imul edx",\
	"mov ebx, eax",\
	"mov eax, esi",\
	"mov esi, edx",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, esi",\
	"shrd eax, edx, 22",\
	parm nomemory [eax][edx][esi][edi]\
	modify exact [eax ebx edx esi]

int dmulscale23(int,int,int,int);
#pragma aux dmulscale23 =\
	"imul edx",\
	"mov ebx, eax",\
	"mov eax, esi",\
	"mov esi, edx",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, esi",\
	"shrd eax, edx, 23",\
	parm nomemory [eax][edx][esi][edi]\
	modify exact [eax ebx edx esi]

int dmulscale24(int,int,int,int);
#pragma aux dmulscale24 =\
	"imul edx",\
	"mov ebx, eax",\
	"mov eax, esi",\
	"mov esi, edx",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, esi",\
	"shrd eax, edx, 24",\
	parm nomemory [eax][edx][esi][edi]\
	modify exact [eax ebx edx esi]

int dmulscale25(int,int,int,int);
#pragma aux dmulscale25 =\
	"imul edx",\
	"mov ebx, eax",\
	"mov eax, esi",\
	"mov esi, edx",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, esi",\
	"shrd eax, edx, 25",\
	parm nomemory [eax][edx][esi][edi]\
	modify exact [eax ebx edx esi]

int dmulscale26(int,int,int,int);
#pragma aux dmulscale26 =\
	"imul edx",\
	"mov ebx, eax",\
	"mov eax, esi",\
	"mov esi, edx",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, esi",\
	"shrd eax, edx, 26",\
	parm nomemory [eax][edx][esi][edi]\
	modify exact [eax ebx edx esi]

int dmulscale27(int,int,int,int);
#pragma aux dmulscale27 =\
	"imul edx",\
	"mov ebx, eax",\
	"mov eax, esi",\
	"mov esi, edx",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, esi",\
	"shrd eax, edx, 27",\
	parm nomemory [eax][edx][esi][edi]\
	modify exact [eax ebx edx esi]

int dmulscale28(int,int,int,int);
#pragma aux dmulscale28 =\
	"imul edx",\
	"mov ebx, eax",\
	"mov eax, esi",\
	"mov esi, edx",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, esi",\
	"shrd eax, edx, 28",\
	parm nomemory [eax][edx][esi][edi]\
	modify exact [eax ebx edx esi]

int dmulscale29(int,int,int,int);
#pragma aux dmulscale29 =\
	"imul edx",\
	"mov ebx, eax",\
	"mov eax, esi",\
	"mov esi, edx",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, esi",\
	"shrd eax, edx, 29",\
	parm nomemory [eax][edx][esi][edi]\
	modify exact [eax ebx edx esi]

int dmulscale30(int,int,int,int);
#pragma aux dmulscale30 =\
	"imul edx",\
	"mov ebx, eax",\
	"mov eax, esi",\
	"mov esi, edx",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, esi",\
	"shrd eax, edx, 30",\
	parm nomemory [eax][edx][esi][edi]\
	modify exact [eax ebx edx esi]

int dmulscale31(int,int,int,int);
#pragma aux dmulscale31 =\
	"imul edx",\
	"mov ebx, eax",\
	"mov eax, esi",\
	"mov esi, edx",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, esi",\
	"shrd eax, edx, 31",\
	parm nomemory [eax][edx][esi][edi]\
	modify exact [eax ebx edx esi]

int dmulscale32(int,int,int,int);
#pragma aux dmulscale32 =\
	"imul edx",\
	"mov ebx, eax",\
	"mov eax, esi",\
	"mov esi, edx",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, esi",\
	parm nomemory [eax][edx][esi][edi]\
	modify exact [eax ebx edx esi]\
	value [edx]

int tmulscale1(int,int,int,int,int,int);
#pragma aux tmulscale1 =\
	"imul edx",\
	"xchg eax, ebx",\
	"xchg edx, ecx",\
	"imul edx",\
	"add ebx, eax",\
	"adc ecx, edx",\
	"mov eax, esi",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, ecx",\
	"shrd eax, edx, 1",\
	parm nomemory [eax][edx][ebx][ecx][esi][edi]\
	modify exact [eax ebx ecx edx]

int tmulscale2(int,int,int,int,int,int);
#pragma aux tmulscale2 =\
	"imul edx",\
	"xchg eax, ebx",\
	"xchg edx, ecx",\
	"imul edx",\
	"add ebx, eax",\
	"adc ecx, edx",\
	"mov eax, esi",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, ecx",\
	"shrd eax, edx, 2",\
	parm nomemory [eax][edx][ebx][ecx][esi][edi]\
	modify exact [eax ebx ecx edx]

int tmulscale3(int,int,int,int,int,int);
#pragma aux tmulscale3 =\
	"imul edx",\
	"xchg eax, ebx",\
	"xchg edx, ecx",\
	"imul edx",\
	"add ebx, eax",\
	"adc ecx, edx",\
	"mov eax, esi",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, ecx",\
	"shrd eax, edx, 3",\
	parm nomemory [eax][edx][ebx][ecx][esi][edi]\
	modify exact [eax ebx ecx edx]

int tmulscale4(int,int,int,int,int,int);
#pragma aux tmulscale4 =\
	"imul edx",\
	"xchg eax, ebx",\
	"xchg edx, ecx",\
	"imul edx",\
	"add ebx, eax",\
	"adc ecx, edx",\
	"mov eax, esi",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, ecx",\
	"shrd eax, edx, 4",\
	parm nomemory [eax][edx][ebx][ecx][esi][edi]\
	modify exact [eax ebx ecx edx]

int tmulscale5(int,int,int,int,int,int);
#pragma aux tmulscale5 =\
	"imul edx",\
	"xchg eax, ebx",\
	"xchg edx, ecx",\
	"imul edx",\
	"add ebx, eax",\
	"adc ecx, edx",\
	"mov eax, esi",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, ecx",\
	"shrd eax, edx, 5",\
	parm nomemory [eax][edx][ebx][ecx][esi][edi]\
	modify exact [eax ebx ecx edx]

int tmulscale6(int,int,int,int,int,int);
#pragma aux tmulscale6 =\
	"imul edx",\
	"xchg eax, ebx",\
	"xchg edx, ecx",\
	"imul edx",\
	"add ebx, eax",\
	"adc ecx, edx",\
	"mov eax, esi",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, ecx",\
	"shrd eax, edx, 6",\
	parm nomemory [eax][edx][ebx][ecx][esi][edi]\
	modify exact [eax ebx ecx edx]

int tmulscale7(int,int,int,int,int,int);
#pragma aux tmulscale7 =\
	"imul edx",\
	"xchg eax, ebx",\
	"xchg edx, ecx",\
	"imul edx",\
	"add ebx, eax",\
	"adc ecx, edx",\
	"mov eax, esi",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, ecx",\
	"shrd eax, edx, 7",\
	parm nomemory [eax][edx][ebx][ecx][esi][edi]\
	modify exact [eax ebx ecx edx]

int tmulscale8(int,int,int,int,int,int);
#pragma aux tmulscale8 =\
	"imul edx",\
	"xchg eax, ebx",\
	"xchg edx, ecx",\
	"imul edx",\
	"add ebx, eax",\
	"adc ecx, edx",\
	"mov eax, esi",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, ecx",\
	"shrd eax, edx, 8",\
	parm nomemory [eax][edx][ebx][ecx][esi][edi]\
	modify exact [eax ebx ecx edx]

int tmulscale9(int,int,int,int,int,int);
#pragma aux tmulscale9 =\
	"imul edx",\
	"xchg eax, ebx",\
	"xchg edx, ecx",\
	"imul edx",\
	"add ebx, eax",\
	"adc ecx, edx",\
	"mov eax, esi",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, ecx",\
	"shrd eax, edx, 9",\
	parm nomemory [eax][edx][ebx][ecx][esi][edi]\
	modify exact [eax ebx ecx edx]

int tmulscale10(int,int,int,int,int,int);
#pragma aux tmulscale10 =\
	"imul edx",\
	"xchg eax, ebx",\
	"xchg edx, ecx",\
	"imul edx",\
	"add ebx, eax",\
	"adc ecx, edx",\
	"mov eax, esi",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, ecx",\
	"shrd eax, edx, 10",\
	parm nomemory [eax][edx][ebx][ecx][esi][edi]\
	modify exact [eax ebx ecx edx]

int tmulscale11(int,int,int,int,int,int);
#pragma aux tmulscale11 =\
	"imul edx",\
	"xchg eax, ebx",\
	"xchg edx, ecx",\
	"imul edx",\
	"add ebx, eax",\
	"adc ecx, edx",\
	"mov eax, esi",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, ecx",\
	"shrd eax, edx, 11",\
	parm nomemory [eax][edx][ebx][ecx][esi][edi]\
	modify exact [eax ebx ecx edx]

int tmulscale12(int,int,int,int,int,int);
#pragma aux tmulscale12 =\
	"imul edx",\
	"xchg eax, ebx",\
	"xchg edx, ecx",\
	"imul edx",\
	"add ebx, eax",\
	"adc ecx, edx",\
	"mov eax, esi",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, ecx",\
	"shrd eax, edx, 12",\
	parm nomemory [eax][edx][ebx][ecx][esi][edi]\
	modify exact [eax ebx ecx edx]

int tmulscale13(int,int,int,int,int,int);
#pragma aux tmulscale13 =\
	"imul edx",\
	"xchg eax, ebx",\
	"xchg edx, ecx",\
	"imul edx",\
	"add ebx, eax",\
	"adc ecx, edx",\
	"mov eax, esi",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, ecx",\
	"shrd eax, edx, 13",\
	parm nomemory [eax][edx][ebx][ecx][esi][edi]\
	modify exact [eax ebx ecx edx]

int tmulscale14(int,int,int,int,int,int);
#pragma aux tmulscale14 =\
	"imul edx",\
	"xchg eax, ebx",\
	"xchg edx, ecx",\
	"imul edx",\
	"add ebx, eax",\
	"adc ecx, edx",\
	"mov eax, esi",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, ecx",\
	"shrd eax, edx, 14",\
	parm nomemory [eax][edx][ebx][ecx][esi][edi]\
	modify exact [eax ebx ecx edx]

int tmulscale15(int,int,int,int,int,int);
#pragma aux tmulscale15 =\
	"imul edx",\
	"xchg eax, ebx",\
	"xchg edx, ecx",\
	"imul edx",\
	"add ebx, eax",\
	"adc ecx, edx",\
	"mov eax, esi",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, ecx",\
	"shrd eax, edx, 15",\
	parm nomemory [eax][edx][ebx][ecx][esi][edi]\
	modify exact [eax ebx ecx edx]

int tmulscale16(int,int,int,int,int,int);
#pragma aux tmulscale16 =\
	"imul edx",\
	"xchg eax, ebx",\
	"xchg edx, ecx",\
	"imul edx",\
	"add ebx, eax",\
	"adc ecx, edx",\
	"mov eax, esi",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, ecx",\
	"shrd eax, edx, 16",\
	parm nomemory [eax][edx][ebx][ecx][esi][edi]\
	modify exact [eax ebx ecx edx]

int tmulscale17(int,int,int,int,int,int);
#pragma aux tmulscale17 =\
	"imul edx",\
	"xchg eax, ebx",\
	"xchg edx, ecx",\
	"imul edx",\
	"add ebx, eax",\
	"adc ecx, edx",\
	"mov eax, esi",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, ecx",\
	"shrd eax, edx, 17",\
	parm nomemory [eax][edx][ebx][ecx][esi][edi]\
	modify exact [eax ebx ecx edx]

int tmulscale18(int,int,int,int,int,int);
#pragma aux tmulscale18 =\
	"imul edx",\
	"xchg eax, ebx",\
	"xchg edx, ecx",\
	"imul edx",\
	"add ebx, eax",\
	"adc ecx, edx",\
	"mov eax, esi",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, ecx",\
	"shrd eax, edx, 18",\
	parm nomemory [eax][edx][ebx][ecx][esi][edi]\
	modify exact [eax ebx ecx edx]

int tmulscale19(int,int,int,int,int,int);
#pragma aux tmulscale19 =\
	"imul edx",\
	"xchg eax, ebx",\
	"xchg edx, ecx",\
	"imul edx",\
	"add ebx, eax",\
	"adc ecx, edx",\
	"mov eax, esi",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, ecx",\
	"shrd eax, edx, 19",\
	parm nomemory [eax][edx][ebx][ecx][esi][edi]\
	modify exact [eax ebx ecx edx]

int tmulscale20(int,int,int,int,int,int);
#pragma aux tmulscale20 =\
	"imul edx",\
	"xchg eax, ebx",\
	"xchg edx, ecx",\
	"imul edx",\
	"add ebx, eax",\
	"adc ecx, edx",\
	"mov eax, esi",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, ecx",\
	"shrd eax, edx, 20",\
	parm nomemory [eax][edx][ebx][ecx][esi][edi]\
	modify exact [eax ebx ecx edx]

int tmulscale21(int,int,int,int,int,int);
#pragma aux tmulscale21 =\
	"imul edx",\
	"xchg eax, ebx",\
	"xchg edx, ecx",\
	"imul edx",\
	"add ebx, eax",\
	"adc ecx, edx",\
	"mov eax, esi",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, ecx",\
	"shrd eax, edx, 21",\
	parm nomemory [eax][edx][ebx][ecx][esi][edi]\
	modify exact [eax ebx ecx edx]

int tmulscale22(int,int,int,int,int,int);
#pragma aux tmulscale22 =\
	"imul edx",\
	"xchg eax, ebx",\
	"xchg edx, ecx",\
	"imul edx",\
	"add ebx, eax",\
	"adc ecx, edx",\
	"mov eax, esi",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, ecx",\
	"shrd eax, edx, 22",\
	parm nomemory [eax][edx][ebx][ecx][esi][edi]\
	modify exact [eax ebx ecx edx]

int tmulscale23(int,int,int,int,int,int);
#pragma aux tmulscale23 =\
	"imul edx",\
	"xchg eax, ebx",\
	"xchg edx, ecx",\
	"imul edx",\
	"add ebx, eax",\
	"adc ecx, edx",\
	"mov eax, esi",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, ecx",\
	"shrd eax, edx, 23",\
	parm nomemory [eax][edx][ebx][ecx][esi][edi]\
	modify exact [eax ebx ecx edx]

int tmulscale24(int,int,int,int,int,int);
#pragma aux tmulscale24 =\
	"imul edx",\
	"xchg eax, ebx",\
	"xchg edx, ecx",\
	"imul edx",\
	"add ebx, eax",\
	"adc ecx, edx",\
	"mov eax, esi",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, ecx",\
	"shrd eax, edx, 24",\
	parm nomemory [eax][edx][ebx][ecx][esi][edi]\
	modify exact [eax ebx ecx edx]

int tmulscale25(int,int,int,int,int,int);
#pragma aux tmulscale25 =\
	"imul edx",\
	"xchg eax, ebx",\
	"xchg edx, ecx",\
	"imul edx",\
	"add ebx, eax",\
	"adc ecx, edx",\
	"mov eax, esi",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, ecx",\
	"shrd eax, edx, 25",\
	parm nomemory [eax][edx][ebx][ecx][esi][edi]\
	modify exact [eax ebx ecx edx]

int tmulscale26(int,int,int,int,int,int);
#pragma aux tmulscale26 =\
	"imul edx",\
	"xchg eax, ebx",\
	"xchg edx, ecx",\
	"imul edx",\
	"add ebx, eax",\
	"adc ecx, edx",\
	"mov eax, esi",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, ecx",\
	"shrd eax, edx, 26",\
	parm nomemory [eax][edx][ebx][ecx][esi][edi]\
	modify exact [eax ebx ecx edx]

int tmulscale27(int,int,int,int,int,int);
#pragma aux tmulscale27 =\
	"imul edx",\
	"xchg eax, ebx",\
	"xchg edx, ecx",\
	"imul edx",\
	"add ebx, eax",\
	"adc ecx, edx",\
	"mov eax, esi",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, ecx",\
	"shrd eax, edx, 27",\
	parm nomemory [eax][edx][ebx][ecx][esi][edi]\
	modify exact [eax ebx ecx edx]

int tmulscale28(int,int,int,int,int,int);
#pragma aux tmulscale28 =\
	"imul edx",\
	"xchg eax, ebx",\
	"xchg edx, ecx",\
	"imul edx",\
	"add ebx, eax",\
	"adc ecx, edx",\
	"mov eax, esi",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, ecx",\
	"shrd eax, edx, 28",\
	parm nomemory [eax][edx][ebx][ecx][esi][edi]\
	modify exact [eax ebx ecx edx]

int tmulscale29(int,int,int,int,int,int);
#pragma aux tmulscale29 =\
	"imul edx",\
	"xchg eax, ebx",\
	"xchg edx, ecx",\
	"imul edx",\
	"add ebx, eax",\
	"adc ecx, edx",\
	"mov eax, esi",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, ecx",\
	"shrd eax, edx, 29",\
	parm nomemory [eax][edx][ebx][ecx][esi][edi]\
	modify exact [eax ebx ecx edx]

int tmulscale30(int,int,int,int,int,int);
#pragma aux tmulscale30 =\
	"imul edx",\
	"xchg eax, ebx",\
	"xchg edx, ecx",\
	"imul edx",\
	"add ebx, eax",\
	"adc ecx, edx",\
	"mov eax, esi",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, ecx",\
	"shrd eax, edx, 30",\
	parm nomemory [eax][edx][ebx][ecx][esi][edi]\
	modify exact [eax ebx ecx edx]

int tmulscale31(int,int,int,int,int,int);
#pragma aux tmulscale31 =\
	"imul edx",\
	"xchg eax, ebx",\
	"xchg edx, ecx",\
	"imul edx",\
	"add ebx, eax",\
	"adc ecx, edx",\
	"mov eax, esi",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, ecx",\
	"shrd eax, edx, 31",\
	parm nomemory [eax][edx][ebx][ecx][esi][edi]\
	modify exact [eax ebx ecx edx]

int tmulscale32(int,int,int,int,int,int);
#pragma aux tmulscale32 =\
	"imul edx",\
	"xchg eax, ebx",\
	"xchg edx, ecx",\
	"imul edx",\
	"add ebx, eax",\
	"adc ecx, edx",\
	"mov eax, esi",\
	"imul edi",\
	"add eax, ebx",\
	"adc edx, ecx",\
	parm nomemory [eax][edx][ebx][ecx][esi][edi]\
	modify exact [eax ebx ecx edx]\
	value [edx]

int boundmulscale(int,int,int);
#pragma aux boundmulscale =\
	"imul ebx",\
	"mov ebx, edx",\
	"shrd eax, edx, cl",\
	"sar edx, cl",\
	"xor edx, eax",\
	"js checkit",\
	"xor edx, eax",\
	"jz skipboundit",\
	"cmp edx, 0xffffffff",\
	"je skipboundit",\
	"checkit:",\
	"mov eax, ebx",\
	"sar eax, 31",\
	"xor eax, 0x7fffffff",\
	"skipboundit:",\
	parm nomemory [eax][ebx][ecx]\
	modify exact [eax ebx edx]

int divscale(int,int,int);
#pragma aux divscale =\
	"mov edx, eax",\
	"shl eax, cl",\
	"neg cl",\
	"sar edx, cl",\
	"idiv ebx",\
	parm nomemory [eax][ebx][ecx]\
	modify exact [eax ecx edx]

int divscale1(int,int);
#pragma aux divscale1 =\
	"add eax, eax",\
	"sbb edx, edx",\
	"idiv ebx",\
	parm nomemory [eax][ebx]\
	modify exact [eax edx]

int divscale2(int,int);
#pragma aux divscale2 =\
	"mov edx, eax",\
	"sar edx, 30",\
	"lea eax, [eax*4]",\
	"idiv ebx",\
	parm nomemory [eax][ebx]\
	modify exact [eax edx]

int divscale3(int,int);
#pragma aux divscale3 =\
	"mov edx, eax",\
	"sar edx, 29",\
	"lea eax, [eax*8]",\
	"idiv ebx",\
	parm nomemory [eax][ebx]\
	modify exact [eax edx]

int divscale4(int,int);
#pragma aux divscale4 =\
	"mov edx, eax",\
	"sar edx, 28",\
	"shl eax, 4",\
	"idiv ebx",\
	parm nomemory [eax][ebx]\
	modify exact [eax edx]

int divscale5(int,int);
#pragma aux divscale5 =\
	"mov edx, eax",\
	"sar edx, 27",\
	"shl eax, 5",\
	"idiv ebx",\
	parm nomemory [eax][ebx]\
	modify exact [eax edx]

int divscale6(int,int);
#pragma aux divscale6 =\
	"mov edx, eax",\
	"sar edx, 26",\
	"shl eax, 6",\
	"idiv ebx",\
	parm nomemory [eax][ebx]\
	modify exact [eax edx]

int divscale7(int,int);
#pragma aux divscale7 =\
	"mov edx, eax",\
	"sar edx, 25",\
	"shl eax, 7",\
	"idiv ebx",\
	parm nomemory [eax][ebx]\
	modify exact [eax edx]

int divscale8(int,int);
#pragma aux divscale8 =\
	"mov edx, eax",\
	"sar edx, 24",\
	"shl eax, 8",\
	"idiv ebx",\
	parm nomemory [eax][ebx]\
	modify exact [eax edx]

int divscale9(int,int);
#pragma aux divscale9 =\
	"mov edx, eax",\
	"sar edx, 23",\
	"shl eax, 9",\
	"idiv ebx",\
	parm nomemory [eax][ebx]\
	modify exact [eax edx]

int divscale10(int,int);
#pragma aux divscale10 =\
	"mov edx, eax",\
	"sar edx, 22",\
	"shl eax, 10",\
	"idiv ebx",\
	parm nomemory [eax][ebx]\
	modify exact [eax edx]

int divscale11(int,int);
#pragma aux divscale11 =\
	"mov edx, eax",\
	"sar edx, 21",\
	"shl eax, 11",\
	"idiv ebx",\
	parm nomemory [eax][ebx]\
	modify exact [eax edx]

int divscale12(int,int);
#pragma aux divscale12 =\
	"mov edx, eax",\
	"sar edx, 20",\
	"shl eax, 12",\
	"idiv ebx",\
	parm nomemory [eax][ebx]\
	modify exact [eax edx]

int divscale13(int,int);
#pragma aux divscale13 =\
	"mov edx, eax",\
	"sar edx, 19",\
	"shl eax, 13",\
	"idiv ebx",\
	parm nomemory [eax][ebx]\
	modify exact [eax edx]

int divscale14(int,int);
#pragma aux divscale14 =\
	"mov edx, eax",\
	"sar edx, 18",\
	"shl eax, 14",\
	"idiv ebx",\
	parm nomemory [eax][ebx]\
	modify exact [eax edx]

int divscale15(int,int);
#pragma aux divscale15 =\
	"mov edx, eax",\
	"sar edx, 17",\
	"shl eax, 15",\
	"idiv ebx",\
	parm nomemory [eax][ebx]\
	modify exact [eax edx]

int divscale16(int,int);
#pragma aux divscale16 =\
	"mov edx, eax",\
	"sar edx, 16",\
	"shl eax, 16",\
	"idiv ebx",\
	parm nomemory [eax][ebx]\
	modify exact [eax edx]

int divscale17(int,int);
#pragma aux divscale17 =\
	"mov edx, eax",\
	"sar edx, 15",\
	"shl eax, 17",\
	"idiv ebx",\
	parm nomemory [eax][ebx]\
	modify exact [eax edx]

int divscale18(int,int);
#pragma aux divscale18 =\
	"mov edx, eax",\
	"sar edx, 14",\
	"shl eax, 18",\
	"idiv ebx",\
	parm nomemory [eax][ebx]\
	modify exact [eax edx]

int divscale19(int,int);
#pragma aux divscale19 =\
	"mov edx, eax",\
	"sar edx, 13",\
	"shl eax, 19",\
	"idiv ebx",\
	parm nomemory [eax][ebx]\
	modify exact [eax edx]

int divscale20(int,int);
#pragma aux divscale20 =\
	"mov edx, eax",\
	"sar edx, 12",\
	"shl eax, 20",\
	"idiv ebx",\
	parm nomemory [eax][ebx]\
	modify exact [eax edx]

int divscale21(int,int);
#pragma aux divscale21 =\
	"mov edx, eax",\
	"sar edx, 11",\
	"shl eax, 21",\
	"idiv ebx",\
	parm nomemory [eax][ebx]\
	modify exact [eax edx]

int divscale22(int,int);
#pragma aux divscale22 =\
	"mov edx, eax",\
	"sar edx, 10",\
	"shl eax, 22",\
	"idiv ebx",\
	parm nomemory [eax][ebx]\
	modify exact [eax edx]

int divscale23(int,int);
#pragma aux divscale23 =\
	"mov edx, eax",\
	"sar edx, 9",\
	"shl eax, 23",\
	"idiv ebx",\
	parm nomemory [eax][ebx]\
	modify exact [eax edx]

int divscale24(int,int);
#pragma aux divscale24 =\
	"mov edx, eax",\
	"sar edx, 8",\
	"shl eax, 24",\
	"idiv ebx",\
	parm nomemory [eax][ebx]\
	modify exact [eax edx]

int divscale25(int,int);
#pragma aux divscale25 =\
	"mov edx, eax",\
	"sar edx, 7",\
	"shl eax, 25",\
	"idiv ebx",\
	parm nomemory [eax][ebx]\
	modify exact [eax edx]

int divscale26(int,int);
#pragma aux divscale26 =\
	"mov edx, eax",\
	"sar edx, 6",\
	"shl eax, 26",\
	"idiv ebx",\
	parm nomemory [eax][ebx]\
	modify exact [eax edx]

int divscale27(int,int);
#pragma aux divscale27 =\
	"mov edx, eax",\
	"sar edx, 5",\
	"shl eax, 27",\
	"idiv ebx",\
	parm nomemory [eax][ebx]\
	modify exact [eax edx]

int divscale28(int,int);
#pragma aux divscale28 =\
	"mov edx, eax",\
	"sar edx, 4",\
	"shl eax, 28",\
	"idiv ebx",\
	parm nomemory [eax][ebx]\
	modify exact [eax edx]

int divscale29(int,int);
#pragma aux divscale29 =\
	"mov edx, eax",\
	"sar edx, 3",\
	"shl eax, 29",\
	"idiv ebx",\
	parm nomemory [eax][ebx]\
	modify exact [eax edx]

int divscale30(int,int);
#pragma aux divscale30 =\
	"mov edx, eax",\
	"sar edx, 2",\
	"shl eax, 30",\
	"idiv ebx",\
	parm nomemory [eax][ebx]\
	modify exact [eax edx]

int divscale31(int,int);
#pragma aux divscale31 =\
	"mov edx, eax",\
	"sar edx, 1",\
	"shl eax, 31",\
	"idiv ebx",\
	parm nomemory [eax][ebx]\
	modify exact [eax edx]

int divscale32(int,int);
#pragma aux divscale32 =\
	"xor eax, eax",\
	"idiv ebx",\
	parm nomemory [edx][ebx]\
	modify exact [eax edx]

int readpixel(void*);
#pragma aux readpixel =\
	"mov al, byte ptr [edi]",\
	parm nomemory [edi]\
	modify exact [eax]

int drawpixel(void*,int);
#pragma aux drawpixel =\
	"mov byte ptr [edi], al",\
	parm [edi][eax]\
	modify exact

int drawpixels(void*,int);
#pragma aux drawpixels =\
	"mov word ptr [edi], ax",\
	parm [edi][eax]\
	modify exact

int drawpixelses(void*,int);
#pragma aux drawpixelses =\
	"mov dword ptr [edi], eax",\
	parm [edi][eax]\
	modify exact

int clearbuf(void*,int,int);
#pragma aux clearbuf =\
	"rep stosd",\
	parm [edi][ecx][eax]\
	modify exact [edi ecx]

int clearbufbyte(void*,int,int);
#pragma aux clearbufbyte =\
	"cmp ecx, 4",\
	"jae longcopy",\
	"test cl, 1",\
	"jz preskip",\
	"stosb",\
	"preskip: shr ecx, 1",\
	"rep stosw",\
	"jmp endit",\
	"longcopy: test edi, 1",\
	"jz skip1",\
	"stosb",\
	"dec ecx",\
	"skip1: test edi, 2",\
	"jz skip2",\
	"stosw",\
	"sub ecx, 2",\
	"skip2: mov ebx, ecx",\
	"shr ecx, 2",\
	"rep stosd",\
	"test bl, 2",\
	"jz skip3",\
	"stosw",\
	"skip3: test bl, 1",\
	"jz endit",\
	"stosb",\
	"endit:",\
	parm [edi][ecx][eax]\
	modify [ebx]

int copybuf(void*,void*,int);
#pragma aux copybuf =\
	"rep movsd",\
	parm [esi][edi][ecx]\
	modify exact [ecx esi edi]

int copybufbyte(void*,void*,int);
#pragma aux copybufbyte =\
	"cmp ecx, 4",\
	"jae longcopy",\
	"test cl, 1",\
	"jz preskip",\
	"movsb",\
	"preskip: shr ecx, 1",\
	"rep movsw",\
	"jmp endit",\
	"longcopy: test edi, 1",\
	"jz skip1",\
	"movsb",\
	"dec ecx",\
	"skip1: test edi, 2",\
	"jz skip2",\
	"movsw",\
	"sub ecx, 2",\
	"skip2: mov ebx, ecx",\
	"shr ecx, 2",\
	"rep movsd",\
	"test bl, 2",\
	"jz skip3",\
	"movsw",\
	"skip3: test bl, 1",\
	"jz endit",\
	"movsb",\
	"endit:",\
	parm [esi][edi][ecx]\
	modify [ebx]

int copybufreverse(void*,void*,int);
#pragma aux copybufreverse =\
	"shr ecx, 1",\
	"jnc skipit1",\
	"mov al, byte ptr [esi]",\
	"dec esi",\
	"mov byte ptr [edi], al",\
	"inc edi",\
	"skipit1: shr ecx, 1",\
	"jnc skipit2",\
	"mov ax, word ptr [esi-1]",\
	"sub esi, 2",\
	"ror ax, 8",\
	"mov word ptr [edi], ax",\
	"add edi, 2",\
	"skipit2: test ecx, ecx",\
	"jz endloop",\
	"begloop: mov eax, dword ptr [esi-3]",\
	"sub esi, 4",\
	"bswap eax",\
	"mov dword ptr [edi], eax",\
	"add edi, 4",\
	"dec ecx",\
	"jnz begloop",\
	"endloop:",\
	parm [esi][edi][ecx]

int qinterpolatedown16(int,int,int,int);
#pragma aux qinterpolatedown16 =\
	"mov ebx, ecx",\
	"shr ecx, 1",\
	"jz skipbegcalc",\
	"begqcalc: lea edi, [edx+esi]",\
	"sar edx, 16",\
	"mov dword ptr [eax], edx",\
	"lea edx, [edi+esi]",\
	"sar edi, 16",\
	"mov dword ptr [eax+4], edi",\
	"add eax, 8",\
	"dec ecx",\
	"jnz begqcalc",\
	"test ebx, 1",\
	"jz skipbegqcalc2",\
	"skipbegcalc: sar edx, 16",\
	"mov dword ptr [eax], edx",\
	"skipbegqcalc2:",\
	parm [eax][ecx][edx][esi]\
	modify exact [eax ebx ecx edx edi]

int qinterpolatedown16short(int,int,int,int);
#pragma aux qinterpolatedown16short =\
	"test ecx, ecx",\
	"jz endit",\
	"test al, 2",\
	"jz skipalignit",\
	"mov ebx, edx",\
	"sar ebx, 16",\
	"mov word ptr [eax], bx",\
	"add edx, esi",\
	"add eax, 2",\
	"dec ecx",\
	"jz endit",\
	"skipalignit: sub ecx, 2",\
	"jc finishit",\
	"begqcalc: mov ebx, edx",\
	"add edx, esi",\
	"sar ebx, 16",\
	"mov edi, edx",\
	"and edi, 0ffff0000h",\
	"add edx, esi",\
	"add ebx, edi",\
	"mov dword ptr [eax], ebx",\
	"add eax, 4",\
	"sub ecx, 2",\
	"jnc begqcalc",\
	"test cl, 1",\
	"jz endit",\
	"finishit: mov ebx, edx",\
	"sar ebx, 16",\
	"mov word ptr [eax], bx",\
	"endit:",\
	parm [eax][ecx][edx][esi]\
	modify exact [eax ebx ecx edx edi]

int mul3(int);
#pragma aux mul3 =\
	"lea eax, [eax+eax*2]",\
	parm nomemory [eax]

int mul5(int);
#pragma aux mul5 =\
	"lea eax, [eax+eax*4]",\
	parm nomemory [eax]

int mul9(int);
#pragma aux mul9 =\
	"lea eax, [eax+eax*8]",\
	parm nomemory [eax]

	//returns eax/ebx, dmval = eax%edx;
int divmod(int,int);
#pragma aux divmod =\
	"xor edx, edx",\
	"div ebx",\
	"mov dmval, edx",\
	parm [eax][ebx]\
	modify exact [eax edx]\
	value [eax]

	//returns eax%ebx, dmval = eax/edx;
int moddiv(int,int);
#pragma aux moddiv =\
	"xor edx, edx",\
	"div ebx",\
	"mov dmval, eax",\
	parm [eax][ebx]\
	modify exact [eax edx]\
	value [edx]

int klabs(int);
#pragma aux klabs =\
	"test eax, eax",\
	"jns skipnegate",\
	"neg eax",\
	"skipnegate:",\
	parm nomemory [eax]

int ksgn(int);
#pragma aux ksgn =\
	"add ebx, ebx",\
	"sbb eax, eax",\
	"cmp eax, ebx",\
	"adc al, 0",\
	parm nomemory [ebx]\
	modify exact [eax ebx]

	//eax = (unsigned min)umin(eax,ebx)
int umin(int,int);
#pragma aux umin =\
	"sub eax, ebx",\
	"sbb ecx, ecx",\
	"and eax, ecx",\
	"add eax, ebx",\
	parm nomemory [eax][ebx]\
	modify exact [eax ecx]

	//eax = (unsigned max)umax(eax,ebx)
int umax(int,int);
#pragma aux umax =\
	"sub eax, ebx",\
	"sbb ecx, ecx",\
	"xor ecx, 0xffffffff",\
	"and eax, ecx",\
	"add eax, ebx",\
	parm nomemory [eax][ebx]\
	modify exact [eax ecx]

int kmin(int,int);
#pragma aux kmin =\
	"cmp eax, ebx",\
	"jl skipit",\
	"mov eax, ebx",\
	"skipit:",\
	parm nomemory [eax][ebx]\
	modify exact [eax]

int kmax(int,int);
#pragma aux kmax =\
	"cmp eax, ebx",\
	"jg skipit",\
	"mov eax, ebx",\
	"skipit:",\
	parm nomemory [eax][ebx]\
	modify exact [eax]

int swapchar(void*,void*);
#pragma aux swapchar =\
	"mov cl, [eax]",\
	"mov ch, [ebx]",\
	"mov [ebx], cl",\
	"mov [eax], ch",\
	parm [eax][ebx]\
	modify exact [ecx]

int swapshort(void*,void*);
#pragma aux swapshort =\
	"mov cx, [eax]",\
	"mov dx, [ebx]",\
	"mov [ebx], cx",\
	"mov [eax], dx",\
	parm [eax][ebx]\
	modify exact [ecx edx]

int swaplong(void*,void*);
#pragma aux swaplong =\
	"mov ecx, [eax]",\
	"mov edx, [ebx]",\
	"mov [ebx], ecx",\
	"mov [eax], edx",\
	parm [eax][ebx]\
	modify exact [ecx edx]

int swapbuf4(void*,void*,int);
#pragma aux swapbuf4 =\
	"begswap:",\
	"mov esi, [eax]",\
	"mov edi, [ebx]",\
	"mov [ebx], esi",\
	"mov [eax], edi",\
	"add eax, 4",\
	"add ebx, 4",\
	"dec ecx",\
	"jnz short begswap",\
	parm [eax][ebx][ecx]\
	modify exact [eax ebx ecx esi edi]

int swap64bit(void*,void*);
#pragma aux swap64bit =\
	"mov ecx, [eax]",\
	"mov edx, [ebx]",\
	"mov [ebx], ecx",\
	"mov ecx, [eax+4]",\
	"mov [eax], edx",\
	"mov edx, [ebx+4]",\
	"mov [ebx+4], ecx",\
	"mov [eax+4], edx",\
	parm [eax][ebx]\
	modify exact [ecx edx]

	//swapchar2(ptr1,ptr2,xsiz); is the same as:
	//swapchar(ptr1,ptr2); swapchar(ptr1+1,ptr2+xsiz);
int swapchar2(void*,void*,int);
#pragma aux swapchar2 =\
	"add esi, ebx",\
	"mov cx, [eax]",\
	"mov dl, [ebx]",\
	"mov [ebx], cl",\
	"mov dh, [esi]",\
	"mov [esi], ch",\
	"mov [eax], dx",\
	parm [eax][ebx][esi]\
	modify exact [ecx edx esi]
//}}}

#elif defined(_MSC_VER) && defined(_M_IX86) && USE_ASM	// __WATCOMC__

//
// Microsoft C inline assembler
//

//{{{
static __inline int sqr(int a)
{
	_asm {
		mov eax, a
		imul eax, eax
	}
}

static __inline int scale(int a, int d, int c)
{
	_asm {
		mov eax, a
		imul d
		idiv c
	}
}

static __inline int mulscale(int a, int d, int c)
{
	_asm {
		mov ecx, c
		mov eax, a
		imul d
		shrd eax, edx, cl
	}
}

#define MULSCALE(x) \
static __inline int mulscale##x (int a, int d) \
{ \
	_asm mov eax, a \
	_asm imul d \
	_asm shrd eax, edx, x \
}

MULSCALE(1)	MULSCALE(2)	MULSCALE(3)	MULSCALE(4)
MULSCALE(5)	MULSCALE(6)	MULSCALE(7)	MULSCALE(8)
MULSCALE(9)	MULSCALE(10)	MULSCALE(11)	MULSCALE(12)
MULSCALE(13)	MULSCALE(14)	MULSCALE(15)	MULSCALE(16)
MULSCALE(17)	MULSCALE(18)	MULSCALE(19)	MULSCALE(20)
MULSCALE(21)	MULSCALE(22)	MULSCALE(23)	MULSCALE(24)
MULSCALE(25)	MULSCALE(26)	MULSCALE(27)	MULSCALE(28)
MULSCALE(29)	MULSCALE(30)	MULSCALE(31)
#undef MULSCALE	
static __inline int mulscale32(int a, int d)
{
	_asm {
		mov eax, a
		imul d
		mov eax, edx
	}
}

static __inline int dmulscale(int a, int d, int S, int D, int c)
{
	_asm {
		mov ecx, c
		mov eax, a
		imul d
		mov ebx, eax
		mov eax, S
		mov esi, edx
		imul D
		add eax, ebx
		adc edx, esi
		shrd eax, edx, cl
	}
}

#define DMULSCALE(x) \
static __inline int dmulscale##x (int a, int d, int S, int D) \
{ \
	_asm mov eax, a \
	_asm imul d \
	_asm mov ebx, eax \
	_asm mov eax, S \
	_asm mov esi, edx \
	_asm imul D \
	_asm add eax, ebx \
	_asm adc edx, esi \
	_asm shrd eax, edx, x \
}

DMULSCALE(1)	DMULSCALE(2)	DMULSCALE(3)	DMULSCALE(4)
DMULSCALE(5)	DMULSCALE(6)	DMULSCALE(7)	DMULSCALE(8)
DMULSCALE(9)	DMULSCALE(10)	DMULSCALE(11)	DMULSCALE(12)
DMULSCALE(13)	DMULSCALE(14)	DMULSCALE(15)	DMULSCALE(16)
DMULSCALE(17)	DMULSCALE(18)	DMULSCALE(19)	DMULSCALE(20)
DMULSCALE(21)	DMULSCALE(22)	DMULSCALE(23)	DMULSCALE(24)
DMULSCALE(25)	DMULSCALE(26)	DMULSCALE(27)	DMULSCALE(28)
DMULSCALE(29)	DMULSCALE(30)	DMULSCALE(31)
#undef DMULSCALE	
static __inline int dmulscale32(int a, int d, int S, int D)
{
	_asm {
		mov eax, a
		imul d
		mov ebx, eax
		mov eax, S
		mov esi, edx
		imul D
		add eax, ebx
		adc edx, esi
		mov eax, edx
	}
}

#define TMULSCALE(x) \
static __inline int tmulscale##x (int a, int d, int b, int c, int S, int D) \
{ \
	_asm mov eax, a \
	_asm mov ebx, b \
	_asm imul d \
	_asm xchg eax, ebx \
	_asm mov ecx, c \
	_asm xchg edx, ecx \
	_asm imul edx \
	_asm add ebx, eax \
	_asm adc ecx, edx \
	_asm mov eax, S \
	_asm imul D \
	_asm add eax, ebx \
	_asm adc edx, ecx \
	_asm shrd eax, edx, x \
}

TMULSCALE(1)	TMULSCALE(2)	TMULSCALE(3)	TMULSCALE(4)
TMULSCALE(5)	TMULSCALE(6)	TMULSCALE(7)	TMULSCALE(8)
TMULSCALE(9)	TMULSCALE(10)	TMULSCALE(11)	TMULSCALE(12)
TMULSCALE(13)	TMULSCALE(14)	TMULSCALE(15)	TMULSCALE(16)
TMULSCALE(17)	TMULSCALE(18)	TMULSCALE(19)	TMULSCALE(20)
TMULSCALE(21)	TMULSCALE(22)	TMULSCALE(23)	TMULSCALE(24)
TMULSCALE(25)	TMULSCALE(26)	TMULSCALE(27)	TMULSCALE(28)
TMULSCALE(29)	TMULSCALE(30)	TMULSCALE(31)
#undef TMULSCALE	
static __inline int tmulscale32(int a, int d, int b, int c, int S, int D)
{
	_asm {
		mov eax, a
		mov ebx, b
		imul d
		xchg eax, ebx
		mov ecx, c
		xchg edx, ecx
		imul edx
		add ebx, eax
		adc ecx, edx
		mov eax, S
		imul D
		add eax, ebx
		adc edx, ecx
		mov eax, edx
	}
}

static __inline int boundmulscale(int a, int b, int c)
{
	_asm {
		mov eax, a
		mov ecx, c
		imul b
		mov ebx, edx
		shrd eax, edx, cl
		sar edx, cl
		xor edx, eax
		js checkit
		xor edx, eax
		jz skipboundit
		cmp edx, 0xffffffff
		je skipboundit
	checkit:
		mov eax, ebx
		sar eax, 31
		xor eax, 0x7fffffff
	skipboundit:
	}
}

static __inline int divscale(int a, int b, int c)
{
	_asm {
		mov eax, a
		mov ecx, c
		mov edx, eax
		shl eax, cl
		neg cl
		sar edx, cl
		idiv b
	}
}

static __inline int divscale1(int a, int b)
{
	_asm {
		mov eax, a
		add eax, eax
		sbb edx, edx
		idiv b
	}
}

static __inline int divscale2(int a, int b)
{
	_asm {
		mov eax, a
		mov edx, eax
		sar edx, 30
		lea eax, [eax*4]
		idiv b
	}
}

static __inline int divscale3(int a, int b)
{
	_asm {
		mov eax, a
		mov edx, eax
		sar edx, 29
		lea eax, [eax*8]
		idiv b
	}
}

#define DIVSCALE(x,y) \
static __inline int divscale##y(int a, int b) \
{ \
	_asm mov eax, a \
	_asm mov edx, eax \
	_asm sar edx, x \
	_asm shl eax, y \
	_asm idiv b \
}

DIVSCALE(28,4)	DIVSCALE(27,5)	DIVSCALE(26,6)	DIVSCALE(25,7)
DIVSCALE(24,8)	DIVSCALE(23,9)	DIVSCALE(22,10)	DIVSCALE(21,11)
DIVSCALE(20,12)	DIVSCALE(19,13)	DIVSCALE(18,14)	DIVSCALE(17,15)
DIVSCALE(16,16)	DIVSCALE(15,17)	DIVSCALE(14,18)	DIVSCALE(13,19)
DIVSCALE(12,20)	DIVSCALE(11,21)	DIVSCALE(10,22)	DIVSCALE(9,23)
DIVSCALE(8,24)	DIVSCALE(7,25)	DIVSCALE(6,26)	DIVSCALE(5,27)
DIVSCALE(4,28)	DIVSCALE(3,29)	DIVSCALE(2,30)	DIVSCALE(1,31)

static __inline int divscale32(int d, int b)
{
	_asm {
		mov edx, d
		xor eax, eax
		idiv b
	}
}

static __inline char readpixel(void *d)
{
	_asm {
		mov edx, d
		mov al, byte ptr [edx]
	}
}

static __inline void drawpixel(void *d, char a)
{
	_asm {
		mov edx, d
		mov al, a
		mov byte ptr [edx], al
	}
}

static __inline void drawpixels(void *d, short a)
{
	_asm {
		mov edx, d
		mov ax, a
		mov word ptr [edx], ax
	}
}

static __inline void drawpixelses(void *d, int a)
{
	_asm {
		mov edx, d
		mov eax, a
		mov dword ptr [edx], eax
	}
}

static __inline void clearbuf(void *d, int c, int a)
{
	_asm {
		mov edi, d
		mov ecx, c
		mov eax, a
		rep stosd
	}
}

static __inline void clearbufbyte(void *d, int c, int a)
{
	_asm {
		mov edi, d
		mov ecx, c
		mov eax, a
		cmp ecx, 4
		jae longcopy
		test cl, 1
		jz preskip
		stosb
	preskip:
		shr ecx, 1
		rep stosw
		jmp endit
	longcopy:
		test edi, 1
		jz skip1
		stosb
		dec ecx
	skip1:
		test edi, 2
		jz skip2
		stosw
		sub ecx, 2
	skip2:
		mov ebx, ecx
		shr ecx, 2
		rep stosd
		test bl, 2
		jz skip3
		stosw
	skip3:
		test bl, 1
		jz endit
		stosb
	endit:
	}
}

static __inline void copybuf(void *s, void *d, int c)
{
	_asm {
		mov esi, s
		mov edi, d
		mov ecx, c
		rep movsd
	}
}

static __inline void copybufbyte(void *s, void *d, int c)
{
	_asm {
		mov esi, s
		mov edi, d
		mov ecx, c
		cmp ecx, 4
		jae longcopy
		test cl, 1
		jz preskip
		movsb
	preskip:
		shr ecx, 1
		rep movsw
		jmp endit
	longcopy:
		test edi, 1
		jz skip1
		movsb
		dec ecx
	skip1:
		test edi, 2
		jz skip2
		movsw
		sub ecx, 2
	skip2:
		mov ebx, ecx
		shr ecx, 2
		rep movsd
		test bl, 2
		jz skip3
		movsw
	skip3:
		test bl, 1
		jz endit
		movsb
	endit:
	}
}

static __inline void copybufreverse(void *s, void *d, int c)
{
	_asm {
		mov esi, s
		mov edi, d
		mov ecx, c
		shr ecx, 1
		jnc skipit1
		mov al, byte ptr [esi]
		dec esi
		mov byte ptr [edi], al
		inc edi
	skipit1:
		shr ecx, 1
		jnc skipit2
		mov ax, word ptr [esi-1]
		sub esi, 2
		ror ax, 8
		mov word ptr [edi], ax
		add edi, 2
	skipit2:
		test ecx, ecx
		jz endloop
	begloop:
		mov eax, dword ptr [esi-3]
		sub esi, 4
		bswap eax
		mov dword ptr [edi], eax
		add edi, 4
		dec ecx
		jnz begloop
	endloop:
	}
}

static __inline void qinterpolatedown16(void *a, int c, int d, int s)
{
	_asm {
		mov eax, a
		mov ecx, c
		mov edx, d
		mov esi, s
		mov ebx, ecx
		shr ecx, 1
		jz skipbegcalc
	begqcalc:
		lea edi, [edx+esi]
		sar edx, 16
		mov dword ptr [eax], edx
		lea edx, [edi+esi]
		sar edi, 16
		mov dword ptr [eax+4], edi
		add eax, 8
		dec ecx
		jnz begqcalc
		test ebx, 1
		jz skipbegqcalc2
	skipbegcalc:
		sar edx, 16
		mov dword ptr [eax], edx
	skipbegqcalc2:
	}
}

static __inline void qinterpolatedown16short(void *a, int c, int d, int s)
{
	_asm {
		mov eax, a
		mov ecx, c
		mov edx, d
		mov esi, s
		test ecx, ecx
		jz endit
		test al, 2
		jz skipalignit
		mov ebx, edx
		sar ebx, 16
		mov word ptr [eax], bx
		add edx, esi
		add eax, 2
		dec ecx
		jz endit
	skipalignit:
		sub ecx, 2
		jc finishit
	begqcalc:
		mov ebx, edx
		add edx, esi
		sar ebx, 16
		mov edi, edx
		and edi, 0ffff0000h
		add edx, esi
		add ebx, edi
		mov dword ptr [eax], ebx
		add eax, 4
		sub ecx, 2
		jnc begqcalc
		test cl, 1
		jz endit
	finishit:
		mov ebx, edx
		sar ebx, 16
		mov word ptr [eax], bx
	endit:
	}
}

static __inline int mul3(int a)
{
	_asm {
		mov eax, a
		lea eax, [eax+eax*2]
	}
}

static __inline int mul5(int a)
{
	_asm {
		mov eax, a
		lea eax, [eax+eax*4]
	}
}

static __inline int mul9(int a)
{
	_asm {
		mov eax, a
		lea eax, [eax+eax*8]
	}
}

	//returns eax/ebx, dmval = eax%edx;
static __inline int divmod(int a, int b)
{
	_asm {
		mov eax, a
		xor edx, edx
		div b
		mov dmval, edx
	}
}

	//returns eax%ebx, dmval = eax/edx;
static __inline int moddiv(int a, int b)
{
	_asm {
		mov eax, a
		xor edx, edx
		div b
		mov dmval, eax
		mov eax, edx
	}
}

static __inline int klabs(int a)
{
	_asm {
		mov eax, a
		test eax, eax
		jns skipnegate
		neg eax
	skipnegate:
	}
}

static __inline int ksgn(int b)
{
	_asm {
		mov ebx, b
		add ebx, ebx
		sbb eax, eax
		cmp eax, ebx
		adc al, 0
	}
}

	//eax = (unsigned min)umin(eax,ebx)
static __inline int umin(int a, int b)
{
	_asm {
		mov eax, a
		sub eax, b
		sbb ecx, ecx
		and eax, ecx
		add eax, b
	}
}

	//eax = (unsigned max)umax(eax,ebx)
static __inline int umax(int a, int b)
{
	_asm {
		mov eax, a
		sub eax, b
		sbb ecx, ecx
		xor ecx, 0xffffffff
		and eax, ecx
		add eax, b
	}
}

static __inline int kmin(int a, int b)
{
	_asm {
		mov eax, a
		mov ebx, b
		cmp eax, ebx
		jl skipit
		mov eax, ebx
	skipit:
	}
}

static __inline int kmax(int a, int b)
{
	_asm {
		mov eax, a
		mov ebx, b
		cmp eax, ebx
		jg skipit
		mov eax, ebx
	skipit:
	}
}

static __inline void swapchar(void *a, void *b)
{
	_asm {
		mov eax, a
		mov ebx, b
		mov cl, [eax]
		mov ch, [ebx]
		mov [ebx], cl
		mov [eax], ch
	}
}

static __inline void swapshort(void *a, void *b)
{
	_asm {
		mov eax, a
		mov ebx, b
		mov cx, [eax]
		mov dx, [ebx]
		mov [ebx], cx
		mov [eax], dx
	}
}

static __inline void swaplong(void *a, void *b)
{
	_asm {
		mov eax, a
		mov ebx, b
		mov ecx, [eax]
		mov edx, [ebx]
		mov [ebx], ecx
		mov [eax], edx
	}
}

static __inline void swapbuf4(void *a, void *b, int c)
{
	_asm {
		mov eax, a
		mov ebx, b
		mov ecx, c
	begswap:
		mov esi, [eax]
		mov edi, [ebx]
		mov [ebx], esi
		mov [eax], edi
		add eax, 4
		add ebx, 4
		dec ecx
		jnz short begswap
	}
}

static __inline void swap64bit(void *a, void *b)
{
	_asm {
		mov eax, a
		mov ebx, b
		mov ecx, [eax]
		mov edx, [ebx]
		mov [ebx], ecx
		mov ecx, [eax+4]
		mov [eax], edx
		mov edx, [ebx+4]
		mov [ebx+4], ecx
		mov [eax+4], edx
	}
}

	//swapchar2(ptr1,ptr2,xsiz); is the same as:
	//swapchar(ptr1,ptr2); swapchar(ptr1+1,ptr2+xsiz);
static __inline void swapchar2(void *a, void *b, int s)
{
	_asm {
		mov eax, a
		mov ebx, b
		mov esi, s
		add esi, ebx
		mov cx, [eax]
		mov dl, [ebx]
		mov [ebx], cl
		mov dh, [esi]
		mov [esi], ch
		mov [eax], dx
	}
}
//}}}

#else				// _MSC_VER

//
// Generic C
//

#define qw(x)	((int64_t)(x))		// quadword cast
#define dw(x)	((int32_t)(x))		// doubleword cast
#define wo(x)	((int16_t)(x))		// word cast
#define by(x)	((int8_t)(x))		// byte cast

#define _scaler(a) \
static inline int mulscale##a(int eax, int edx) \
{ \
	return dw((qw(eax) * qw(edx)) >> a); \
} \
\
static inline int divscale##a(int eax, int ebx) \
{ \
	return dw((qw(eax) << a) / qw(ebx)); \
} \
\
static inline int dmulscale##a(int eax, int edx, int esi, int edi) \
{ \
	return dw(((qw(eax) * qw(edx)) + (qw(esi) * qw(edi))) >> a); \
} \
\
static inline int tmulscale##a(int eax, int edx, int ebx, int ecx, int esi, int edi) \
{ \
	return dw(((qw(eax) * qw(edx)) + (qw(ebx) * qw(ecx)) + (qw(esi) * qw(edi))) >> a); \
} \

_scaler(1)	_scaler(2)	_scaler(3)	_scaler(4)
_scaler(5)	_scaler(6)	_scaler(7)	_scaler(8)
_scaler(9)	_scaler(10)	_scaler(11)	_scaler(12)
_scaler(13)	_scaler(14)	_scaler(15)	_scaler(16)
_scaler(17)	_scaler(18)	_scaler(19)	_scaler(20)
_scaler(21)	_scaler(22)	_scaler(23)	_scaler(24)
_scaler(25)	_scaler(26)	_scaler(27)	_scaler(28)
_scaler(29)	_scaler(30)	_scaler(31)	_scaler(32)

static inline void swapchar(void* a, void* b)  { int8_t t = *((int8_t*)b); *((int8_t*)b) = *((int8_t*)a); *((int8_t*)a) = t; }
static inline void swapchar2(void* a, void* b, int s) { swapchar(a,b); swapchar((int8_t*)a+1, (int8_t*)b+s); }
static inline void swapshort(void* a, void* b) { int16_t t = *((int16_t*)b); *((int16_t*)b) = *((int16_t*)a); *((int16_t*)a) = t; }
static inline void swaplong(void* a, void* b)  { int32_t t = *((int32_t*)b); *((int32_t*)b) = *((int32_t*)a); *((int32_t*)a) = t; }
static inline void swap64bit(void* a, void* b) { int64_t t = *((int64_t*)b); *((int64_t*)b) = *((int64_t*)a); *((int64_t*)a) = t; }

static inline int8_t readpixel(void* s)    { return (*((int8_t*)(s))); }
static inline void drawpixel(void* s, int8_t a)    { *((int8_t*)(s)) = a; }
static inline void drawpixels(void* s, int16_t a)  { *((int16_t*)(s)) = a; }
static inline void drawpixelses(void* s, int32_t a) { *((int32_t*)(s)) = a; }

static inline int mul3(int a) { return (a<<1)+a; }
static inline int mul5(int a) { return (a<<2)+a; }
static inline int mul9(int a) { return (a<<3)+a; }

static inline int divmod(int a, int b) { unsigned int _a=(unsigned int)a, _b=(unsigned int)b; dmval = _a%_b; return _a/_b; }
static inline int moddiv(int a, int b) { unsigned int _a=(unsigned int)a, _b=(unsigned int)b; dmval = _a/_b; return _a%_b; }

static inline int klabs(int a) { if (a < 0) return -a; return a; }
static inline int ksgn(int a)  { if (a > 0) return 1; if (a < 0) return -1; return 0; }

static inline int umin(int a, int b) { if ((unsigned int)a < (unsigned int)b) return a; return b; }
static inline int umax(int a, int b) { if ((unsigned int)a < (unsigned int)b) return b; return a; }
static inline int kmin(int a, int b) { if ((signed int)a < (signed int)b) return a; return b; }
static inline int kmax(int a, int b) { if ((signed int)a < (signed int)b) return b; return a; }

static inline int sqr(int eax) { return (eax) * (eax); }
static inline int scale(int eax, int edx, int ecx) { return dw((qw(eax) * qw(edx)) / qw(ecx)); }
static inline int mulscale(int eax, int edx, int ecx) { return dw((qw(eax) * qw(edx)) >> by(ecx)); }
static inline int divscale(int eax, int ebx, int ecx) { return dw((qw(eax) << by(ecx)) / qw(ebx)); }
static inline int dmulscale(int eax, int edx, int esi, int edi, int ecx) { return dw(((qw(eax) * qw(edx)) + (qw(esi) * qw(edi))) >> by(ecx)); }

static inline int boundmulscale(int a, int d, int c)
{ // courtesy of Ken
    int64_t p;
    p = (((int64_t)a)*((int64_t)d))>>c;
    if (p >= INT_MAX) p = INT_MAX;
    if (p < INT_MIN) p = INT_MIN;
    return((int)p);
}

#undef qw
#undef dw
#undef wo
#undef by
#undef _scaler

void qinterpolatedown16 (void *bufptr, int num, int val, int add);
void qinterpolatedown16short (void *bufptr, int num, int val, int add);

void clearbuf(void* d, int c, int a);
void copybuf(void* s, void* d, int c);
void swapbuf4(void* a, void* b, int c);

void clearbufbyte(void *D, int c, int a);
void copybufbyte(void *S, void *D, int c);
void copybufreverse(void *S, void *D, int c);

#endif

#ifdef __cplusplus
}
#endif

#endif // __pragmas_h__

