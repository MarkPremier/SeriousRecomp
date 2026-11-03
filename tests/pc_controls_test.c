/* Native PC bindings: profile independence, menu isolation, release semantics,
 * count-based mouse look, rebinding and persistence in a private INI. */
#include "../src/pc_controls.c"

RECOMP_TLS uint32_t g_eax,g_ecx,g_edx,g_ebx,g_esi,g_edi,g_esp,g_ebp,g_seh_ebp;
ptrdiff_t g_xbox_mem_offset;
static int down[0x108], mouse_dx,mouse_dy,raw_mouse,capture;
static int clicks[5], wheel, pointer_on, px,py,pw,ph,backs;
static int menu_events,last_menu_key,start_pulses;
#define CHECK(x) do {if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);}}while(0)
uint32_t xbox_HeapAlloc(uint32_t size,uint32_t alignment){(void)size;(void)alignment;abort();}
void sub_0002F12B(void){abort();}
void sub_0002727C(void){abort();}
void sub_0002A807(void){abort();}
void sub_0002B52A(void){++backs;g_esp+=4;}
void sub_0002B588(void){++menu_events;last_menu_key=(int)WORD(g_esp+4);g_esp+=4;}
void xbox_KbmSetMenuMode(int on){(void)on;}
void xbox_KbmSetRawMouse(int on){raw_mouse=on;}
void xbox_KbmSetCapture(int on){capture=on;}
void xbox_KbmSetBinding(const char *target,const int *binding,int count){(void)target;(void)binding;(void)count;}
void xbox_KbmPulse(const char *target,int ms){CHECK(!strcmp(target,"START"));if(ms)++start_pulses;}
int xbox_KbmInputDown(int key){return key>=0 && key<0x108?down[key]:0;}
int xbox_KbmMenuWheel(void){int n=wheel;wheel=0;return n;}
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

int main(void)
{
    char temp[MAX_PATH],path[MAX_PATH],label[64];
    GetTempPathA(sizeof temp,temp);
    snprintf(path,sizeof path,"%ssam_controls_%lu.ini",temp,GetCurrentProcessId());
    _putenv_s("RECOMP_PC_CONTROLS_FILE",path);
    _putenv_s("RECOMP_KBM","1");
    g_xbox_mem_offset=(ptrdiff_t)calloc(1,0x800000);
    CHECK(g_xbox_mem_offset);
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
    px=200;py=646;pc_controls_frame();CHECK(selected==FIRE);
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
    DeleteFileA(path);free((void *)g_xbox_mem_offset);
    puts("PC actions, release, profile isolation, raw mouse, settings and page input passed");return 0;
}
