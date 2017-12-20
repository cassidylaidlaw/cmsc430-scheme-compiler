
//#include "gc.h"    // Add back in and change tags if we want to use GC
#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include "string.h"
#include "stdbool.h"


// prevent lots of warnings from format differences between Linux and Mac OS
#pragma GCC diagnostic ignored "-Wformat"


#define CLO_TAG 0
#define CONS_TAG 1
#define INT_TAG 2
#define STR_TAG 3
#define SYM_TAG 4
#define OTHER_TAG 6
#define ENUM_TAG 7


#define VECTOR_OTHERTAG 1
#define CHAR_OTHERTAG 2
// Hashes, Sets, gen records, can all be added here


#define V_VOID 39  //32 +7 (+7 is for anything enumerable other than null)
#define V_TRUE 31  //24 +7
#define V_FALSE 15 //8  +7
#define V_NULL 0



#define MASK64 0xffffffffffffffff // useful for tagging related operations


#define ASSERT_TAG(v,tag,msg) \
    if(((v)&7ULL) != (tag)) \
        fatal_err(msg);

#define ASSERT_VALUE(v,val,msg) \
    if(((u64)(v)) != (val))     \
        fatal_err(msg);


#define DECODE_CLO(v) ((u64*)((v)&(7ULL^MASK64)))
#define ENCODE_CLO(v) (((u64)(v)) | CLO_TAG)

#define DECODE_CONS(v) ((u64*)((v)&(7ULL^MASK64)))
#define ENCODE_CONS(v) (((u64)(v)) | CONS_TAG)

#define DECODE_INT(v) ((s32)((u32)(((v)&(7ULL^MASK64)) >> 32)))
#define ENCODE_INT(v) ((((u64)((u32)(v))) << 32) | INT_TAG)

#define DECODE_STR(v) ((char*)((v)&(7ULL^MASK64)))
#define ENCODE_STR(v) (((u64)(v)) | STR_TAG)

#define DECODE_SYM(v) ((char*)((v)&(7ULL^MASK64)))
#define ENCODE_SYM(v) (((u64)(v)) | SYM_TAG)

#define DECODE_OTHER(v) ((u64*)((v)&(7ULL^MASK64)))
#define ENCODE_OTHER(v) (((u64)(v)) | OTHER_TAG)


// some apply-prim macros for expecting 1 argument or 2 arguments
#define GEN_EXPECT1ARGLIST(f,g) \
    u64 f(u64 lst) \
    { \
        u64 v0 = expect_args1(lst); \
        return g(v0); \
    } 

#define GEN_EXPECT2ARGLIST(f,g) \
    u64 f(u64 lst) \
    { \
        u64 rest; \
        u64 v0 = expect_cons(lst, &rest); \
        u64 v1 = expect_cons(rest, &rest); \
        if (rest != V_NULL) \
            fatal_err("prim applied on more than 2 arguments."); \
        return g(v0,v1);                                           \
    } 

#define GEN_EXPECT3ARGLIST(f,g) \
    u64 f(u64 lst) \
    { \
        u64 rest; \
        u64 v0 = expect_cons(lst, &rest); \
        u64 v1 = expect_cons(rest, &rest); \
        u64 v2 = expect_cons(rest, &rest); \
        if (rest != V_NULL) \
            fatal_err("prim applied on more than 2 arguments."); \
        return g(v0,v1,v2);                                        \
    } 





// No mangled names
extern "C"
{



typedef uint64_t u64;
typedef int64_t s64;
typedef uint32_t u32;
typedef int32_t s32;


    
// UTILS


u64* alloc(const u64 m)
{
    return (u64*)(malloc(m));
    //return new u64[m];
    //return (u64*)GC_MALLOC(m);
}

void fatal_err(const char* msg)
{
    printf("library run-time error: ");
    printf("%s", msg);
    printf("\n");
    exit(1);
}

void print_u64(u64 i)
{
    printf("%lu\n", i);
}

u64 expect_args0(u64 args)
{
    if (args != V_NULL)
        fatal_err("Expected value: null (in expect_args0). Prim cannot take arguments.");
    return V_NULL;
}

u64 expect_args1(u64 args)
{
    ASSERT_TAG(args, CONS_TAG, "Expected cons value (in expect_args1). Prim applied on an empty argument list.")
    u64* p = DECODE_CONS(args);
    ASSERT_VALUE((p[1]), V_NULL, "Expected null value (in expect_args1). Prim can only take 1 argument.")
    return p[0];
}

u64 expect_cons(u64 p, u64* rest)
{
    // pass a pair value p and a pointer to a word *rest                          
    // verifiies (cons? p), returns the value (car p) and assigns *rest = (cdr p) 
    ASSERT_TAG(p, CONS_TAG, "Expected a cons value. (expect_cons)")               

    u64* pp = DECODE_CONS(p);
    *rest = pp[1];
    return pp[0];
}

u64 expect_other(u64 v, u64* rest)
{
    // returns the runtime tag value
    // puts the untagged value at *rest
    ASSERT_TAG(v, OTHER_TAG, "Expected a vector or special value. (expect_other)")
    
    u64* p = DECODE_OTHER(v);
    *rest = p[1];
    return p[0];
}


// UTF-8 utils

typedef struct utf8_char utf8_char;
struct utf8_char {
	u64 tag;
	char utf8[5];
};


// based on http://www.zedwood.com/article/cpp-utf8-char-to-codepoint
int utf8_to_codepoint(const char* utf8)
{
    int l = strlen(utf8);
    unsigned char* u = (unsigned char*) utf8;
    if (l<1) return -1; unsigned char u0 = u[0]; if (u0>=0   && u0<=127) return u0;
    if (l<2) return -1; unsigned char u1 = u[1]; if (u0>=192 && u0<=223) return (u0-192)*64 + (u1-128);
    if (u[0]==0xed && (u[1] & 0xa0) == 0xa0) return -1; //code points, 0xd800 to 0xdfff
    if (l<3) return -1; unsigned char u2 = u[2]; if (u0>=224 && u0<=239) return (u0-224)*4096 + (u1-128)*64 + (u2-128);
    if (l<4) return -1; unsigned char u3 = u[3]; if (u0>=240 && u0<=247) return (u0-240)*262144 + (u1-128)*4096 + (u2-128)*64 + (u3-128);
    return -1;
}

// based on http://www.zedwood.com/article/cpp-utf8-strlen-function
int utf8_strlen(const char* str)
{
    int c,i,ix,q;
    for (q=0, i=0, ix=strlen(str); i < ix; i++, q++)
    {
        c = (unsigned char) str[i];
        if      (c>=0   && c<=127) i+=0;
        else if ((c & 0xE0) == 0xC0) i+=1;
        else if ((c & 0xF0) == 0xE0) i+=2;
        else if ((c & 0xF8) == 0xF0) i+=3;
        //else if (($c & 0xFC) == 0xF8) i+=4; // 111110bb //byte 5, unnecessary in 4 byte UTF-8
        //else if (($c & 0xFE) == 0xFC) i+=5; // 1111110b //byte 6, unnecessary in 4 byte UTF-8
        else return 0;//invalid utf8
    }
    return q;
}

// based on http://www.zedwood.com/article/cpp-utf-8-mb_substr-function
const char* utf8_substr(const char* str, unsigned int start, unsigned int leng)
{
	int slen = strlen(str);
    unsigned int c, i, ix, q, min=slen, max=slen;
    for (q=0, i=0, ix=slen; i < ix; i++, q++)
    {
        if (q==start){ min=i; }
        if (q<=start+leng || leng==slen){ max=i; }
 
        c = (unsigned char) str[i];
        if                  (c<=127) i+=0;
        else if ((c & 0xE0) == 0xC0) i+=1;
        else if ((c & 0xF0) == 0xE0) i+=2;
        else if ((c & 0xF8) == 0xF0) i+=3;
        //else if (($c & 0xFC) == 0xF8) i+=4; // 111110bb //byte 5, unnecessary in 4 byte UTF-8
        //else if (($c & 0xFE) == 0xFC) i+=5; // 1111110b //byte 6, unnecessary in 4 byte UTF-8
        else return NULL;//invalid utf8
    }
    if (q<=start+leng || leng==slen){ max=i; }
    int size_to_alloc = max-min+1;
    char* result = (char*) alloc(size_to_alloc * sizeof(char));
    strncpy(result, &str[min], max-min);
    result[max-min] = '\0';
    return result;
}


/////// CONSTANTS
    
    
u64 const_init_int(s64 i)
{
    return ENCODE_INT((s32)i);
}

u64 const_init_void()
{
    return V_VOID;
}


u64 const_init_null()
{
    return V_NULL;
}


u64 const_init_true()
{
    return V_TRUE;
}

    
u64 const_init_false()
{
    return V_FALSE;
}

    
u64 const_init_string(const char* s)
{
    return ENCODE_STR(s);
}
        
u64 const_init_symbol(const char* s)
{
    return ENCODE_SYM(s);
}


u64 const_init_char(const char* s)
{	
    utf8_char* chr = (utf8_char*)alloc(sizeof(utf8_char));
    chr->tag = CHAR_OTHERTAG;
    strcpy(chr->utf8, s);
    return ENCODE_OTHER(chr);
}




/////////// PRIMS

    
///// effectful prims:

    
u64 prim_print_aux(u64 v) 
{
    if (v == V_NULL)
        printf("()");
	else if (v == V_TRUE)
		printf("#t");
	else if (v == V_FALSE)
		printf("#f");
	else if (v == V_VOID)
		printf("#<void>");
    else if ((v&7) == CLO_TAG)
        printf("#<procedure>");
    else if ((v&7) == CONS_TAG)
    {
        u64* p = DECODE_CONS(v);
        printf("(");
        prim_print_aux(p[0]);
        printf(" . ");
        prim_print_aux(p[1]);
        printf(")");
    }
    else if ((v&7) == INT_TAG)
    {
        printf("%d", (int)((s32)(v >> 32)));
    }
    else if ((v&7) == STR_TAG)
    {   // needs to handle escaping to be correct
        printf("\"%s\"", DECODE_STR(v));
    }
    else if ((v&7) == SYM_TAG)
    {   // needs to handle escaping to be correct
        printf("%s", DECODE_SYM(v));
    }
    else if ((v&7) == OTHER_TAG)
    {
    	u64 othertag = (*((u64*)DECODE_OTHER(v)) & 7);
    	if (othertag == VECTOR_OTHERTAG)
    	{
			printf("#(");
			u64* vec = (u64*)DECODE_OTHER(v);
			u64 len = vec[0] >> 3;
			if (len > 0) {
				prim_print_aux(vec[1]);
			}
			for (u64 i = 2; i <= len; ++i)
			{
				printf(" ");
				prim_print_aux(vec[i]);
			}
			printf(")");
		}
    	else if (othertag == CHAR_OTHERTAG)
    	{
    		utf8_char* chr = (utf8_char*)DECODE_OTHER(v);
    		int chr_codepoint = utf8_to_codepoint(chr->utf8);
    		printf("#\\u%X", chr_codepoint);
    	}
    	else
    		printf("(print v); unrecognized other tag %lu", othertag);
    }
    else
        printf("(print.. v); unrecognized value %lu", v);
    //...
    return V_VOID; 
}

u64 prim_print(u64 v) 
{
    if (v == V_NULL)
        printf("'()");
	else if (v == V_TRUE)
		printf("#t");
	else if (v == V_FALSE)
		printf("#f");
	else if (v == V_VOID)
		printf("#<void>");
    else if ((v&7) == CLO_TAG)
        printf("#<procedure>");
    else if ((v&7) == CONS_TAG)
    {
        u64* p = (u64*)(v&(7ULL^MASK64));
        printf("'(");
        prim_print_aux(p[0]);
        printf(" . ");
        prim_print_aux(p[1]);
        printf(")");
    }
    else if ((v&7) == INT_TAG)
    {
        printf("%d", ((s32)(v >> 32)));
    }
    else if ((v&7) == STR_TAG)
    {   // needs to handle escaping to be correct
        printf("\"%s\"", DECODE_STR(v));
    }
    else if ((v&7) == SYM_TAG)
    {   // needs to handle escaping to be correct
        printf("'%s", DECODE_SYM(v));
    }
    else if ((v&7) == OTHER_TAG)
    {
    	u64 othertag = (*((u64*)DECODE_OTHER(v)) & 7);
    	if (othertag == VECTOR_OTHERTAG)
    	{
			printf("#(");
			u64* vec = (u64*)DECODE_OTHER(v);
			u64 len = vec[0] >> 3;
			if (len > 0) {
				prim_print(vec[1]);
			}
			for (u64 i = 2; i <= len; ++i)
			{
				printf(" ");
				prim_print(vec[i]);
			}
			printf(")");
		}
    	else if (othertag == CHAR_OTHERTAG)
    	{
    		utf8_char* chr = (utf8_char*)DECODE_OTHER(v);
    		int chr_codepoint = utf8_to_codepoint(chr->utf8);
    		printf("#\\u%X", chr_codepoint);
    	}
    	else
    		printf("(print v); unrecognized other tag %lu", othertag);
    }
    else
        printf("(print v); unrecognized value %lu", v);
    //...
    return V_VOID; 
}
GEN_EXPECT1ARGLIST(applyprim_print,prim_print)


u64 prim_halt(u64 v) // halt
{
    prim_print(v); // display the final value
    printf("\n");
    exit(0);
    return V_NULL; 
}


u64 applyprim_vector(u64 lst)
{
    // pretty terrible, but works
    u64* buffer = (u64*)malloc(512*sizeof(u64));
    u64 l = 0;
    while ((lst&7) == CONS_TAG && l < 512) 
        buffer[l++] = expect_cons(lst, &lst);
    u64* mem = alloc((l + 1) * sizeof(u64));
    mem[0] = (l << 3) | VECTOR_OTHERTAG;
    for (u64 i = 0; i < l; ++i)
        mem[i+1] = buffer[i];
    delete [] buffer;
    return ENCODE_OTHER(mem);
}



u64 prim_make_45vector(u64 lenv, u64 iv)
{
    ASSERT_TAG(lenv, INT_TAG, "first argument to make-vector must be an integer")
    
    const u64 l = DECODE_INT(lenv);
    u64* vec = (u64*)alloc((l + 1) * sizeof(u64));
    vec[0] = (l << 3) | VECTOR_OTHERTAG;
    for (u64 i = 1; i <= l; ++i)
        vec[i] = iv;
    return ENCODE_OTHER(vec);
}
GEN_EXPECT2ARGLIST(applyprim_make_45vector, prim_make_45vector)


u64 prim_vector_45ref(u64 v, u64 i)
{
    ASSERT_TAG(i, INT_TAG, "second argument to vector-ref must be an integer")
    ASSERT_TAG(v, OTHER_TAG, "first argument to vector-ref must be a vector")

    if ((((u64*)DECODE_OTHER(v))[0]&7) != VECTOR_OTHERTAG)
        fatal_err("vector-ref not given a properly formed vector");

    return ((u64*)DECODE_OTHER(v))[1+(DECODE_INT(i))];
}
GEN_EXPECT2ARGLIST(applyprim_vector_45ref, prim_vector_45ref)


u64 prim_vector_45set_33(u64 a, u64 i, u64 v)
{
    ASSERT_TAG(i, INT_TAG, "second argument to vector-ref must be an integer")
    ASSERT_TAG(a, OTHER_TAG, "first argument to vector-ref must be an integer")

    if ((((u64*)DECODE_OTHER(a))[0]&7) != VECTOR_OTHERTAG)
        fatal_err("vector-ref not given a properly formed vector");

        
    ((u64*)(DECODE_OTHER(a)))[1+DECODE_INT(i)] = v;
        
    return V_VOID;
}
GEN_EXPECT3ARGLIST(applyprim_vector_45set_33, prim_vector_45set_33)


u64 prim_vector_45length(u64 v)
{
    ASSERT_TAG(v, OTHER_TAG, "first argument to vector-length must be a vector")

    if ((((u64*)DECODE_OTHER(v))[0]&7) != VECTOR_OTHERTAG)
        fatal_err("vector-length not given a properly formed vector");

    u64* vec = (u64*)DECODE_OTHER(v);
    u64 len = vec[0] >> 3;
    return ENCODE_INT(len);
}
GEN_EXPECT1ARGLIST(applyprim_vector_45length, prim_vector_45length)


u64 prim_vector_63(u64 a)
{
	if((a&7) == OTHER_TAG) {
		if ((((u64*)DECODE_OTHER(a))[0]&7) == VECTOR_OTHERTAG) {
			return V_TRUE;
		} else {
			return V_FALSE;
		}
	} else {
		return V_FALSE;
	}
}
GEN_EXPECT1ARGLIST(applyprim_vector_63, prim_vector_63)


///// void, ...

    
u64 prim_void()
{
    return V_VOID;
}

u64 applyprim_void(u64 a) {
	return V_VOID;
}
    



///// eq?, eqv?, equal?

    
u64 prim_eq_63(u64 a, u64 b)
{
    if (a == b)
        return V_TRUE;
    else
        return V_FALSE;
}
GEN_EXPECT2ARGLIST(applyprim_eq_63, prim_eq_63)


u64 prim_eqv_63(u64 a, u64 b)
{
    if (a == b)
        return V_TRUE;
    //else if  // optional extra logic, see r7rs reference
    else
        return V_FALSE;
}
GEN_EXPECT2ARGLIST(applyprim_eqv_63, prim_eqv_63)


u64 prim_equal_63(u64 a, u64 b)
{
	int a_tag = a&7ULL, b_tag = b&7ULL;
	
	if (a_tag != b_tag)
		return V_FALSE;
	else if (a_tag == STR_TAG)
		return strcmp(DECODE_STR(a), DECODE_STR(b)) == 0 ? V_TRUE : V_FALSE;
	else
		fatal_err("(equal? a b); equal? is not defined for the given values");
			
    return 0;
}
GEN_EXPECT2ARGLIST(applyprim_equal_63, prim_equal_63)



///// Other predicates


u64 prim_number_63(u64 a)
{
    // We assume that ints are the only number
    if ((a&7) == INT_TAG)
        return V_TRUE;
    else
        return V_FALSE;
}
GEN_EXPECT1ARGLIST(applyprim_number_63, prim_number_63)


u64 prim_integer_63(u64 a)
{
    if ((a&7) == INT_TAG)
        return V_TRUE;
    else
        return V_FALSE;
}
GEN_EXPECT1ARGLIST(applyprim_integer_63, prim_integer_63)


u64 prim_void_63(u64 a)
{
    if (a == V_VOID)
        return V_TRUE;
    else
        return V_FALSE;
}
GEN_EXPECT1ARGLIST(applyprim_void_63, prim_void_63)


u64 prim_procedure_63(u64 a)
{
    if ((a&7) == CLO_TAG)
        return V_TRUE;
    else
        return V_FALSE;
}
GEN_EXPECT1ARGLIST(applyprim_procedure_63, prim_procedure_63)


///// null?, cons?, cons, car, cdr


u64 prim_null_63(u64 p) // null?
{
    if (p == V_NULL)
        return V_TRUE;
    else
        return V_FALSE;
}
GEN_EXPECT1ARGLIST(applyprim_null_63, prim_null_63)    


u64 prim_cons_63(u64 p) // cons?
{
    if ((p&7) == CONS_TAG)
        return V_TRUE;
    else
        return V_FALSE;
}
GEN_EXPECT1ARGLIST(applyprim_cons_63, prim_cons_63)    


u64 prim_cons(u64 a, u64 b)
{
    u64* p = alloc(2*sizeof(u64));
    p[0] = a;
    p[1] = b;
    return ENCODE_CONS(p);
}
GEN_EXPECT2ARGLIST(applyprim_cons, prim_cons)


u64 prim_car(u64 p)
{
    u64 rest;
    u64 v0 = expect_cons(p,&rest);
    
    return v0;
}
GEN_EXPECT1ARGLIST(applyprim_car, prim_car)


u64 prim_cdr(u64 p)
{
    u64 rest;
    u64 v0 = expect_cons(p,&rest);
    
    return rest;
}
GEN_EXPECT1ARGLIST(applyprim_cdr, prim_cdr)


///// s32 prims, +, -, *, =, ...

    
u64 prim__43(u64 a, u64 b) // +
{
    ASSERT_TAG(a, INT_TAG, "(prim + a b); a is not an integer")
    ASSERT_TAG(b, INT_TAG, "(prim + a b); b is not an integer")

        //printf("sum: %d\n", DECODE_INT(a) + DECODE_INT(b));
    
    return ENCODE_INT(DECODE_INT(a) + DECODE_INT(b));
}

u64 applyprim__43(u64 p)
{
    if (p == V_NULL)
        return ENCODE_INT(0);
    else
    {
        ASSERT_TAG(p, CONS_TAG, "Tried to apply + on non list value.")
        u64* pp = DECODE_CONS(p);
        return ENCODE_INT(DECODE_INT(pp[0]) + DECODE_INT(applyprim__43(pp[1])));
    }
}
    
u64 prim__45(u64 a, u64 b) // -
{
    ASSERT_TAG(a, INT_TAG, "(prim + a b); a is not an integer")
    ASSERT_TAG(b, INT_TAG, "(prim - a b); b is not an integer")
    
    return ENCODE_INT(DECODE_INT(a) - DECODE_INT(b));
}

u64 applyprim__45(u64 p)
{
    if (p == V_NULL)
        return ENCODE_INT(0);
    else
    {
        ASSERT_TAG(p, CONS_TAG, "Tried to apply + on non list value.")
        u64* pp = DECODE_CONS(p);
        if (pp[1] == V_NULL)
            return ENCODE_INT(0 - DECODE_INT(pp[0]));
        else // ideally would be properly left-to-right
            return ENCODE_INT(DECODE_INT(pp[0]) - DECODE_INT(applyprim__43(pp[1])));
    }
}
    
u64 prim__42(u64 a, u64 b) // *
{
    ASSERT_TAG(a, INT_TAG, "(prim * a b); a is not an integer")
    ASSERT_TAG(b, INT_TAG, "(prim * a b); b is not an integer")
    
    return ENCODE_INT(DECODE_INT(a) * DECODE_INT(b));
}

u64 applyprim__42(u64 p)
{
    if (p == V_NULL)
        return ENCODE_INT(1);
    else
    {
        ASSERT_TAG(p, CONS_TAG, "Tried to apply + on non list value.")
        u64* pp = DECODE_CONS(p);
        return ENCODE_INT(DECODE_INT(pp[0]) * DECODE_INT(applyprim__42(pp[1])));
    }
}
    
u64 prim__47(u64 a, u64 b) // /
{
    ASSERT_TAG(a, INT_TAG, "(prim / a b); a is not an integer")
    ASSERT_TAG(b, INT_TAG, "(prim / a b); b is not an integer")
    
    return ENCODE_INT(DECODE_INT(a) / DECODE_INT(b));
}

#define GEN_BOOLEAN_OPERATOR(op,name) \
	u64 name(u64 a, u64 b) \
	{ \
		ASSERT_TAG(a, INT_TAG, "(prim < a b); a is not an integer") \
		ASSERT_TAG(b, INT_TAG, "(prim < a b); b is not an integer") \
		if ((s32)((a&(7ULL^MASK64)) >> 32) op (s32)((b&(7ULL^MASK64)) >> 32)) \
			return V_TRUE; \
		else \
			return V_FALSE; \
	} \
	GEN_EXPECT2ARGLIST(apply ## name, name)

GEN_BOOLEAN_OPERATOR(==, prim__61)
GEN_BOOLEAN_OPERATOR(<, prim__60)
GEN_BOOLEAN_OPERATOR(<=, prim__60_61)
GEN_BOOLEAN_OPERATOR(>, prim__62)
GEN_BOOLEAN_OPERATOR(>=, prim__62_61)

u64 prim_not(u64 a) 
{
    if (a == V_FALSE)
        return V_TRUE;
    else
        return V_FALSE;
}
GEN_EXPECT1ARGLIST(applyprim_not, prim_not)


///// string and char prims



u64 concat_strings(u64 l, bool is_chrs)
{
	u64 remainder;
	char* next_str;

	unsigned int total_len = 0;
	remainder = l;
	while (remainder != V_NULL)
	{
		u64 next_elt = expect_cons(remainder, &remainder);
		if (is_chrs)
		{
			ASSERT_TAG(next_elt, OTHER_TAG, "(string c ...); one of c is not a char")
			utf8_char* next_chr = (utf8_char*)DECODE_OTHER(next_elt);
			if (next_chr->tag != CHAR_OTHERTAG)
				fatal_err("(string c ...); one of c is not a char");
			total_len += strlen(next_chr->utf8);
		}
		else
		{
			ASSERT_TAG(next_elt, STR_TAG, "(string-append s ...); one of s is not a string")
			next_str = DECODE_STR(next_elt);
			total_len += strlen(next_str);
		}
	}
	
	char* result = (char*)alloc((total_len + 1) * sizeof(char));
	unsigned int current_pos = 0;
	remainder = l;
	while (remainder != V_NULL)
	{
		u64 next_elt = expect_cons(remainder, &remainder);
		if (is_chrs)
		{
			utf8_char* next_chr = (utf8_char*)DECODE_OTHER(next_elt);
			next_str = next_chr->utf8;
		}
		else
			next_str = DECODE_STR(next_elt);
		strcpy(&result[current_pos], next_str);
		current_pos += strlen(next_str);
	}
	result[current_pos] = '\0';
	
	return ENCODE_STR(result);
}

u64 applyprim_string(u64 l)
{
	return concat_strings(l, true);
}

u64 string_to_list(char* str) {
	if (str[0] == '\0')
		return V_NULL;
	else
	{
		const char* first_chr_str = utf8_substr(str, 0, 1);
		int first_chr_width = strlen(first_chr_str);
		u64 first_chr = const_init_char(first_chr_str);
		
		char* remainder = &str[first_chr_width];
		u64 remainder_list = string_to_list(remainder);
		
		return prim_cons(first_chr, remainder_list);
	}
}

u64 prim_string_45_62list(u64 s)
{
	ASSERT_TAG(s, STR_TAG, "(string->list s); s is not a string")
	
	char* str = DECODE_STR(s);
	return string_to_list(str);
}
GEN_EXPECT1ARGLIST(applyprim_string_45_62list, prim_string_45_62list)

u64 prim_string_45ref(u64 s, u64 n)
{
	ASSERT_TAG(s, STR_TAG, "(string-ref s n); s is not a string")
	ASSERT_TAG(n, INT_TAG, "(string-ref s n); n is not an integer")
	
	char* str = DECODE_STR(s);
	int pos = DECODE_INT(n);
	
	const char* char_str = utf8_substr(str, pos, 1);
	
	return const_init_char(char_str);
}
GEN_EXPECT2ARGLIST(applyprim_string_45ref, prim_string_45ref)

u64 prim_substring(u64 s, u64 n, u64 m)
{
	ASSERT_TAG(s, STR_TAG, "(substring s n m); s is not a string")
	ASSERT_TAG(n, INT_TAG, "(substring s n m); n is not an integer")
	ASSERT_TAG(m, INT_TAG, "(substring s n m); m is not an integer")
	
	char* str = DECODE_STR(s);
	int start = DECODE_INT(n);
	int end = DECODE_INT(m);
	
	if (end == -1) {
		end = utf8_strlen(str);
	}
	
	const char* result_str = utf8_substr(str, start, end-start);
	
	return const_init_string(result_str);
}
u64 applyprim_substring(u64 lst)
{
	u64 rest = lst;
	u64 s = expect_cons(rest, &rest);
	u64 n = expect_cons(rest, &rest);
	u64 m;
	if (rest == V_NULL)
	{
		m = ENCODE_INT(-1);
	}
	else
	{
		m = expect_cons(rest, &rest);
        if (rest != V_NULL)
            fatal_err("substring applied on more than 3 arguments.");
    } 
	return prim_substring(s, n, m);
}

u64 applyprim_string_45append(u64 l)
{
	return concat_strings(l, false);
}

u64 prim_string_45length(u64 s)
{
	ASSERT_TAG(s, STR_TAG, "(string-length s); s is not a string")

	char* decoded_str = DECODE_STR(s);
	return ENCODE_INT(utf8_strlen(decoded_str));
}
GEN_EXPECT1ARGLIST(applyprim_string_45length, prim_string_45length)

u64 prim_string_63(u64 s) // string?
{
	if ((s&7ULL) == STR_TAG)
		return V_TRUE;
	else
		return V_FALSE;
}
GEN_EXPECT1ARGLIST(applyprim_string_63, prim_string_63)

u64 prim_char_63(u64 s) // char?
{
	if ((s&7ULL) == OTHER_TAG)
	{
		utf8_char* chr = (utf8_char*) DECODE_OTHER(s);
		if (chr->tag == CHAR_OTHERTAG)
			return V_TRUE;
		else
			return V_FALSE;
	}
	else
		return V_FALSE;
}
GEN_EXPECT1ARGLIST(applyprim_char_63, prim_char_63)

///// closure utilities


u64 make_clo(u64* f, u64 env)
{
    u64* c = new u64[2];
    c[0] = (u64) f;
    c[1] = env;
    return ENCODE_CLO(c);
}

u64* get_clo_func(u64 c)
{
    ASSERT_TAG(c, CLO_TAG, "trying to apply a value that is not a procedure")
	
	u64* cc = DECODE_CLO(c);
	return (u64*) cc[0];
}

u64 get_clo_env(u64 c)
{
	u64* cc = DECODE_CLO(c);
	return cc[1];
}


}

