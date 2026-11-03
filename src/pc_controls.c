#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "pc_controls.h"
#include "pc_settings.h"
#include <xbox/xboxrecomp.h>

extern ptrdiff_t g_xbox_mem_offset;
extern RECOMP_TLS uint32_t g_eax, g_ecx, g_edx, g_ebx, g_esi, g_edi;
extern RECOMP_TLS uint32_t g_esp, g_ebp, g_seh_ebp;
extern uint32_t xbox_HeapAlloc(uint32_t size, uint32_t alignment);
extern void sub_0002F12B(void), sub_0002727C(void), sub_0002A807(void), sub_0002B52A(void);
extern void sub_0002B588(void);
extern void xbox_KbmSetMenuMode(int on), xbox_KbmSetRawMouse(int on), xbox_KbmSetCapture(int on);
extern void xbox_KbmSetBinding(const char *target, const int *keys, int count);
extern void xbox_KbmPulse(const char *target,int ms);
extern int xbox_KbmInputDown(int key), xbox_KbmMenuWheel(void);
extern int xbox_FramebufferKeyDown(int key), xbox_FramebufferMouseButton(int button);
extern int xbox_FramebufferMouseClicks(int button), xbox_FramebufferMouseDelta(int *dx, int *dy);
extern int xbox_FramebufferPointer(int *x, int *y, int *w, int *h);
extern void xbox_FramebufferSetMenuPointer(int on);

#define WORD(a) (*(uint32_t *)((uintptr_t)g_xbox_mem_offset + (uint32_t)(a)))
#define FLOAT(a) (*(float *)((uintptr_t)g_xbox_mem_offset + (uint32_t)(a)))
#define PTR(a) ((void *)((uintptr_t)g_xbox_mem_offset + (uint32_t)(a)))
#define OPTIONS 0x00472568u
#define ACTIVE_MENU 0x0047011Cu
#define MOUSE1 0x101
#define MOUSE2 0x102
#define MOUSE3 0x103
#define WHEEL_UP 0x104
#define WHEEL_DOWN 0x105
#define MOUSE4 0x106
#define MOUSE5 0x107
/* Dispatch tokens in unused alignment padding after Options::Initialize.
 * These pass the runtime's code-range guard; no original code bytes change. */
#define OPEN_PAGE 0x00030CF7u
#define PAGE_KEY  0x00030CF8u
#define OPEN_SETTINGS 0x00030CF9u
#define SETTINGS_KEY  0x00030CFAu
#define VISIBLE_ROWS 9
#define BUTTON_SIZE 0xC8u
#define TEXT_SIZE 128u

enum { FORWARD, BACKWARD, LEFT, RIGHT, JUMP, CROUCH, FIRE, USE, COMPUTER,
       NEXT, PREVIOUS, RELOAD, FAVORITE1, FAVORITE2, THIRD_PERSON, PAUSE, BIND_COUNT };
static const struct { const char *name, *label; int key; uint32_t address; } actions[] = {
    {"forward", "Move forward", 'W', 0x480468},
    {"backward", "Move backward", 'S', 0x48046C},
    {"strafe_left", "Strafe left", 'A', 0x480470},
    {"strafe_right", "Strafe right", 'D', 0x480474},
    {"jump", "Jump / swim up", VK_SPACE, 0x480478},
    {"crouch", "Crouch / swim down", VK_CONTROL, 0x48047C},
    {"fire", "Fire", MOUSE1, 0x480530},
    {"use", "Use / sniper zoom", MOUSE2, 0x480538},
    {"netricsa", "NETRICSA", VK_TAB, 0x48053C},
    {"next_weapon", "Next weapon", WHEEL_UP, 0x480514},
    {"previous_weapon", "Previous weapon", WHEEL_DOWN, 0x480518},
    {"reload", "Reload", 'R', 0x480534},
    {"favorite_1", "Favourite weapon 1", '1', 0x480520},
    {"favorite_2", "Favourite weapon 2", '2', 0x480524},
    {"third_person", "Third-person view", 'H', 0x480548},
    {"pause", "Pause", VK_ESCAPE, 0},
};
enum { SPEED_ROW = BIND_COUNT, INVERT_ROW, DEFAULTS_ROW, ROW_COUNT };
static int keys[BIND_COUNT], speed = 5, invert_y, enabled;
static char ini[MAX_PATH];
static uint32_t page, entry, rows[ROW_COUNT], values[ROW_COUNT], title, help, status, back_button;
static uint32_t settings_page,settings_entry,setting_rows[PC_SETTINGS_ROWS],setting_values[PC_SETTINGS_ROWS];
static int selected_setting;
static void update_settings_page(void),open_settings_page(void),settings_key(void),settings_frame(void);
static int selected, scroll, capturing = -1, menu_mode = -1;
static unsigned char previous[0x108], blocked[BIND_COUNT];
static DWORD capture_deadline;
static int pointer_x=-1,pointer_y=-1,delete_down;
static unsigned char menu_held[6];
static DWORD menu_repeat[4];
static int pause_held;
static const int menu_keys[6][3]={
    {VK_UP,'W',VK_UP},{VK_DOWN,'S',VK_DOWN},{VK_LEFT,'A',VK_LEFT},
    {VK_RIGHT,'D',VK_RIGHT},{VK_RETURN,VK_SPACE,VK_RETURN},{VK_ESCAPE,VK_BACK,VK_ESCAPE}
};
static RECOMP_TLS uint32_t saved_buttons[BIND_COUNT - FIRE];
static RECOMP_TLS int injected;

/* Translated callees share these TLS registers. Helpers restore every one,
 * including the frameless exception-chain frame, before returning to the title. */
static uint32_t call(void (*fn)(void), uint32_t self, const uint32_t *args, unsigned count)
{
    uint32_t save[] = {g_eax,g_ecx,g_edx,g_ebx,g_esi,g_edi,g_esp,g_ebp,g_seh_ebp};
    uint32_t result;
    g_esp -= 4 * (count + 1);
    WORD(g_esp) = 0;
    for (unsigned i = 0; i < count; ++i) WORD(g_esp + 4 + i * 4) = args[i];
    g_ecx = self;
    fn();
    result = g_eax;
    g_eax=save[0];g_ecx=save[1];g_edx=save[2];g_ebx=save[3];g_esi=save[4];
    g_edi=save[5];g_esp=save[6];g_ebp=save[7];g_seh_ebp=save[8];
    return result;
}

static void key_name(int key, char *out, size_t size)
{
    const char *name = NULL;
    switch (key) {
    case 0:name="None";break;
    case VK_SPACE:name="Space";break;case VK_RETURN:name="Enter";break;
    case VK_ESCAPE:name="Escape";break;case VK_BACK:name="Backspace";break;
    case VK_CONTROL:name="Ctrl";break;case VK_SHIFT:name="Shift";break;
    case VK_MENU:name="Alt";break;case VK_TAB:name="Tab";break;
    case MOUSE1:name="Mouse 1";break;case MOUSE2:name="Mouse 2";break;
    case MOUSE3:name="Mouse 3";break;case MOUSE4:name="Mouse 4";break;
    case MOUSE5:name="Mouse 5";break;case WHEEL_UP:name="Wheel up";break;
    case WHEEL_DOWN:name="Wheel down";break;
    }
    if (name) snprintf(out,size,"%s",name);
    else if ((key >= 'A' && key <= 'Z') || (key >= '0' && key <= '9')) snprintf(out,size,"%c",key);
    else {
        UINT scan = MapVirtualKeyA(key, MAPVK_VK_TO_VSC);
        if (key >= VK_PRIOR && key <= VK_DOWN) scan |= 0x100;
        if (!GetKeyNameTextA((LONG)(scan << 16),out,(int)size)) snprintf(out,size,"Key %d",key);
    }
}

static int named_key(const char *name)
{
    char label[64];
    for (int k = 0; k <= MOUSE5; ++k) {
        if (k == 0x100) continue;
        if (k == VK_F11 || k == VK_F12) continue;
        key_name(k,label,sizeof label);
        if (!_stricmp(name,label)) return k;
    }
    return -1;
}

static void save(void)
{
    char label[64];
    for (int i=0;i<BIND_COUNT;++i) {
        key_name(keys[i],label,sizeof label);
        WritePrivateProfileStringA("pc_controls",actions[i].name,label,ini);
    }
    snprintf(label,sizeof label,"%d",speed);
    WritePrivateProfileStringA("pc_controls","mouse_speed",label,ini);
    WritePrivateProfileStringA("pc_controls","invert_y",invert_y ? "1" : "0",ini);
}

static void apply(void)
{
    /* Actions enter Sam's own action builder, independently of the console
     * player's selected stick/button layout. Pause pulses the normal Start
     * input when its configured PC key is pressed. */
    xbox_KbmSetBinding("START",NULL,0);
    xbox_KbmSetRawMouse(1);
}

void pc_controls_load(void)
{
    char text[128];
    const char *flag = getenv("RECOMP_KBM"), *path = getenv("RECOMP_PC_CONTROLS_FILE");
    enabled = !flag || (*flag && *flag != '0');
    if (!flag) _putenv_s("RECOMP_KBM","1");
    if (!getenv("RECOMP_KBM_LOOK_DIRECT")) _putenv_s("RECOMP_KBM_LOOK_DIRECT","0");
    if (!getenv("RECOMP_KBM_WHEEL_MOVE")) _putenv_s("RECOMP_KBM_WHEEL_MOVE","0");
    /* Menu keys remain usable after any gameplay rebinding. No game-specific
     * camera addresses or controller defaults from Turok are used. */
    _putenv_s("RECOMP_KBM_MAP","A=None,B=None,X=None,Y=None,BLACK=None,WHITE=None,LT=None,RT=None,START=None,BACK=None,LTHUMB=None,RTHUMB=None,UP=None,DOWN=None,LEFT=None,RIGHT=None,MOVE_UP=None,MOVE_DOWN=None,MOVE_LEFT=None,MOVE_RIGHT=None");
    _putenv_s("RECOMP_KBM_MENU_MAP","A=None,B=None,START=None,UP=None,DOWN=None,LEFT=None,RIGHT=None,MOVE_UP=None,MOVE_DOWN=None,MOVE_LEFT=None,MOVE_RIGHT=None");
    if (path && *path) snprintf(ini,sizeof ini,"%s",path);
    else {
        DWORD n=GetModuleFileNameA(NULL,ini,sizeof ini);
        while(n && ini[n-1]!='\\' && ini[n-1]!='/') --n;
        snprintf(ini+n,sizeof ini-n,"serious_sam_settings.ini");
    }
    for(int i=0;i<BIND_COUNT;++i) {
        GetPrivateProfileStringA("pc_controls",actions[i].name,"",text,sizeof text,ini);
        int key=named_key(text);
        keys[i]=key >= 0 ? key : actions[i].key;
    }
    speed=(int)GetPrivateProfileIntA("pc_controls","mouse_speed",5,ini);
    if(speed<1 || speed>10) speed=5;
    invert_y=GetPrivateProfileIntA("pc_controls","invert_y",0,ini)==1;
    pc_settings_load(ini);
    fprintf(stderr,"[PC CONTROLS] keyboard/mouse %s; settings %s\n",enabled?"enabled":"disabled",ini);
}

static uint32_t alloc(uint32_t bytes)
{
    uint32_t va=xbox_HeapAlloc(bytes,16);
    if(va) memset(PTR(va),0,bytes);
    return va;
}

static void text(uint32_t gadget,const char *label)
{
    if(gadget) snprintf((char *)PTR(WORD(gadget+0x4C)),TEXT_SIZE,"%s",label);
}

static void position(uint32_t gadget,float x,float y,float scale)
{
    FLOAT(gadget+0x14)=FLOAT(gadget+0x28)=x;
    FLOAT(gadget+0x18)=FLOAT(gadget+0x2C)=y;
    FLOAT(gadget+0x20)=FLOAT(gadget+0x34)=scale;
}

static uint32_t button(uint32_t screen,const char *label,float x,float y,float scale)
{
    uint32_t va=alloc(BUTTON_SIZE+TEXT_SIZE);
    if(!va) return 0;
    /* Sam's ordinary text button, preserving its font and focus renderer. The
     * clone owns its text and is retained for the lifetime of this process. */
    memcpy(PTR(va),PTR(0x46F3C8),BUTTON_SIZE);
    WORD(va+8)=WORD(va+12)=0;
    WORD(va+0x4C)=va+BUTTON_SIZE;
    WORD(va+0x50)=WORD(va+0x90)=0x4493F8; /* title's empty CTString */
    WORD(va+0x58)=WORD(va+0x5C)=WORD(va+0x60)=WORD(va+0x64)=0;
    WORD(va+0x7C)=0;WORD(va+0x80)=WORD(va+0x74)=1;
    WORD(va+0x94)=WORD(va+0x98)=WORD(va+0x9C)=WORD(va+0xA0)=0;
    WORD(va+0xA4)=WORD(va+0xA8)=WORD(va+0xBC)=WORD(va+0xC0)=0;
    WORD(va+0x6C)=0xFFFFFFFFu;WORD(va+0x8C)=2; /* left-aligned text */
    position(va,x,y,scale);text(va,label);
    call(sub_0002727C,screen,&va,1);
    return va;
}

static void update_page(void)
{
    char label[128];
    if(selected<scroll) scroll=selected;
    if(selected>=scroll+VISIBLE_ROWS) scroll=selected-VISIBLE_ROWS+1;
    for(int i=0;i<ROW_COUNT;++i) {
        int visible=i>=scroll && i<scroll+VISIBLE_ROWS;
        WORD(rows[i]+0x80)=WORD(values[i]+0x80)=visible;
        WORD(rows[i]+0x7C)=WORD(values[i]+0x7C)=i==selected;
        position(rows[i],90,145+(i-scroll)*26.0f,0.57f);
        position(values[i],410,145+(i-scroll)*26.0f,0.57f);
        if(i<BIND_COUNT) {
            if(i==capturing) snprintf(label,sizeof label,"Press a key...");
            else key_name(keys[i],label,sizeof label);
        } else if(i==SPEED_ROW) snprintf(label,sizeof label,"< %d / 10 >",speed);
        else if(i==INVERT_ROW) snprintf(label,sizeof label,"< %s >",invert_y?"On":"Off");
        else snprintf(label,sizeof label,"Enter / A");
        text(values[i],label);
    }
    WORD(page+0x14)=rows[selected];
    snprintf(label,sizeof label,"%d - %d of %d",scroll+1,
             scroll+VISIBLE_ROWS<ROW_COUNT?scroll+VISIBLE_ROWS:ROW_COUNT,ROW_COUNT);
    text(status,label);
    text(help,capturing>=0?"Press a key, mouse button or wheel. Esc cancels.":
         "Arrows: select   Enter/A: change   Delete: clear   Esc/B: back");
}

void pc_controls_build_menu(void)
{
    if(page) return;
    page=alloc(0x40+0x2C+TEXT_SIZE);
    if(!page) return;
    call(sub_0002F12B,page,NULL,0);
    memcpy(PTR(page+0x40),PTR(0x3AA210),0x2C);
    WORD(page)=page+0x40;
    WORD(page+0x40+0x18)=PAGE_KEY;
    WORD(page+0x20)=page+0x6C;
    snprintf((char *)PTR(page+0x6C),TEXT_SIZE,"PC Controls");
    title=button(page,"PC CONTROLS",90,88,1.05f);
    for(int i=0;i<ROW_COUNT;++i) {
        const char *label=i<BIND_COUNT?actions[i].label:i==SPEED_ROW?"Mouse speed":i==INVERT_ROW?"Invert mouse Y":"Restore defaults";
        rows[i]=button(page,label,90,145,0.57f);
        values[i]=button(page,"",410,145,0.57f);
        if(!rows[i] || !values[i]) {fprintf(stderr,"[PC CONTROLS] menu allocation failed\n");return;}
    }
    status=button(page,"",90,397,0.43f);
    help=button(page,"",55,430,0.38f);
    back_button=button(page,"BACK",510,397,0.5f);
    entry=button(OPTIONS,"PC CONTROLS",140,372,0.65f);
    if(!entry || !title || !status || !help || !back_button) return;
    WORD(entry+0xBC)=OPEN_PAGE;
    settings_page=alloc(0x40+0x2C+TEXT_SIZE);
    if(!settings_page)return;
    call(sub_0002F12B,settings_page,NULL,0);
    memcpy(PTR(settings_page+0x40),PTR(0x3AA210),0x2C);
    WORD(settings_page)=settings_page+0x40;
    WORD(settings_page+0x40+0x18)=SETTINGS_KEY;
    WORD(settings_page+0x20)=settings_page+0x6C;
    snprintf((char *)PTR(settings_page+0x6C),TEXT_SIZE,"PC Settings");
    if(!button(settings_page,"PC SETTINGS",90,88,1.05f))return;
    for(int i=0;i<PC_SETTINGS_ROWS;++i) {
        setting_rows[i]=button(settings_page,pc_settings_label(i),90,145+i*26.0f,0.57f);
        setting_values[i]=button(settings_page,"",410,145+i*26.0f,0.57f);
        if(!setting_rows[i] || !setting_values[i])return;
    }
    if(!button(settings_page,"Changes apply immediately",90,370,0.43f) ||
       !button(settings_page,"Arrows: select   Left/Right/A: change   Esc/B: back",55,430,0.38f) ||
       !button(settings_page,"BACK",510,397,0.5f))return;
    settings_entry=button(OPTIONS,"PC SETTINGS",140,400,0.65f);
    if(!settings_entry)return;
    WORD(settings_entry+0xBC)=OPEN_SETTINGS;
    update_settings_page();
    update_page();apply();
    fprintf(stderr,"[PC CONTROLS] Options entry %08X, page %08X\n",entry,page);
}

void pc_controls_options_layout(void)
{
    if(!entry || !settings_entry) return;
    static const uint32_t original[]={0x472768,0x472830,0x4728F8,0x4729E8,0x472AB0,0x472BA0};
    uint32_t first=0,last=0;
    int visible=0;
    for(unsigned i=0;i<sizeof original/sizeof original[0];++i) {
        uint32_t row=original[i];
        if(!WORD(row+0x80)) continue;
        position(row,140,157+30.0f*visible,FLOAT(row+0x20));
        ++visible;
        if(!first) first=row;
        last=row;
    }
    if(!last) return;
    position(entry,140,157+30.0f*visible-19,0.65f);
    position(settings_entry,140,157+30.0f*(visible+1)-19,0.65f);
    WORD(last+0xA0)=entry;WORD(first+0x9C)=settings_entry;
    WORD(entry+0x9C)=last;WORD(entry+0xA0)=settings_entry;
    WORD(settings_entry+0x9C)=entry;WORD(settings_entry+0xA0)=first;
}

static int physical_down(int key)
{
    if(key>=MOUSE1 && key<=MOUSE5) {
        int button_index=key==MOUSE1?0:key==MOUSE2?1:key==MOUSE3?2:key==MOUSE4?3:key==MOUSE5?4:-1;
        return button_index>=0 ? xbox_FramebufferMouseButton(button_index):0;
    }
    return key>0 && key<256 && xbox_FramebufferKeyDown(key);
}

static void remember_menu_keys(void)
{
    DWORD now=GetTickCount();
    for(int i=0;i<6;++i) {
        menu_held[i]=(unsigned char)(physical_down(menu_keys[i][0]) || physical_down(menu_keys[i][1]));
        if(i<4)menu_repeat[i]=now+350;
    }
}

static void cancel_capture(void)
{
    capturing=-1;xbox_KbmSetCapture(0);
    remember_menu_keys();
}

static void begin_capture(void)
{
    capturing=selected;capture_deadline=GetTickCount()+6000;
    for(int k=0;k<=MOUSE5;++k) previous[k]=(unsigned char)physical_down(k);
    for(int i=0;i<5;++i) xbox_FramebufferMouseClicks(i);
    xbox_KbmMenuWheel();
    xbox_KbmSetCapture(1);
}

static void set_key(int row,int key)
{
    int old=keys[row];
    for(int i=0;i<BIND_COUNT;++i) if(i!=row && key && keys[i]==key) keys[i]=old;
    keys[row]=key;apply();save();
}

static void change_row(int direction)
{
    if(selected<BIND_COUNT) begin_capture();
    else {
        if(selected==SPEED_ROW) {speed+=direction;if(speed<1)speed=1;if(speed>10)speed=10;}
        else if(selected==INVERT_ROW) invert_y=!invert_y;
        else {for(int i=0;i<BIND_COUNT;++i)keys[i]=actions[i].key;speed=5;invert_y=0;}
        apply();save();
    }
    update_page();
}

static void open_page(void)
{
    uint32_t args[]={page,0};
    if(page && entry) {
        selected=scroll=0;cancel_capture();update_page();
        int w,h;
        xbox_FramebufferPointer(&pointer_x,&pointer_y,&w,&h);
        for(int i=0;i<5;++i) xbox_FramebufferMouseClicks(i);
        xbox_KbmMenuWheel();
        delete_down=xbox_FramebufferKeyDown(VK_DELETE);
        call(sub_0002A807,0,args,2);
    }
    g_esp+=4;
}

static void page_key(void)
{
    uint32_t key=WORD(g_esp+4);
    if(key==VK_ESCAPE) {
        if(capturing>=0) cancel_capture();
        else call(sub_0002B52A,0,NULL,0);
    } else if(capturing<0) {
        if(key==VK_UP) selected=(selected+ROW_COUNT-1)%ROW_COUNT;
        else if(key==VK_DOWN) selected=(selected+1)%ROW_COUNT;
        else if(key==VK_RETURN || key==0x64 || key==VK_LEFT || key==VK_RIGHT) change_row(key==VK_LEFT?-1:1);
        else if(key==VK_DELETE && selected<BIND_COUNT) set_key(selected,0);
    }
    update_page();g_eax=1;g_esp+=12; /* thiscall ret 8 */
}

void (*pc_controls_dispatch(uint32_t address))(void)
{
    if(address==OPEN_PAGE)return open_page;
    if(address==PAGE_KEY)return page_key;
    if(address==OPEN_SETTINGS)return open_settings_page;
    if(address==SETTINGS_KEY)return settings_key;
    return NULL;
}

static void follow_mode(void)
{
    int in_menu=WORD(ACTIVE_MENU)!=0,dx,dy;
    if(menu_mode!=in_menu) {
        menu_mode=in_menu;
        xbox_KbmSetMenuMode(in_menu);
        xbox_FramebufferSetMenuPointer(in_menu);
        xbox_FramebufferMouseDelta(&dx,&dy);
        for(int i=0;i<BIND_COUNT;++i) blocked[i]=(unsigned char)physical_down(keys[i]);
        remember_menu_keys();
        pause_held=physical_down(keys[PAUSE]);
        if(in_menu)xbox_KbmPulse("START",0);
    }
}

/* PC menu events use Sam's own key handler on the render thread. This keeps
 * short taps responsive independently of USB report timing, while real Xbox
 * controllers retain their original input path. */
static void keyboard_frame(void)
{
    if(!enabled)return;
    if(!WORD(ACTIVE_MENU)) {
        int down=physical_down(keys[PAUSE]);
        if(down && !pause_held)xbox_KbmPulse("START",350);
        pause_held=down;
        return;
    }
    if(capturing>=0)return;
    DWORD now=GetTickCount();
    for(int i=0;i<6;++i) {
        int down=physical_down(menu_keys[i][0]) || physical_down(menu_keys[i][1]);
        int press=down && (!menu_held[i] || (i<4 && (int32_t)(now-menu_repeat[i])>=0));
        int was_held=menu_held[i];menu_held[i]=(unsigned char)down;
        if(!press)continue;
        if(i<4)menu_repeat[i]=now+(was_held?100:350);
        uint32_t args[]={(uint32_t)menu_keys[i][2],0};
        call(sub_0002B588,0,args,2);
        if(capturing>=0 || !WORD(ACTIVE_MENU))break;
    }
}

void pc_controls_frame(void)
{
    follow_mode();
    keyboard_frame();
    pc_settings_poll();
    if(settings_page && WORD(ACTIVE_MENU)==settings_page) {
        if(capturing>=0)cancel_capture();
        settings_frame();return;
    }
    if(!page || WORD(ACTIVE_MENU)!=page) {
        if(capturing>=0) cancel_capture();
        return;
    }
    int wheel=xbox_KbmMenuWheel();
    if(capturing>=0) {
        int key=wheel>0?WHEEL_UP:wheel<0?WHEEL_DOWN:0;
        static const int mouse_keys[]={MOUSE1,MOUSE2,MOUSE3,MOUSE4,MOUSE5};
        for(int i=0;i<5;++i) if(xbox_FramebufferMouseClicks(i)) key=mouse_keys[i];
        for(int k=1;k<=MOUSE5;++k) {
            int down=physical_down(k);
            if(down && !previous[k] && k!=VK_F11 && k!=VK_F12) key=k;
            previous[k]=(unsigned char)down;
        }
        if(key==VK_ESCAPE || (int32_t)(GetTickCount()-capture_deadline)>=0) cancel_capture();
        else if(key) {int row=capturing;cancel_capture();set_key(row,key);}
    } else {
        int x,y,w,h;
        int click=xbox_FramebufferMouseClicks(0),right=xbox_FramebufferMouseClicks(1);
        int del=xbox_FramebufferKeyDown(VK_DELETE);
        if(del && !delete_down && selected<BIND_COUNT) set_key(selected,0);
        delete_down=del;
        if(xbox_FramebufferPointer(&x,&y,&w,&h)) {
            /* Sam is presented as 4:3, with pillarboxing in wide windows. */
            float view_w=(float)w,view_h=(float)w*3/4;
            if(view_h>h) {view_h=(float)h;view_w=(float)h*4/3;}
            float left=((float)w-view_w)/2,top=((float)h-view_h)/2;
            float gx=(x-left)*640/view_w, gy=(y-top)*480/view_h;
            /* The title font's glyphs sit below the text button's anchor;
             * use their visible 26-pixel rows, rather than the anchor itself. */
            int row=scroll+(int)((gy-154)/26);
            if(click && gx>=495 && gx<=610 && gy>=390 && gy<425)
                call(sub_0002B52A,0,NULL,0);
            else if(gx>=75 && gx<=610 && gy>=154 && gy<154+VISIBLE_ROWS*26 && row<ROW_COUNT &&
               (x!=pointer_x || y!=pointer_y || click || right)) {
                selected=row;
                if(right && selected<BIND_COUNT) set_key(selected,0);
                else if(click) change_row(1);
            }
            pointer_x=x;pointer_y=y;
        }
        if(wheel) {selected+=wheel>0?-1:1;if(selected<0)selected=0;if(selected>=ROW_COUNT)selected=ROW_COUNT-1;}
    }
    update_page();
}

static void update_settings_page(void)
{
    char value[96],label[128];
    for(int i=0;i<PC_SETTINGS_ROWS;++i) {
        WORD(setting_rows[i]+0x7C)=WORD(setting_values[i]+0x7C)=i==selected_setting;
        pc_settings_value(i,value,sizeof value);
        if(i==PC_VIDEO_DEFAULTS)snprintf(label,sizeof label,"%s",value);
        else snprintf(label,sizeof label,"< %s >",value);
        text(setting_values[i],label);
    }
    WORD(settings_page+0x14)=setting_rows[selected_setting];
}

static void open_settings_page(void)
{
    if(settings_page && settings_entry) {
        int w,h;
        uint32_t args[]={settings_page,0};
        selected_setting=0;cancel_capture();update_settings_page();
        xbox_FramebufferPointer(&pointer_x,&pointer_y,&w,&h);
        for(int i=0;i<5;++i)xbox_FramebufferMouseClicks(i);
        xbox_KbmMenuWheel();
        call(sub_0002A807,0,args,2);
    }
    g_esp+=4;
}

static void settings_key(void)
{
    uint32_t key=WORD(g_esp+4);
    if(key==VK_ESCAPE)call(sub_0002B52A,0,NULL,0);
    else if(key==VK_UP)selected_setting=(selected_setting+PC_SETTINGS_ROWS-1)%PC_SETTINGS_ROWS;
    else if(key==VK_DOWN)selected_setting=(selected_setting+1)%PC_SETTINGS_ROWS;
    else if(key==VK_RETURN || key==0x64 || key==VK_LEFT || key==VK_RIGHT)
        pc_settings_step(selected_setting,key==VK_LEFT?-1:1);
    update_settings_page();g_eax=1;g_esp+=12;
}

static void settings_frame(void)
{
    int x,y,w,h,click=xbox_FramebufferMouseClicks(0),right=xbox_FramebufferMouseClicks(1);
    int wheel=xbox_KbmMenuWheel();
    if(xbox_FramebufferPointer(&x,&y,&w,&h)) {
        float view_w=(float)w,view_h=(float)w*3/4;
        if(view_h>h){view_h=(float)h;view_w=(float)h*4/3;}
        float gx=(x-((float)w-view_w)/2)*640/view_w;
        float gy=(y-((float)h-view_h)/2)*480/view_h;
        int row=(int)((gy-154)/26);
        if(click && gx>=495 && gx<=610 && gy>=390 && gy<425)
            call(sub_0002B52A,0,NULL,0);
        else if(gx>=75 && gx<=630 && gy>=154 && gy<154+PC_SETTINGS_ROWS*26 &&
                (x!=pointer_x || y!=pointer_y || click || right)) {
            selected_setting=row;
            if(click || right)pc_settings_step(row,right?-1:1);
        }
        pointer_x=x;pointer_y=y;
    }
    if(wheel) {
        selected_setting+=wheel>0?-1:1;
        if(selected_setting<0)selected_setting=0;
        if(selected_setting>=PC_SETTINGS_ROWS)selected_setting=PC_SETTINGS_ROWS-1;
    }
    update_settings_page();
}

int pc_controls_input_begin(uint32_t profile)
{
    injected=0;
    follow_mode();
    if(!WORD(ACTIVE_MENU))keyboard_frame();
    if(!enabled || capturing>=0 || WORD(ACTIVE_MENU) || profile<0x10000 || profile>0x07FFFFC0 || WORD(profile+0x38)!=0) return 0;
    /* The original action builder returns early without consuming controls
     * when this player has no entity. Leave those loading/spectator calls alone. */
    uint32_t net=WORD(0x4A2AE0),session=net?WORD(net+0x14):0,players=session?WORD(session+0x10):0;
    if(!players || !WORD(players) || !WORD(players+4)) return 0;
    for(int i=FIRE;i<PAUSE;++i) saved_buttons[i-FIRE]=WORD(actions[i].address);
    for(int i=0;i<PAUSE;++i) {
        int down=xbox_KbmInputDown(keys[i]);
        if(blocked[i]) {if(!down)blocked[i]=0;continue;}
        if(down) {
            if(i<FIRE) {if(FLOAT(actions[i].address)<1) FLOAT(actions[i].address)=1;}
            else WORD(actions[i].address)=1;
        }
    }
    int dx=0,dy=0;
    xbox_FramebufferMouseDelta(&dx,&dy);
    /* ctl_fTurn* are angles, not stick deflections. Cancel the console
     * profile's sensitivity/invert here, then let Sam build the normal action.
     * Equal mouse counts turn equally at 50 or 300 fps, without a stick cap. */
    int sensitivity=*(int8_t *)PTR(profile+0x22);
    float gain=0.10f*powf(1.4f,(float)(speed-5));
    float h=1+(float)sensitivity*(0.75f/127),v=1+(float)sensitivity*(0.5f/127);
    float pitch_sign=(WORD(profile+0x1C)&2)?-1.0f:1.0f;
    FLOAT(0x480484)+=(float)dx*gain/h;
    FLOAT(0x480488)-=(float)dy*gain*(invert_y?-1.0f:1.0f)/v*pitch_sign;
    injected=1;
    return 1;
}

void pc_controls_input_end(void)
{
    if(!injected)return;
    /* Keyboard-held buttons must not remain set in the console script state
     * after release; preserve a simultaneously held controller button. */
    for(int i=FIRE;i<PAUSE;++i) WORD(actions[i].address)=saved_buttons[i-FIRE];
    injected=0;
}
