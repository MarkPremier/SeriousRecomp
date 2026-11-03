/* Execute Sam's actual generated directional-light mixer over nonrepeating
 * shadow bits, including byte boundaries, sub-byte starts and padded rows. */
#define RECOMP_GENERATED_CODE
#include "recomp_types.h"
#include <stdio.h>
#include <stdlib.h>

ptrdiff_t g_xbox_mem_offset;
RECOMP_TLS uint32_t g_eax,g_ecx,g_edx,g_esp,g_ebx,g_esi,g_edi,g_ebp,g_seh_ebp;
RECOMP_TLS RecompMmx g_mm0,g_mm1,g_mm2,g_mm3,g_mm4,g_mm5,g_mm6,g_mm7;
#include "sam_lightmap_mask.inc"

static uint32_t saturated_add(uint32_t a,uint32_t b)
{
    uint32_t result=0;
    for(unsigned shift=0;shift<32;shift+=8) {
        unsigned sum=((a>>shift)&255)+((b>>shift)&255);
        result|=(sum>255?255:sum)<<shift;
    }
    return result;
}

int main(void)
{
    static const uint8_t mask[]={0x80,0x01,0x5A,0xA5,0x0F,0xF0,0x33,0xCC,0x12};
    unsigned failures=0;
    g_xbox_mem_offset=(ptrdiff_t)calloc(1,0x800000);
    if(!g_xbox_mem_offset)return 2;
    memcpy((void *)(g_xbox_mem_offset+0x30000),mask,sizeof mask);
    for(unsigned first=0;first<8;++first) {
        for(unsigned i=0;i<66;++i)MEM32(0x40000+i*4)=0xCDCDCDCD;
        for(unsigned y=0;y<3;++y)for(unsigned x=0;x<19;++x)
            MEM32(0x40000+(y*22+x)*4)=0xFF0A0A0A;
        MEM32(0x20060)=0x50403000; /* directional colour, title's byte order */
        MEM32(0x20064)=0x09070500; /* ambient */
        MEM32(0x4CD0B4)=0x40000;MEM32(0x4CD0E8)=3;
        MEM32(0x4CD0EC)=19;MEM32(0x4CD0E4)=12;
        g_esp=0x70000;MEM32(g_esp)=0;
        MEM32(g_esp+4)=0x30000;MEM32(g_esp+8)=1u<<first;
        g_ecx=0x20000;g_ebx=11;g_esi=12;g_edi=13;
        sub_00294650();
        if(g_esp!=0x7000C || g_ebx!=11 || g_esi!=12 || g_edi!=13)++failures;
        for(unsigned y=0;y<3;++y)for(unsigned x=0;x<22;++x) {
            uint32_t expected=0xCDCDCDCD;
            if(x<19) {
                unsigned bit=first+y*19+x;
                expected=saturated_add(0xFF0A0A0A,0x00050709);
                if(mask[bit/8]&(1u<<(bit%8)))expected=saturated_add(expected,0x00304050);
            }
            uint32_t actual=MEM32(0x40000+(y*22+x)*4);
            if(actual!=expected && ++failures<8)
                printf("start %u pixel %u,%u: %08X expected %08X\n",first,x,y,actual,expected);
        }
    }
    free((void *)g_xbox_mem_offset);
    printf("Sam shadow-mask mixer: 456 lit/shadow pixels, row guards and guest ABI; %u failures\n",failures);
    return failures?1:0;
}
