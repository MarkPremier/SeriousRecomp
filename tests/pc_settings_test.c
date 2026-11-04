#include "../src/pc_settings.c"
#define CHECK(x) do {if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static float scale;
static int anisotropy,samples,want_fullscreen,actual_fullscreen,interval,limit;
void nv2a_gpu_set_render_scale(float value){scale=value;}
void nv2a_gpu_set_anisotropy(int value){anisotropy=value;}
void nv2a_gpu_set_msaa(int value){samples=value;}
void xbox_FramebufferSetFullscreen(int value){want_fullscreen=value;}
int xbox_FramebufferFullscreen(void){return actual_fullscreen;}
void xbox_SetPresentInterval(int value){interval=value;}
int xbox_GetPresentInterval(void){return interval;}
void xbox_SetFrameLimit(int value){limit=value;}
int xbox_GetFrameLimit(void){return limit;}
static float window_aspect;
void xbox_FramebufferSetWindowAspect(float value){window_aspect=value;}
int main(void)
{
    char temp[MAX_PATH],path[MAX_PATH],text[64];
    GetTempPathA(sizeof temp,temp);
    snprintf(path,sizeof path,"%ssam_video_%lu.ini",temp,GetCurrentProcessId());
    _putenv_s("RECOMP_PC_SETTINGS_FILE","");_putenv_s("RECOMP_RENDER_SCALE","");
    _putenv_s("RECOMP_VSYNC","");_putenv_s("RECOMP_FPS_CAP","");
    WritePrivateProfileStringA("pc_controls","fire","F",path);
    pc_settings_load(path);
    CHECK(scale==0 && anisotropy==16 && samples==1 && !interval && !limit && !want_fullscreen);
    CHECK(choices[PC_ASPECT]==COUNT(aspects)-1 && pc_settings_aspect()>=4.0f/3 && window_aspect==pc_settings_aspect());
    choices[PC_ASPECT]=COUNT(aspects)-1;
    pc_settings_step(PC_ASPECT,1);CHECK(fabsf(pc_settings_aspect()-4.0f/3)<1e-6f && window_aspect==pc_settings_aspect());
    pc_settings_step(PC_ASPECT,-1);pc_settings_step(PC_ASPECT,-1);
    CHECK(fabsf(pc_settings_aspect()-64.0f/27)<1e-6f); /* 21:9 */
    pc_settings_value(PC_ASPECT,text,sizeof text);CHECK(!strcmp(text,"21:9"));
    pc_settings_step(PC_RESOLUTION,1);CHECK(scale==1);
    pc_settings_step(PC_RESOLUTION,1);CHECK(scale==2);
    pc_settings_step(PC_AA,1);CHECK(samples==2);
    pc_settings_step(PC_AA,1);CHECK(samples==4);
    pc_settings_step(PC_AF,-1);CHECK(anisotropy==8);
    pc_settings_step(PC_FRAMES,1);CHECK(limit==30);
    pc_settings_step(PC_FRAMES,1);CHECK(limit==60);
    pc_settings_step(PC_VSYNC,1);CHECK(interval==1);
    pc_settings_step(PC_DISPLAY,1);CHECK(want_fullscreen==1);
    pc_settings_poll();CHECK(choices[PC_DISPLAY]==1); /* asynchronous request is not undone */
    actual_fullscreen=1;pc_settings_poll();CHECK(choices[PC_DISPLAY]==1);
    actual_fullscreen=0;pc_settings_poll();CHECK(choices[PC_DISPLAY]==0); /* Alt+Enter */
    pc_settings_load(path);CHECK(scale==2 && samples==4 && anisotropy==8 && limit==60 && interval==1);
    CHECK(fabsf(pc_settings_aspect()-64.0f/27)<1e-6f); /* the aspect persists */
    GetPrivateProfileStringA("pc_controls","fire","",text,sizeof text,path);CHECK(!strcmp(text,"F"));
    _putenv_s("RECOMP_RENDER_SCALE","5");_putenv_s("RECOMP_FPS_CAP","75");_putenv_s("RECOMP_VSYNC","0");
    pc_settings_load(path);CHECK(scale==5 && limit==75 && interval==0);
    pc_settings_value(PC_RESOLUTION,text,sizeof text);CHECK(!strcmp(text,"5x scale"));
    pc_settings_step(PC_AF,1); /* saving a custom value preserves it */
    _putenv_s("RECOMP_RENDER_SCALE","");_putenv_s("RECOMP_FPS_CAP","");_putenv_s("RECOMP_VSYNC","");
    pc_settings_load(path);CHECK(scale==5 && limit==75);
    pc_settings_step(PC_VIDEO_DEFAULTS,1);CHECK(scale==0 && limit==0 && samples==1 && anisotropy==16 && interval==0);
    WritePrivateProfileStringA("pc_settings","resolution","garbage",path);
    WritePrivateProfileStringA("pc_settings","msaa","99",path);
    pc_settings_load(path);CHECK(scale==0 && samples==1);
    _putenv_s("RECOMP_FPS_CAP","vblank");pc_settings_load(path);CHECK(limit==-2);
    DeleteFileA(path);puts("PC graphics live values, persistence, overrides and fullscreen synchronization passed");return 0;
}
