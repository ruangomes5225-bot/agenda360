#include <xenos/xenos.h>
#include <time/time.h>
#include <xenon_soc/xenon_power.h>

extern struct XenosDevice *g_pVideoDevice;

int main(void) {
    unsigned int cores[4] = {0xFFFF0000, 0xFF00FF00, 0xFF0000FF, 0xFFFFFFFF};
    int i = 0;

    xenon_make_it_faster(XENON_SPEED_FULL);
    xenos_init(VIDEO_MODE_AUTO);

    struct XenosDevice *xe = g_pVideoDevice;
    Xe_SetRenderTarget(xe, Xe_GetFramebufferSurface(xe));

    for (;;) {
        Xe_SetClearColor(xe, cores[i % 4]);
        Xe_Resolve(xe);
        Xe_Sync(xe);
        i++;
        udelay(1000000);
    }
    return 0;
}
