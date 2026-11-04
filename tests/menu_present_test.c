#include <xbox/xboxrecomp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
RECOMP_TLS uint32_t g_eax,g_ecx,g_edx,g_ebx,g_esi,g_edi,g_esp,g_ebp,g_seh_ebp,g_fs_base;
ptrdiff_t g_xbox_mem_offset;
volatile uint32_t g_icall_trace[16],g_icall_trace_idx;
volatile uint64_t g_icall_count;
#define WORD(a) (*(uint32_t *)((uintptr_t)g_xbox_mem_offset+(a)))
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static int interval,waits;
int xbox_GetPresentInterval(void){return interval;}
void sub_003041E0(void);
void sub_0002B6BE(void);
void sub_0002A089(void);
extern unsigned pc_frame_calls,pc_menu_draw_calls;
extern uint32_t pc_hovered_shortcut,pc_text_effect;
extern unsigned pc_text_calls;
void sub_003041E0_gen(void){++waits;g_eax=0;g_esp+=4;}
void sub_001A6FF0_gen(void){abort();}
void sub_0019F1E0_gen(void){abort();}
void sub_0019F5E0_gen(void){abort();}
void sub_0019D980(void){abort();}
void sub_0019D880(void){abort();}
void sub_002B7552(void){abort();}
void sub_002B78DF(void){abort();}
void sub_0019D290(void){abort();}
void sub_00367844_gen(void){abort();}
void sub_003098A0_gen(void){abort();}
void sub_00367741_gen(void) { abort(); }
void sub_00368B2C_gen(void) { abort(); }
void sub_00304F90(void) { abort(); }
static void run(uint32_t caller,int sync,int expected){
 interval=sync;waits=0;pc_frame_calls=0;g_esp=0x1000;WORD(g_esp)=caller;
 g_eax=99;g_ecx=2;g_edx=3;g_ebx=4;g_esi=5;g_edi=6;g_ebp=7;g_seh_ebp=8;
 sub_003041E0();
 CHECK(waits==expected && g_esp==0x1004 && g_eax==0);
 CHECK(g_ecx==2 && g_edx==3 && g_ebx==4 && g_esi==5 && g_edi==6 && g_ebp==7 && g_seh_ebp==8);
 CHECK(WORD(0x1000)==caller);
 CHECK(pc_frame_calls==((caller==0x001A95B8 || caller==0x001DA528)?1u:0u));
}
static void text_hover_test(void){
 const uint32_t callers[]={0x1CCC1,0x1CE15,0x1B5EB};
 for(unsigned c=0;c<3;++c)for(int hovered=0;hovered<2;++hovered){
  g_esp=0x1000;g_ebp=g_seh_ebp=0x18000;WORD(g_ebp-0x50)=0x123400;
  WORD(g_esp)=callers[c];
  for(unsigned i=1;i<=7;++i)WORD(g_esp+4*i)=0x55550000+i;
  WORD(g_esp+0x1C)=0;
  pc_hovered_shortcut=hovered?0x123400:0;pc_text_calls=0;
  sub_0002A089();
  CHECK(pc_text_calls==1 && pc_text_effect==((c<2 && hovered)?0x3F800000u:0));
  CHECK(g_esp==0x1004 && g_ebp==0x18000 && g_seh_ebp==0x18000 && WORD(0x101C)==0);
  for(unsigned i=1;i<=6;++i)CHECK(WORD(0x1000+4*i)==0x55550000+i);
 }
}
int main(void){
 g_xbox_mem_offset=(ptrdiff_t)calloc(1,0x20000);CHECK(g_xbox_mem_offset);
 run(0x001A95B8,0,0); /* Native vsync off: engine menu swap is unlocked. */
 for(int i=1;i<=4;++i)run(0x001A95B8,i,1);
 run(0x001DA528,0,0); /* Loading/video presenter follows native vsync too. */
 for(int i=1;i<=4;++i)run(0x001DA528,i,1);
 const uint32_t movies[]={0x001D9C86,0x0031080D};
 for(unsigned i=0;i<2;++i){run(movies[i],0,1);run(movies[i],1,1);}
 run(0x12345678,0,1); /* Unknown callers retain the original semantics. */
 pc_frame_calls=pc_menu_draw_calls=0;g_esp=0x1000;
 sub_0002B6BE(); /* Pause rendering need not enter a presentation wait. */
 CHECK(pc_frame_calls==1 && pc_menu_draw_calls==1 && g_esp==0x1004);
 text_hover_test();
 free((void *)g_xbox_mem_offset);puts("menu presentation, movie waits and guest ABI checks passed");return 0;
}
