/* Run real DSP scatter/gather transfers against two colliding RAM banks. */
#include <assert.h>
#include "apu/apu_dsp.c"

static uint8_t ordinary[0x10000], contiguous[0x10000], table[0x1000];
static int bank[16], mapped;
MCPXAPUState *g_state;
struct McpxApuDebug g_dbg, g_dbg_cache;
int g_dbg_voice_monitor = -1;
uint64_t g_dbg_muted_voices[4];
static uint8_t *mapper(uint64_t address, uint32_t bytes)
{
    ++mapped;
    if (address < 0x4000 || address + bytes > 0x8000) return NULL;
    return (bank[address / 4096] == 1 ? ordinary : contiguous) + address;
}
uint8_t *mcpx_apu_ram_address(uint64_t a, uint32_t n)
{
    if (a >= 0x1000 && a + n <= 0x2000) return table + a - 0x1000;
    return mapper(a,n);
}
uint8_t *mcpx_apu_ds_address(uint64_t a, uint32_t n)
{
    assert(a >= 0x1000 && a + n <= 0x2000); return table + a - 0x1000;
}
uint8_t *xbox_ApuPhysicalPointer(uint64_t a, uint32_t n) { return mapper(a,n); }
uint8_t *xbox_ApuScratchPointer(uint64_t a, uint32_t n)
{
    assert(a + n <= sizeof contiguous); return contiguous + a;
}
ptrdiff_t xbox_GetMemoryOffset(void) { return (ptrdiff_t)ordinary; }
void xbox_ApuHostWrite(uint8_t *p, uint32_t n) { (void)p;(void)n; }
int xbox_DmaBankOf(uint64_t a) { return bank[a/4096]; }
int xbox_ContigOwnsOffset(uint64_t a) { (void)a;return 1; }
void xbox_DumpArenas(uint32_t a, uint32_t n) { (void)a;(void)n; }
static void entry(unsigned page, uint32_t physical) { memcpy(table+page*8,&physical,4); }

int main(void)
{
    _putenv_s("RECOMP_APU_DATA_ORDINARY", "");
    _putenv_s("RECOMP_APU_DMA_NOWRITE", "");
    _putenv_s("RECOMP_APU_DMA_TRACE", "");
    MCPXAPUState *d = calloc(1,sizeof *d); assert(d); g_state=d; d->physical_mapper=mapper;
    memset(ordinary,0xA5,sizeof ordinary); memset(contiguous,0xC3,sizeof contiguous);
    entry(0,0x4000);entry(1,0x5000);bank[4]=2;bank[5]=1;
    uint8_t data[32]; memset(data,0x7E,sizeof data);
    /* The initial table describes both contiguous program/control storage
     * and ordinary effects buffers. Bind them before graphics reuses numbers. */
    scatter_gather_rw(d,0,0x1000,1,data,0,1,false);
    assert(data[0]==0xC3 && mapped==2);
    bank[4]=1;bank[5]=2;
    memset(data,0x7E,sizeof data);
    scatter_gather_rw(d,0,0x1000,1,data,4096+17,sizeof data,true);
    assert(ordinary[0x5011]==0x7E && ordinary[0x5030]==0x7E);
    assert(contiguous[0x5011]==0xC3 && ordinary[0x5010]==0xA5 && ordinary[0x5031]==0xA5);
    scatter_gather_rw(d,0,0x1000,1,data,0,1,false);
    assert(data[0]==0xC3 && mapped==2);
    puts("PASS: ordinary DSP effects buffers cannot overwrite colliding graphics textures; program pages stay pinned");

    memset(data,0x6A,sizeof data);
    scatter_gather_rw(d,0,0x1000,1,data,4096-8,16,true);
    assert(contiguous[0x4ff8]==0x6A && ordinary[0x5000]==0x6A);
    assert(ordinary[0x4ff8]==0xA5 && contiguous[0x5000]==0xC3);
    entry(2,0x6000);bank[6]=1;
    scatter_gather_rw(d,0,0x1000,2,data,0,1,false); /* Bind new entries on growth. */
    bank[6]=2;
    memset(data,0x25,sizeof data);
    scatter_gather_rw(d,0,0x1000,2,data,8192+4,1,true);
    assert(ordinary[0x6004]==0x25 && contiguous[0x6004]==0xC3);
    assert(d->dsp_dma_pages[0].host[0]==contiguous+0x4000 && d->dsp_dma_pages[0].host[1]==ordinary+0x5000);
    entry(1,0x7000);bank[7]=2;
    scatter_gather_rw(d,0,0x1000,2,data,4096+2,1,true);
    assert(contiguous[0x7002]==0x25 && ordinary[0x7002]==0xA5);
    d->dsp_dma_epoch[0]++;
    scatter_gather_rw(d,0,0x1000,2,data,0,1,false);
    assert(data[0]==0xA5); /* Re-registering permits a new backing. */
    puts("PASS: page boundaries, table growth, changed descriptors and re-registration preserve correct DMA ownership");

    bank[4]=2; scatter_gather_rw(d,2,0x1000,0,data,0,1,false); assert(data[0]==0xC3);
    bank[4]=1; scatter_gather_rw(d,2,0x1000,0,data,0,1,false); assert(data[0]==0xC3);
    scatter_gather_rw(d,3,0x1000,0,data,0,1,false); assert(data[0]==0xA5);
    memset(data,0x77,sizeof data);
    scatter_gather_rw(d,1,0x1000,32768,data,0,sizeof data,false);
    for(unsigned i=0;i<sizeof data;i++) assert(data[i]==0);
    memset(data,0x77,sizeof data);
    scatter_gather_rw(d,1,0x1000,0,data,4096,sizeof data,false);
    for(unsigned i=0;i<sizeof data;i++) assert(data[i]==0);
    puts("PASS: GP/EP FIFO bindings are independent and malformed extents return silence");
    mcpx_apu_dsp_dma_finalize(d); free(d); g_state=NULL;
    return 0;
}
