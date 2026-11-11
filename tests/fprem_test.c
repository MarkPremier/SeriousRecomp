/* The combat freeze sat in this real CRT FPREM/FNSTSW/SAHF/JP loop. */
#define RECOMP_GENERATED_CODE
#include "recomp_types.h"
#include <stdio.h>
#include <stdlib.h>

ptrdiff_t g_xbox_mem_offset;
RECOMP_TLS uint32_t g_eax,g_ecx,g_edx,g_esp,g_ebx,g_esi,g_edi,g_ebp,g_seh_ebp;
RECOMP_TLS uint32_t g_eflags;
RECOMP_TLS double g_fp_stack[8];
RECOMP_TLS int g_fp_top,g_fp_cmp;
RECOMP_TLS uint16_t g_fp_control_word,g_fp_cc;
#include "sam_fprem.inc"

int main(void)
{
    const double cases[][2]={{0,360},{9,4},{-9,4},{9,-4},{-9,-4},{1e300,3},{1e-300,3e-310},
        {0,4},{-0.0,4},{INFINITY,4},{NAN,4},{3,0},{3,INFINITY}};
    const int old_compares[]={2,-1,0,1};
    const uint16_t old_status[]={0x4500,0,0x100,0x400};
    unsigned failures=0,count=0;
    for(unsigned c=0;c<sizeof cases/sizeof cases[0];++c)
    for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j) {
        double expected=fmod(cases[c][0],cases[c][1]);
        for(unsigned k=0;k<8;++k)g_fp_stack[k]=12345;
        g_fp_top=3;g_fp_stack[3]=cases[c][1];g_fp_stack[4]=cases[c][0];
        g_fp_cmp=old_compares[i];g_fp_cc=old_status[j];g_fp_control_word=0x37F;
        g_eax=0xCAFE0000;g_esp=0x1000;g_ebx=11;g_esi=22;g_edi=33;
        sub_002B6724();
        double result=g_fp_stack[g_fp_top];
        if(g_fp_top!=4 || g_esp!=0x1004 || g_ebx!=11 || g_esi!=22 || g_edi!=33 ||
           g_fp_control_word!=0x37F || (g_eax>>16)!=0xCAFE || (g_eax&0x400) ||
           !(isnan(expected)?isnan(result):(result==expected && signbit(result)==signbit(expected))))++failures;
        for(unsigned k=0;k<8;++k)if(k!=3 && k!=4 && g_fp_stack[k]!=12345)++failures;
        ++count;
    }
    printf("CRT remainder loop: %u cases including stale unordered/C2 state, signs, large ratios, NaNs and guest ABI; %u failures\n",count,failures);
    return failures?1:0;
}
