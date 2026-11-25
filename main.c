#include <stdio.h>
#include <time.h>
#include <xenos/xenos.h>
#include <console/console.h>
#include <time/time.h>
#include <xenon_soc/xenon_power.h>

int main(void) {
    static const char *dias[] = {"Dom", "Seg", "Ter", "Qua", "Qui", "Sex", "Sab"};

    xenon_make_it_faster(XENON_SPEED_FULL);
    xenos_init(VIDEO_MODE_AUTO);
    console_init();

    for (;;) {
        time_t now = time(NULL);
        console_clrscr();
        printf("=== RELOGIO BRASILIA ===\n\n");

        if (now < 1577836800) { /* antes de 01/01/2020: sem hora valida */
            printf("O relogio do console nao deu uma hora valida.\n");
            printf("Valor lido: %ld\n", (long)now);
        } else {
            time_t br = now - 3 * 3600; /* UTC-3 */
            struct tm *t = gmtime(&br);
            printf("%s %02d/%02d/%04d\n\n", dias[t->tm_wday], t->tm_mday,
                   t->tm_mon + 1, t->tm_year + 1900);
            printf("      %02d:%02d:%02d\n\n", t->tm_hour, t->tm_min, t->tm_sec);
            printf("(hora de Brasilia, UTC-3)\n\n");
            printf("[teste] valor lido: %ld\n", (long)now);
        }
        udelay(500000);
    }
    return 0;
}
