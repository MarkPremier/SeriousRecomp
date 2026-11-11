#define RECOMP_GENERATED_CODE
#include "recomp_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <float.h>

ptrdiff_t g_xbox_mem_offset;
RECOMP_TLS uint32_t g_eax,g_ecx,g_edx,g_esp,g_ebx,g_esi,g_edi,g_ebp,g_seh_ebp,g_eflags;
RECOMP_TLS double g_fp_stack[8];
RECOMP_TLS int g_fp_top,g_fp_cmp;
RECOMP_TLS uint16_t g_fp_control_word,g_fp_cc;
void native_math_reference(unsigned,const double *,double *,uint16_t *);
void sub_002BB00E(void);void sub_002BB01B(void);void sub_002BB113(void);void sub_002BAF9C(void);
#define eax g_eax
#define ecx g_ecx
#define edx g_edx
#define esp g_esp
#define ebx g_ebx
#define esi g_esi
#define edi g_edi
#undef RECOMP_ABI_CALL
#define RECOMP_ABI_CALL(va,fn) (fn)()
#include "sam_math_safety.inc"
void sub_002BB00E(void) {g_esp+=8;}
void sub_002BB01B(void) {g_esp+=8;}
void sub_002BB113(void) {g_esp+=4;}
void sub_002BAF9C(void) {g_esp+=4;}

static int same(double a,double b) {
    if(isnan(a)||isnan(b))return isnan(a)&&isnan(b);
    if(a==0 && b==0)return signbit(a)==signbit(b);
    return a==b || (isfinite(a)&&isfinite(b)&&fabs(a-b)<=1e-12*fmax(fabs(b),1e-300));
}
static unsigned failures,checks;
static void check(int ok,const char *what,double a,double b) {
    ++checks;if(!ok && ++failures<=12)printf("FAIL %s: %.17g / %.17g\n",what,a,b);
}
int main(void) {
    const double angles[]={0,-0.0,.5,-.5,1,-1,3.14,0x1.fffffffffffffp62,-0x1.fffffffffffffp62,
        1e20,-1e20,0x1p63,-0x1p63,NAN,INFINITY,-INFINITY};
    const double bases[]={0,-0.0,1,-1,DBL_MAX,DBL_MIN,INFINITY,-INFINITY,NAN};
    const double scales[]={0,3.7,-3.7,0x1p31,-0x1p31,1e300,-1e300,INFINITY,-INFINITY,NAN};
    g_xbox_mem_offset=(ptrdiff_t)calloc(1,0x800000);if(!g_xbox_mem_offset)return 2;
    for(unsigned op=0;op<5;++op)for(unsigned i=0;i<(op==4?sizeof bases/sizeof bases[0]:sizeof angles/sizeof angles[0]);++i)
    for(unsigned j=0;j<(op==4?sizeof scales/sizeof scales[0]:1);++j) {
        double input[]={op==4?bases[i]:angles[i],scales[j]},output[2]={0,0};uint16_t status=0;
        native_math_reference(op,input,output,&status);
        g_fp_top=op==4?6:7;g_fp_stack[g_fp_top]=input[0];g_fp_stack[(g_fp_top+1)&7]=input[1];
        g_fp_cmp=2;g_fp_cc=0x400;g_fp_control_word=0x37F;g_eflags=0x246;
        emitted_math(op);
        if(op!=4) {
            check((g_fp_cc&0x400)==(status&0x400),"trig completion",op,input[0]);
            check(g_fp_top==((status>>11)&7),"trig stack",op,input[0]);
        }
        if(op==4 && !same(g_fp_stack[g_fp_top],output[0]))
            printf("SCALE %.17g by %.17g: translated %.17g, CPU %.17g\n",input[0],input[1],g_fp_stack[g_fp_top],output[0]);
        if(op==4 || !isfinite(input[0]) || fabs(input[0])<10 || (status&0x400)) {
            check(same(g_fp_stack[g_fp_top],output[0]),"math result",input[0],input[1]);
            if(op>=2 && op<=3 && !(status&0x400))check(same(g_fp_stack[(g_fp_top+1)&7],output[1]),"second result",op,input[0]);
        }
        check(g_eflags==0x246,"x87 preserves integer flags",op,input[0]);
    }
    MEM16(0x3BC1A4)=0x27F;MEM32(0x4CE954)=0;
    recomp_store_ext80((void *)XBOX_PTR(0x3BC1A6),6.2831853071795864769);
    for(unsigned seed=0;seed<2;++seed) {
        g_fp_top=7;g_fp_stack[7]=.5;g_fp_cc=seed?0x400:0;g_fp_control_word=0x27F;
        g_esp=0x10000;sub_002B50F1();
        check(same(g_fp_stack[g_fp_top],sin(.5)),"real CRT sine with stale C2",seed,g_fp_stack[g_fp_top]);
    }
    const uint32_t values[]={0,1,0x80,0x8000,0x80000000,0xFFFFFFFF,0x12345678};
    for(unsigned kind=0;kind<2;++kind)for(unsigned bits=8;bits<=32;bits*=2)
    for(unsigned c=0;c<256;++c)for(unsigned v=0;v<sizeof values/sizeof values[0];++v) {
        uint32_t mask=bits==32?UINT32_MAX:(1u<<bits)-1,n=c&31u,initial=values[v],low=initial&mask;
        uint32_t value=kind?low>>n:low<<n,want=(initial&~mask)|(value&mask);unsigned carry;
        g_eax=initial;g_ecx=c;uint32_t got=emitted_shift(kind,bits,c,1,&carry);
        check(got==want,"logical shift count/value",kind,c);
        if(!n)check(carry==1,"zero shift carry preserved",kind,c);
        else if(n<bits || (kind && n==bits)) {
            unsigned expected=(kind?low>>(n-1):low>>(bits-n))&1u;
            check(carry==expected,"logical shift carry",kind,c);
        }
    }
    g_eax=0xE0000000;check(emitted_immediate_shift()==7,"immediate count 61 masks to 29",0,0);
    free((void *)g_xbox_mem_offset);
    printf("Math safety: %u hardware/model and boundary checks, %u failures\n",checks,failures);return failures?1:0;
}
