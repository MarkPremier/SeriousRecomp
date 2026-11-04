/* Native PC bindings: profile independence, menu isolation, release semantics,
 * count-based mouse look, rebinding and persistence in a private INI. */
#include "../src/pc_controls.c"

RECOMP_TLS uint32_t g_eax,g_ecx,g_edx,g_ebx,g_esi,g_edi,g_esp,g_ebp,g_seh_ebp;
ptrdiff_t g_xbox_mem_offset;
#define RECOMP_GENERATED_CODE
#include "recomp_types.h"
RECOMP_TLS double g_fp_stack[8];
RECOMP_TLS int g_fp_top;
RECOMP_TLS uint16_t g_fp_control_word=0x27F;
void sub_0001ACF9(void);
void sub_0001AD5E(void);
#include "sam_menu_animation.inc"
static void test_text_size(void);
static void test_focus(void);
static void test_unfocus(void);
static void test_gadget_key(void);
recomp_func_t recomp_lookup(uint32_t va){
    switch(va){
    case 0x1C953:return sub_0001C953;
    case 0x100010:return test_text_size;
    case 0x100020:return test_focus;
    case 0x100030:return test_unfocus;
    case 0x100040:return test_gadget_key;
    }
    fprintf(stderr,"Unexpected menu call %08X\n",va);abort();
}
recomp_func_t recomp_lookup_manual(uint32_t va){return recomp_lookup(va);}
recomp_func_t recomp_lookup_kernel(uint32_t va){return recomp_lookup(va);}
void recomp_icall_fail_log(uint32_t va){(void)va;abort();}
void recomp_icall_not_code_log(uint32_t va){(void)va;abort();}
uint32_t g_xbox_code_lo=0x10000,g_xbox_code_hi=0x3A9840;
volatile uint32_t g_icall_trace[ICALL_TRACE_SIZE],g_icall_trace_idx;
volatile uint64_t g_icall_count;
static int down[0x108], mouse_dx,mouse_dy,raw_mouse,capture;
static int clicks[5], wheel, pointer_on, px,py,pw,ph,backs;
static int menu_events,last_menu_key,start_pulses;
static int quits, sounds;
#define CHECK(x) do {if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);}}while(0)
uint32_t xbox_HeapAlloc(uint32_t size,uint32_t alignment){CHECK(size==16 && alignment==16);return 0x680000;}
void sub_0002F12B(void){abort();}
void sub_0002727C(void){abort();}
void sub_0002A807(void){abort();}
void sub_0002B52A(void){++backs;g_esp+=4;}
void sub_00028A11(void){++sounds;g_eax=123;g_ecx=456;g_edx=789;g_esp+=4;}
void sub_0001ACF9(void){g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=FLOAT(g_ecx+GADGET_X);g_esp+=4;}
void sub_0001AD5E(void){g_fp_top=(g_fp_top+7)&7;g_fp_stack[g_fp_top]=FLOAT(g_ecx+GADGET_Y);g_esp+=4;}
static void test_text_size(void){FLOAT(WORD(g_esp+4))=220;FLOAT(WORD(g_esp+8))=30;g_esp+=16;}
static void test_focus(void){WORD(g_ecx+GADGET_FOCUSED)=1;g_esp+=4;}
static void test_unfocus(void){WORD(g_ecx+GADGET_FOCUSED)=0;g_esp+=4;}
static void test_gadget_key(void){g_eax=0;g_esp+=8;}
void sub_0002B588(void){++menu_events;last_menu_key=(int)WORD(g_esp+4);g_esp+=4;}
VOID __stdcall xbox_HalReturnToFirmware(ULONG routine){CHECK(routine==2);++quits;}
void xbox_KbmSetMenuMode(int on){(void)on;}
void xbox_KbmSetRawMouse(int on){raw_mouse=on;}
void xbox_KbmSetCapture(int on){capture=on;}
void xbox_KbmSetBinding(const char *target,const int *binding,int count){(void)target;(void)binding;(void)count;}
void xbox_KbmPulse(const char *target,int ms){CHECK(!strcmp(target,"START"));if(ms)++start_pulses;}
int xbox_KbmInputDown(int key){return key>=0 && key<0x108?down[key]:0;}
int xbox_KbmMenuWheel(void){int n=wheel;wheel=0;return n;}
static int video_active;
static DWORD WINAPI frame_on_worker(LPVOID unused){(void)unused;pc_controls_frame();pc_controls_movie_frame();return 0;}
int xbox_VideoActive(void){return video_active;}
static int frames_2d;
int nv2a_gpu_frames_without_3d(void){return frames_2d;}
static float wide_set=1;
void nv2a_gpu_set_wide_factor(float factor){wide_set=factor;}
void nv2a_gpu_set_wide_title_fov(int on){(void)on;}
float pc_settings_aspect(void){return 64.0f/27;}
int xbox_FramebufferKeyDown(int key){return xbox_KbmInputDown(key);}
int xbox_FramebufferMouseButton(int button){(void)button;return 0;}
int xbox_FramebufferMouseClicks(int button){int n=clicks[button];clicks[button]=0;return n;}
int xbox_FramebufferMouseDelta(int *dx,int *dy){*dx=mouse_dx;*dy=mouse_dy;mouse_dx=mouse_dy=0;return *dx || *dy;}
int xbox_FramebufferPointer(int *x,int *y,int *w,int *h){*x=px;*y=py;*w=pw;*h=ph;return pointer_on;}
void xbox_FramebufferSetMenuPointer(int on){(void)on;}
void pc_settings_load(const char *path){(void)path;}
const char *pc_settings_label(int row){(void)row;return "Setting";}
void pc_settings_value(int row,char *out,size_t size){(void)row;snprintf(out,size,"Value");}
void pc_settings_step(int row,int direction){(void)row;(void)direction;}
void pc_settings_poll(void){}

static void clear_axes(void){memset(PTR(0x480468),0,12*4);}
static void close_enough(float value,float expected){CHECK(fabsf(value-expected)<0.00001f);}

static void options_animation_test(void)
{
    static const uint32_t original[]={0x472768,0x472830,0x4728F8,0x4729E8,0x472AB0,0x472BA0};
    entry=0x640000;settings_entry=0x640200;g_esp=0x70000;
    /* Native switch targets for slide-left and slide-right. */
    WORD(0x1B095+4)=0x1AFFD;WORD(0x1B095+8)=0x1B00E;
    for(int hidden=-1;hidden<2;++hidden) {
        unsigned n=0;uint32_t order[8];
        for(int i=0;i<6;++i) {
            WORD(original[i]+0x80)=i!=hidden;
            FLOAT(original[i]+0x20)=FLOAT(original[i]+0x34)=0.8f;
            if(i!=hidden)order[n++]=original[i];
        }
        order[n]=entry;order[n+1]=settings_entry;
        pc_controls_options_layout();
        CHECK(g_esp==0x70000 && g_fp_top==0);
        for(unsigned i=0;i<n+2;++i) {
            uint32_t row=order[i];
            close_enough(FLOAT(row+0x14),140);
            close_enough(FLOAT(row+0x28),140+(i&1?640:-640));
            close_enough(FLOAT(row+0x18),157+30.0f*i-(i>=n?19:0));
            close_enough(FLOAT(row+0x2C),FLOAT(row+0x18));
            CHECK(WORD(row+0x8C)==2+(i&1));
        }
        CHECK(WORD(order[n-1]+0xA0)==entry && WORD(entry+0xA0)==settings_entry);
        CHECK(WORD(order[0]+0x9C)==settings_entry && WORD(settings_entry+0xA0)==order[0]);
    }
    entry=settings_entry=0;
}

static void native_mouse_test(void)
{
    const uint32_t menu=0x660000, row=0x661000, next=0x661200, texture=0x665000;
    const uint32_t footer[]={0x662000,0x662200,0x662400,0x662600};
    const int anchors[]={49,152,302,430}, icons[]={65,168,447,575}, labels[]={100,240,350,520};
    const char letters[]={'B','Y','X','A'};
    const int events[]={VK_ESCAPE,0xAF0,0xA8C,VK_RETURN};
    uint32_t gadgets[]={row,next,footer[0],footer[1],footer[2],footer[3]};
    WORD(0x3A9D98)=0x40000000; /* native alignment divisor: 2 */
    WORD(0x4702B8)=texture;WORD(texture+0x20)=WORD(texture+0x24)=32;WORD(texture+0x28)=0;
    WORD(0x670000+VTABLE_TEXT_SIZE)=0x100010;
    WORD(0x670000+VTABLE_SET_FOCUS)=0x100020;
    WORD(0x670000+VTABLE_KILL_FOCUS)=0x100030;
    WORD(0x670000+VTABLE_GADGET_KEY)=0x100040;
    WORD(SHORTCUT_VTABLE+VTABLE_TEXT_SIZE)=0x1C953;
    WORD(menu+SCREEN_GADGETS)=row+GADGET_NODE;
    /* Actual Select Player anchors and controller keys. The page offset is
     * deliberately nonzero: footer rendering ignores it, text rows do not. */
    FLOAT(menu+SCREEN_OFFSET_X)=10;FLOAT(menu+SCREEN_OFFSET_Y)=20;
    for(unsigned i=0;i<6;++i){
        uint32_t g=gadgets[i];
        WORD(g+GADGET_NODE)=i<5?gadgets[i+1]+GADGET_NODE:0x66F000;
        WORD(g+GADGET_VISIBLE)=WORD(g+GADGET_ENABLED)=1;
        WORD(g+GADGET_CENTRED)=WORD(g+0x70)=0xFFFFFFFFu;
        WORD(g+GADGET_SCREEN)=menu;
        WORD(g)=i<2?0x670000:SHORTCUT_VTABLE;
        FLOAT(g+GADGET_X)=i<2?100:(float)anchors[i-2];
        FLOAT(g+GADGET_Y)=i<2?100+50.0f*i:412;
    }
    for(int i=0;i<4;++i){WORD(footer[i]+SHORTCUT_BUTTON)=letters[i];WORD(footer[i]+0xB4)=128;WORD(footer[i]+SHORTCUT_ICON_LEFT)=i<2;}
    WORD(row+GADGET_FOCUSED)=1;WORD(row+GADGET_DOWN)=next;WORD(next+GADGET_UP)=row;
    WORD(ACTIVE_MENU)=menu;page=settings_page=quit_page=0;capturing=-1;menu_draw_seen=0;
    pointer_on=1;pw=640;ph=480;g_esp=0x70000;
    uint32_t found[MAX_MENU_GADGETS], focused;
    CHECK(menu_buttons(menu,found,&focused)==2 && focused==row);
    CHECK(menu_shortcuts(menu,found)==4);
    int before=sounds;
    uint32_t saved_regs[]={g_eax,g_ecx,g_edx,g_ebx,g_esi,g_edi,g_ebp,g_seh_ebp};
    px=200;py=180;native_mouse_frame();
    CHECK(WORD(next+GADGET_FOCUSED) && !WORD(row+GADGET_FOCUSED) && sounds==before+1);
    CHECK(g_eax==saved_regs[0] && g_ecx==saved_regs[1] && g_edx==saved_regs[2] &&
          g_ebx==saved_regs[3] && g_esi==saved_regs[4] && g_edi==saved_regs[5] &&
          g_ebp==saved_regs[6] && g_seh_ebp==saved_regs[7]);
    ++px;native_mouse_frame();CHECK(sounds==before+1); /* motion within a row is silent */
    for(int i=0;i<4;++i){
        px=icons[i];py=430;before=sounds;native_mouse_frame();CHECK(sounds==before+1);
        CHECK(pc_controls_shortcut_hovered(footer[i]) && !pc_controls_shortcut_hovered(row));
        CHECK(!WORD(footer[i]+GADGET_FOCUSED)); /* animation must not steal keyboard focus */
        CHECK(WORD(next+GADGET_FOCUSED)); /* shortcut hover retains the selected profile */
        int previous_events=menu_events;
        clicks[0]=1;native_mouse_frame();CHECK(menu_events==previous_events+1 && last_menu_key==events[i]);
        CHECK(sounds==before+1); /* no repeat while held over the same shortcut */
        px=labels[i];clicks[0]=1;native_mouse_frame();CHECK(menu_events==previous_events+2 && last_menu_key==events[i]);
        CHECK(g_esp==0x70000 && g_fp_top==0);
    }
    /* Same geometry in pillarboxed and letterboxed windows. */
    pw=1600;ph=900;px=1175;py=806;clicks[0]=1;native_mouse_frame();CHECK(last_menu_key==VK_RETURN);
    pw=600;ph=900;px=94;py=628;clicks[0]=1;native_mouse_frame();CHECK(last_menu_key==VK_ESCAPE);
    pw=640;ph=480;px=350;py=430;
    WORD(footer[2]+GADGET_ENABLED)=0;CHECK(menu_shortcuts(menu,found)==3);
    before=menu_events;clicks[0]=1;native_mouse_frame();CHECK(menu_events==before);
    WORD(footer[1]+GADGET_VISIBLE)=0;CHECK(menu_shortcuts(menu,found)==2);
    WORD(row+GADGET_ENABLED)=0;
    CHECK(menu_buttons(menu,found,&focused)==2); /* disabled rows must not sever the navigation graph */
    px=20;py=430;clicks[0]=1;native_mouse_frame();CHECK(menu_events==before); /* outside all buttons */
    for(int i=0;i<4;++i)CHECK(!pc_controls_shortcut_hovered(footer[i]));
    px=100;py=430;native_mouse_frame();CHECK(pc_controls_shortcut_hovered(footer[0]));
    pointer_on=0;native_mouse_frame();CHECK(!pc_controls_shortcut_hovered(footer[0]));
    WORD(menu+SCREEN_GADGETS)=0;WORD(ACTIVE_MENU)=0;pointer_on=0;
}

int main(void)
{
    char temp[MAX_PATH],path[MAX_PATH],label[64];
    _putenv_s("RECOMP_PC_CONTROLS_NO_THREAD","1"); /* the skip checks below drive it directly */
    GetTempPathA(sizeof temp,temp);
    snprintf(path,sizeof path,"%ssam_controls_%lu.ini",temp,GetCurrentProcessId());
    _putenv_s("RECOMP_PC_CONTROLS_FILE",path);
    _putenv_s("RECOMP_KBM","1");
    g_xbox_mem_offset=(ptrdiff_t)calloc(1,0x800000);
    CHECK(g_xbox_mem_offset);
    options_animation_test();
    pc_controls_load();CHECK(enabled && keys[JUMP]==VK_SPACE && keys[USE]==MOUSE2 && speed==5);
    CHECK(named_key("1")=='1' && named_key("Wheel down")==WHEEL_DOWN && named_key("garbage")==-1);
    WritePrivateProfileStringA("paths","game_dir","unchanged",path);
    save();keys[FAVORITE1]=0;pc_controls_load();CHECK(keys[FAVORITE1]=='1');
    GetPrivateProfileStringA("paths","game_dir","",label,sizeof label,path);CHECK(!strcmp(label,"unchanged"));
    set_key(FIRE,VK_SPACE);CHECK(keys[FIRE]==VK_SPACE && keys[JUMP]==MOUSE1 && raw_mouse);
    set_key(FIRE,0);CHECK(keys[FIRE]==0 && keys[JUMP]==MOUSE1);
    for(int i=0;i<BIND_COUNT;++i)keys[i]=actions[i].key;
    WORD(0x4A2AE0)=0x700000;WORD(0x700014)=0x700100;WORD(0x700110)=0x700200;
    WORD(0x700200)=1;WORD(0x700204)=0x700300;
    uint32_t profile=0x10000;
    pc_controls_frame(); /* Already in play before the first deliberate press. */
    down['W']=down[VK_SPACE]=down[MOUSE1]=1;
    FLOAT(0x480468)=0.25f;WORD(0x480530)=7;
    CHECK(pc_controls_input_begin(profile));
    close_enough(FLOAT(0x480468),1);close_enough(FLOAT(0x480478),1);
    CHECK(WORD(0x480530)==1);
    pc_controls_input_end();CHECK(WORD(0x480530)==7);
    memset(down,0,sizeof down);clear_axes();
    CHECK(pc_controls_input_begin(profile));CHECK(WORD(0x480530)==7);pc_controls_input_end();
    WORD(ACTIVE_MENU)=OPTIONS;mouse_dx=10;
    CHECK(!pc_controls_input_begin(profile) && mouse_dx==0);WORD(ACTIVE_MENU)=0;
    WORD(profile+0x38)=1;CHECK(!pc_controls_input_begin(profile));WORD(profile+0x38)=0;
    enabled=0;CHECK(!pc_controls_input_begin(profile));enabled=1;
    WORD(0x700200)=0;CHECK(!pc_controls_input_begin(profile));WORD(0x700200)=1;
    for(int sensitivity=-127;sensitivity<=127;sensitivity+=127) {
        *(int8_t *)PTR(profile+0x22)=(int8_t)sensitivity;
        for(int invert=0;invert<2;++invert) {
            WORD(profile+0x1C)=invert?2:0;clear_axes();mouse_dx=10;mouse_dy=-10;
            CHECK(pc_controls_input_begin(profile));
            float h=1+sensitivity*(0.75f/127),v=1+sensitivity*(0.5f/127);
            close_enough(FLOAT(0x480484)*h,1);
            close_enough(FLOAT(0x480488)*v*(invert?-1:1),1);
            pc_controls_input_end();
        }
    }
    *(int8_t *)PTR(profile+0x22)=0;WORD(profile+0x1C)=0;
    float total=0;
    for(int i=0;i<10;++i){clear_axes();mouse_dx=10;pc_controls_input_begin(profile);total+=FLOAT(0x480484);pc_controls_input_end();}
    clear_axes();mouse_dx=100;pc_controls_input_begin(profile);close_enough(FLOAT(0x480484),total);pc_controls_input_end();
    clear_axes();invert_y=1;mouse_dy=-10;pc_controls_input_begin(profile);close_enough(FLOAT(0x480488),-1);pc_controls_input_end();
    blocked[JUMP]=1;down[VK_SPACE]=1;clear_axes();pc_controls_input_begin(profile);close_enough(FLOAT(0x480478),0);pc_controls_input_end();
    down[VK_SPACE]=0;pc_controls_input_begin(profile);pc_controls_input_end();CHECK(!blocked[JUMP]);
    down[VK_SPACE]=1;clear_axes();pc_controls_input_begin(profile);close_enough(FLOAT(0x480478),1);pc_controls_input_end();
    cancel_capture();CHECK(!capture);
    /* Exercise the page's real event path, including clicks shorter than a
     * frame, keyboard Delete (not an Xbox key), and resized-window hit tests. */
    page=0x600000;WORD(ACTIVE_MENU)=page;menu_mode=1;
    for(int i=0;i<ROW_COUNT;++i) {
        rows[i]=0x610000+i*0x400;values[i]=rows[i]+0x200;
        WORD(rows[i]+0x4C)=rows[i]+BUTTON_SIZE;
        WORD(values[i]+0x4C)=values[i]+BUTTON_SIZE;
    }
    status=0x620000;help=0x620200;
    WORD(status+0x4C)=status+BUTTON_SIZE;WORD(help+0x4C)=help+BUTTON_SIZE;
    memset(down,0,sizeof down);selected=scroll=0;
    pointer_on=1;pw=1280;ph=960;px=200;py=334;clicks[0]=1;
    pc_controls_frame();CHECK(selected==0 && capturing==0 && capture);
    clicks[3]=1;pc_controls_frame();CHECK(capturing==-1 && keys[FORWARD]==MOUSE4 && !capture);
    down[VK_DELETE]=1;pc_controls_frame();CHECK(keys[FORWARD]==0);
    down[VK_DELETE]=0;pc_controls_frame();
    int before_sound=sounds;
    px=200;py=646;pc_controls_frame();CHECK(selected==FIRE && sounds==before_sound+1);
    ++px;pc_controls_frame();CHECK(sounds==before_sound+1);
    pw=1600;ph=900;px=400;py=313;pc_controls_frame();CHECK(selected==FORWARD);
    pw=600;ph=900;px=100;py=381;pc_controls_frame();CHECK(selected==FORWARD);
    g_esp=0x30000;WORD(g_esp+4)=VK_DOWN;g_ebx=44;g_esi=45;g_edi=46;
    pc_controls_dispatch(PAGE_KEY)();CHECK(g_esp==0x3000C && selected==BACKWARD && g_eax==1);
    CHECK(g_ebx==44 && g_esi==45 && g_edi==46);
    begin_capture();wheel=-1;pc_controls_frame();CHECK(keys[BACKWARD]==WHEEL_DOWN && capturing==-1);
    begin_capture();capture_deadline=GetTickCount()-1;pc_controls_frame();CHECK(capturing==-1 && !capture);
    pw=1280;ph=960;px=1040;py=820;clicks[0]=1;pc_controls_frame();CHECK(backs==1);
    pointer_on=0;menu_events=0;
    down[VK_DOWN]=1;pc_controls_frame();CHECK(menu_events==1 && last_menu_key==VK_DOWN);
    pc_controls_frame();CHECK(menu_events==1); /* no repeat on the following render */
    down[VK_DOWN]=0;pc_controls_frame();down[VK_DOWN]=1;pc_controls_frame();CHECK(menu_events==2);
    down[VK_DOWN]=0;WORD(ACTIVE_MENU)=0;pc_controls_frame();
    down[VK_ESCAPE]=1;pc_controls_frame();CHECK(start_pulses==1);
    pc_controls_frame();CHECK(start_pulses==1);
    WORD(ACTIVE_MENU)=page;pc_controls_frame();CHECK(menu_events==2); /* held pause cannot go Back */
    quit_page=0x630000;quit_choices[0]=0x630100;quit_choices[1]=0x630300;
    quit_selected=1;g_esp=0x30000;WORD(g_esp+4)=VK_RETURN;
    pc_controls_dispatch(QUIT_KEY)();CHECK(backs==2 && quits==0 && g_esp==0x3000C);
    g_esp=0x30000;WORD(g_esp+4)=VK_LEFT;pc_controls_dispatch(QUIT_KEY)();
    CHECK(quit_selected==0 && WORD(quit_page+0x14)==quit_choices[0]);
    g_esp=0x30000;WORD(g_esp+4)=VK_ESCAPE;pc_controls_dispatch(QUIT_KEY)();CHECK(backs==3 && quits==0);
    g_esp=0x30000;WORD(g_esp+4)=0x64;pc_controls_dispatch(QUIT_KEY)();CHECK(quits==1 && g_esp==0x3000C);
    CHECK(g_ebx==44 && g_esi==45 && g_edi==46);
    pointer_on=1;pw=640;ph=480;px=pointer_x=200;py=pointer_y=285;quit_selected=1;
    quit_frame();CHECK(quit_selected==1); /* an unmoved pointer cannot replace default No */
    px=201;quit_frame();CHECK(quit_selected==0);
    px=300;clicks[0]=1;quit_frame();CHECK(quits==1); /* gap between buttons */
    px=200;clicks[0]=1;quit_frame();CHECK(quits==2);
    /* Movies are skipped from a thread of the file's own: a skip key there
     * pulses Start once per press, and Sam's menu code never runs off the
     * main thread (pc_controls_frame from another thread does nothing). */
    {
        int before_pulses=start_pulses,before_events=menu_events;
        HANDLE worker;
        memset(down,0,sizeof down);
        pc_controls_frame(); /* release everything on the main thread first */
        WORD(ACTIVE_MENU)=page;video_active=1;down[VK_RETURN]=1;down[VK_DOWN]=1;
        worker=CreateThread(NULL,0,frame_on_worker,NULL,0,NULL);
        WaitForSingleObject(worker,INFINITE);CloseHandle(worker);
        CHECK(start_pulses==before_pulses+1 && menu_events==before_events);
        worker=CreateThread(NULL,0,frame_on_worker,NULL,0,NULL);
        WaitForSingleObject(worker,INFINITE);CloseHandle(worker);
        CHECK(start_pulses==before_pulses+1); /* held, not pressed again */
        down[VK_RETURN]=0;video_active=0;pc_controls_movie_frame();
        down[VK_SPACE]=1;pc_controls_movie_frame();CHECK(start_pulses==before_pulses+1); /* no video, no skip */
        down[VK_SPACE]=0;down[VK_DOWN]=0;pc_controls_movie_frame();
        /* The post-load briefing: no menu, no 3D, no full-screen video. */
        WORD(ACTIVE_MENU)=0;frames_2d=5;down[VK_RETURN]=1;pc_controls_movie_frame();
        CHECK(start_pulses==before_pulses+2);
        down[VK_RETURN]=0;pc_controls_movie_frame();
        frames_2d=0;down[VK_RETURN]=1;pc_controls_movie_frame();CHECK(start_pulses==before_pulses+2); /* in play */
        down[VK_RETURN]=0;pc_controls_movie_frame();
        WORD(ACTIVE_MENU)=page;frames_2d=5;down[VK_RETURN]=1;pc_controls_movie_frame();
        CHECK(start_pulses==before_pulses+2); /* a menu takes Enter itself */
        down[VK_RETURN]=0;pc_controls_movie_frame();frames_2d=0;
    }
    /* B on the pause menu resumes play but leaves ACTIVE_MENU on the Difficulty
     * page: only a menu the renderer still draws counts as shown. */
    WORD(ACTIVE_MENU)=0x46C9B0;pc_controls_menu_drawn();CHECK(shown_menu()==0x46C9B0);
    menu_draw_ms-=1000;CHECK(shown_menu()==0);
    menu_draw_seen=0;WORD(ACTIVE_MENU)=0;
    native_mouse_test();
    DeleteFileA(path);free((void *)g_xbox_mem_offset);
    puts("PC actions, release, profile isolation, raw mouse, settings and page input passed");return 0;
}
