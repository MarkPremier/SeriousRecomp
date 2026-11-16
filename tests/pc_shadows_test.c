#include "../src/pc_shadows.c"
#include <string.h>
#define CHECK(x) do {if (!(x)) {fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);}} while(0)
ptrdiff_t g_xbox_mem_offset;
RECOMP_TLS uint32_t g_ecx,g_esi,g_esp;
static float observed;
static unsigned calls;
static const uint32_t stack=0x20000,entity=0x30000,model=0x40000;
static void draw(void) {
    observed=REAL(g_esp+4);
    CHECK(g_ecx==model && g_esi==entity);
    CHECK(WORD(g_esp+8)==0x50000); /* receiver plane is preserved */
    ++calls;g_esp+=12;
}
static void run(float input,float expected) {
    g_esp=stack;REAL(stack+4)=input;WORD(stack+8)=0x50000;
    uint32_t before=WORD(stack+4);
    pc_shadows_simple(draw);
    CHECK(fabsf(observed-expected)<1e-6f);
    CHECK(g_esp==stack+12 && WORD(stack+4)==before);
}
int main(void) {
    g_xbox_mem_offset=(ptrdiff_t)calloc(1,0x80000);CHECK(g_xbox_mem_offset);
    g_ecx=model;g_esi=entity;
    WORD(entity)=BIOMECH_VTABLE;WORD(entity+0x7C)=model;
    WORD(stack)=SKELETAL_SHADOW_CALLER;
    _putenv_s("RECOMP_WALKER_SHADOW_MIN","");pc_shadows_load();
    run(.101960786f,1);run(.8f,1);
    _putenv_s("RECOMP_WALKER_SHADOW_MIN","0.5");pc_shadows_load();
    run(.101960786f,.5f);run(.8f,.8f);run(0,0);run(1,1);
    WORD(entity)=0x3ABCD0;run(.101960786f,.101960786f); /* player / other entity */
    WORD(entity)=BIOMECH_VTABLE;WORD(stack)=0x123456;run(.101960786f,.101960786f);
    WORD(stack)=SKELETAL_SHADOW_CALLER;WORD(entity+0x7C)=model+4;run(.101960786f,.101960786f);
    WORD(entity+0x7C)=model;
    const char *invalid[]={"0","nan","inf","-1","1.1","0.5oops"};
    for(unsigned i=0;i<sizeof invalid/sizeof invalid[0];++i) {
        _putenv_s("RECOMP_WALKER_SHADOW_MIN",invalid[i]);pc_shadows_load();run(.101960786f,.101960786f);
    }
    _putenv_s("RECOMP_WALKER_SHADOW_MIN","1");pc_shadows_load();
    run(.101960786f,1);run(1,1);
    CHECK(calls==17);
    puts("Walker shadows: default full strength, scope, overrides and guest ABI passed");
    free((void *)g_xbox_mem_offset);return 0;
}
