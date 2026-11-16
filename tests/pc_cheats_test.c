/* Exercise the effect boundaries independently of the menu and original game. */
#include "../src/pc_cheats.c"
#include <stdlib.h>
#define CHECK(x) do {if (!(x)) {fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);}} while(0)
RECOMP_TLS uint32_t g_eax,g_ecx,g_edx,g_ebx,g_esi,g_edi,g_esp,g_ebp,g_seh_ebp;
RECOMP_TLS double g_fp_stack[8];
RECOMP_TLS int g_fp_top;
ptrdiff_t g_xbox_mem_offset;
#define RECOMP_GENERATED_CODE
#include "recomp_types.h"
RECOMP_TLS int g_fp_cmp;
RECOMP_TLS uint16_t g_fp_cc,g_fp_control_word=0x27F;
RECOMP_TLS uint32_t g_eflags;
RECOMP_TLS int g_df;
uint32_t g_xbox_code_lo=0x10000,g_xbox_code_hi=0x3A9840;
volatile uint32_t g_icall_trace[ICALL_TRACE_SIZE],g_icall_trace_idx;
volatile uint64_t g_icall_count;
void sub_001E5960(void) { g_eax=CW(CW(g_esp+4))==PLAYER_VT;g_esp+=4; }
/* Difficulty/mode helpers do not own the SP flag. Exercise the original
 * initializers themselves instead of inventing session flags in this test. */
static int test_game_mode;
void sub_0007FEF0(void) {g_esp+=4;}
void sub_0007FE90(void) {CW(g_ecx+0x18)=test_game_mode;g_esp+=4;}
static void inventory_updated(void) { ++g_eax;g_esp+=4; }
recomp_func_t recomp_lookup(uint32_t va) {CHECK(va==0x100010);return inventory_updated;}
recomp_func_t recomp_lookup_manual(uint32_t va) {return recomp_lookup(va);}
recomp_func_t recomp_lookup_kernel(uint32_t va) {return recomp_lookup(va);}
void recomp_icall_fail_log(uint32_t va) {(void)va;abort();}
void recomp_icall_not_code_log(uint32_t va) {(void)va;abort();}
#include "sam_cheat_helpers.inc"
static const uint32_t p=0x600000,w=0x610000,action=0x620000,net=0x630000,sp=0x70000;
static int calls,grants;
static float timer_value;
static void original(void) {++calls;g_esp+=8;}
static void grant(void) {++grants;CHECK(g_ecx==w);CW(w+0x120)=0x7FFF;g_eax=0x123;g_ecx=0;g_esp+=4;}
static void player_original(void) {
    ++calls;
    if(!CW(p+0x3F0) && CF(p+0xE8)>0) {
        if(values[CHEAT_SPEED]) CHECK(CF(0x407A98)==multipliers[values[CHEAT_SPEED]]);
        if(values[CHEAT_SUPER_JUMP]) CHECK(CF(0x407AE0)==20);
    }
    CHECK(g_ecx==p);g_esp+=8;
}
static void damage_original(void) {++calls;g_fp_top=7;g_fp_stack[7]=4;g_esp+=4;}
static void receive(void) {++calls;CF(p+0xE8)-=10;CF(p+0x37C)-=15;g_esp+=24;}
static void timer(void) {++calls;timer_value=CF(g_esp+4);g_esp+=8;}
static void native_tick(void) {++calls;CHECK(CW(0x480754)==(uint32_t)values[CHEAT_FLY]);g_esp+=4;}
static void frame(void) {g_esp=sp;g_ecx=p;CW(sp+4)=action;pc_cheats_player(player_original,grant);CHECK(g_esp==sp+8);}
static DWORD WINAPI other_thread(void *unused) {(void)unused;CHECK(!pc_cheats_single_player());return 0;}
int main(void) {
    char path[MAX_PATH],tmp[MAX_PATH],value[48];GetTempPathA(MAX_PATH,tmp);GetTempFileNameA(tmp,"sch",0,path);
    g_xbox_mem_offset=(ptrdiff_t)calloc(1,0x800000);CHECK(g_xbox_mem_offset);
    pc_cheats_load(path);for(int i=0;i<CHEAT_ROWS;++i)CHECK(values[i]==0);
    CW(NET)=net;
    g_esp=sp;CW(sp+4)=net+0x9CC;sub_00080040();
    CHECK(g_esp==sp+8 && !CW(net+0x9CC+8) && CW(net+0x9CC+0x10)==1);
    CHECK(pc_cheats_single_player());
    /* The native cooperative/deathmatch constructors must remain excluded,
     * even when only one local player is connected. */
    for(test_game_mode=0;test_game_mode<=2;test_game_mode+=2) {
        g_esp=sp;CW(sp+4)=net+0x9CC;sub_000800E0();
        CHECK(g_esp==sp+8 && !pc_cheats_single_player());
    }
    g_esp=sp;CW(sp+4)=net+0x9CC;sub_00080040();
    CW(net+0x14)=0x640000;CW(0x640010)=0x650000;
    CW(0x650000)=1;CW(0x650004)=p;CW(p)=PLAYER_VT;CF(p+0xE8)=100;
    CW(p+0x390)=w;CW(w+0x108)=p;CF(0x407A98)=1;CF(0x407AE0)=10;
    /* The actual translated powerup helper composes with the new multiplier
     * and preserves its x87 return stack. The actual GiveAll maps capacities. */
    CF(0x3A9BDC)=1;CF(0x3A9D9C)=4;CF(p+0x224)=10;CF(p+0x1128)=0;
    for(int powerup=0;powerup<2;++powerup) {
        values[CHEAT_DAMAGE]=2;g_fp_top=3;g_esp=sp;CW(sp+4)=p;
        CF(p+0x1128)=powerup?20:0;pc_cheats_damage(sub_0004C580_gen);
        CHECK(g_fp_top==2 && g_fp_stack[2]==(powerup?8:2) && g_esp==sp+4);
    }
    values[CHEAT_DAMAGE]=0;g_fp_top=0;
    CW(w)=0x660000;CW(0x660014)=0x100010;
    for(uint32_t a=0x300;a<=0x340;a+=8)CW(w+a+4)=a;
    g_esp=sp;give_weapons(sub_000CA160,w);CHECK(CW(w+0x120)==0x7FFF && g_esp==sp);
    for(uint32_t a=0x300;a<=0x340;a+=8)CHECK(CW(w+a)==a);
    CW(w+0x120)=0;
    CHECK(pc_cheats_single_player() && local_player()==p);
    HANDLE thread=CreateThread(NULL,0,other_thread,NULL,0,NULL);CHECK(thread);WaitForSingleObject(thread,INFINITE);CloseHandle(thread);
    frame();CHECK(CF(p+0xE8)==100 && CF(p+0x37C)==0 && !grants);
    for(int i=CHEAT_HEALTH;i<CHEAT_RESET;++i)values[i]=1;
    for(uint32_t a=0x300;a<=0x340;a+=8)CW(w+a+4)=60;
    frame();CHECK(CF(p+0xE8)==200 && CF(p+0x37C)==200 && grants==1);
    CHECK(CF(0x407A98)==1 && CF(0x407AE0)==10 && !CW(0x480754) && !CW(net+0x9CC+0x50));
    for(uint32_t a=0x300;a<=0x340;a+=8)CHECK(CW(w+a)==60);
    frame();CHECK(grants==1); /* not every frame */
    CW(w+0x120)=0;frame();CHECK(grants==2); /* inventory reset on a new level */
    values[CHEAT_FLY]=0;values[CHEAT_SUPER_JUMP]=0;
    CF(action+4)=0;CW(p+0x138)=1;frame();
    CF(action+4)=1;frame();CW(p+0x138)=0;CF(action+4)=0;frame();
    CF(p+0x348)=-4;CF(action+4)=1;frame();CHECK(CF(p+0x348)==10);
    CF(p+0x348)=-4;frame();CHECK(CF(p+0x348)==-4); /* holding cannot fly */
    CF(action+4)=0;frame();CF(action+4)=1;frame();CHECK(CF(p+0x348)==-4); /* one extra only */
    CW(p+0x138)=1;frame();CW(p+0x138)=0;CF(action+4)=0;frame();CF(action+4)=1;frame();CHECK(CF(p+0x348)==10);
    values[CHEAT_DAMAGE]=3;g_esp=sp;CW(sp+4)=p;pc_cheats_damage(damage_original);CHECK(g_fp_stack[7]==12 && g_esp==sp+4);
    g_esp=sp;CW(sp+4)=p+4;pc_cheats_damage(damage_original);CHECK(g_fp_stack[7]==4);
    int before=calls;g_ecx=p;g_esp=sp;pc_cheats_receive_damage(receive);CHECK(calls==before && g_esp==sp+24 && CF(p+0xE8)==200);
    values[CHEAT_HEALTH]=0;g_ecx=p;g_esp=sp;pc_cheats_receive_damage(receive);CHECK(CF(p+0xE8)==190 && CF(p+0x37C)==200);
    values[CHEAT_FIRE_RATE]=2;g_ecx=w;g_esp=sp;CW(sp)=0xDB9C2;CF(sp+4)=1;
    pc_cheats_timer(timer);CHECK(timer_value==0.05f && CF(sp+4)==1 && g_esp==sp+8);
    values[CHEAT_SNIPER]=0;g_esp=sp;pc_cheats_timer(timer);CHECK(timer_value==0.5f);
    CW(sp)=0x70000;g_esp=sp;pc_cheats_timer(timer);CHECK(timer_value==1); /* non-weapon call */
    values[CHEAT_FLY]=1;g_ecx=p;g_esp=sp;pc_cheats_native_tick(native_tick);CHECK(CW(0x480754)==0 && g_esp==sp+4);
    /* Multiplayer, malformed objects, cinematics and dead players are inert. */
    CW(net+0x9CC+0x10)=0;CF(p+0xE8)=100;CF(p+0x37C)=20;CW(w+0x300)=3;
    g_ecx=p;g_esp=sp;CW(sp+4)=action;pc_cheats_player(original,grant);CHECK(CF(p+0x37C)==20 && CW(w+0x300)==3);
    g_esp=sp;CW(sp+4)=p;pc_cheats_damage(damage_original);CHECK(g_fp_stack[7]==4);
    g_ecx=w;g_esp=sp;CW(sp)=0xDB9C2;CF(sp+4)=1;pc_cheats_timer(timer);CHECK(timer_value==1);
    CW(net+0x9CC+0x10)=1;CW(p+0x3F0)=0x660000;frame();CHECK(CF(p+0x37C)==20);CW(p+0x3F0)=0;
    CF(p+0xE8)=0;frame();CHECK(CF(p+0xE8)==0);CF(p+0xE8)=100;
    CW(0x650004)=0xFFFFFFFC;CHECK(!local_player());CW(0x650004)=p;
    /* Reset/persistence, wrapping multipliers and scoped frontend unlocks. */
    pc_cheats_step(CHEAT_RESET,1);for(int i=CHEAT_HEALTH;i<CHEAT_RESET;++i)CHECK(!values[i]);
    g_ecx=w;g_esp=sp;CW(sp)=0xDB9C2;CF(sp+4)=0.01f;pc_cheats_timer(timer);CHECK(timer_value==0.01f);
    pc_cheats_step(CHEAT_SPEED,-1);pc_cheats_value(CHEAT_SPEED,value,sizeof value);CHECK(!strcmp(value,"5x"));
    pc_cheats_step(CHEAT_PAUSE_MENU,1);pc_cheats_load(path);CHECK(pc_cheats_pause_menu() && values[CHEAT_SPEED]==4);
    pc_cheats_step(CHEAT_UNLOCK,1);CW(0x47C5E0)=0;pc_cheats_frontend(1);CHECK(CW(0x47C5E0)==1);
    pc_cheats_frontend(0);CHECK(CW(0x47C5E0)==0);
    WritePrivateProfileStringA("cheats","damage","999",path);pc_cheats_load(path);CHECK(values[CHEAT_DAMAGE]==0);
    DeleteFileA(path);free((void *)g_xbox_mem_offset);puts("PASS: cheats, original defaults, ownership, session isolation and persistence");return 0;
}
