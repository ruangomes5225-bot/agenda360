#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <xenos/xenos.h>
#include <console/console.h>
#include <input/input.h>
#include <usb/usbmain.h>
#include <time/time.h>
#include <xenon_soc/xenon_power.h>

void kmem_init(void);

#define BAR 40
#define POS_MAX 400
#define ZONA_MIN 150
#define ZONA_MAX 250

static struct controller_data_s c, p;
static int dbg_x[4];
static unsigned int seed = 12345;

static int rnd(int n) {
    seed = seed * 1103515245u + 12345u;
    return (int)((seed >> 16) % (unsigned)n);
}

static void poll(void) {
    struct controller_data_s t;
    p = c;
    usb_do_poll();
    memset(&c, 0, sizeof c);
    for (int i = 0; i < 4; i++) {
        memset(&t, 0, sizeof t);
        get_controller_data(&t, i);
        dbg_x[i] = t.s1_x;
        c.a |= t.a;
        c.start |= t.start;
    }
}

static void barra(int pos) {
    int marca = pos * (BAR - 1) / POS_MAX;
    int z1 = ZONA_MIN * (BAR - 1) / POS_MAX;
    int z2 = ZONA_MAX * (BAR - 1) / POS_MAX;
    printf("[");
    for (int i = 0; i < BAR; i++) {
        if (i == marca) printf("#");
        else if (i >= z1 && i <= z2) printf("-");
        else printf(".");
    }
    printf("]\n");
}

int main(void) {
    int estado = 0; /* 0 espera, 1 jogando, 2 caiu */
    int pos = 120, pontos = 0, recorde = 0, frame = 0;
    const char *motivo = "";

    xenon_make_it_faster(XENON_SPEED_FULL);
    xenos_init(VIDEO_MODE_AUTO);
    console_init();
    kmem_init();
    usb_init();
    usb_do_poll();
    memset(&c, 0, sizeof c);
    memset(&p, 0, sizeof p);

    for (int w = 0; w < 60; w++) {
        console_clrscr();
        printf("Procurando controle USB... %d\n", 60 - w);
        poll();
        udelay(50000);
    }

    for (;;) {
        poll();
        frame++;
        seed += frame + dbg_x[0];

        if (estado == 0) {
            if (c.a && !p.a) { estado = 1; pos = 120; pontos = 0; }
        } else if (estado == 1) {
            pos += c.a ? 9 : -6;
            pos += rnd(9) - 4;
            if (pos >= ZONA_MIN && pos <= ZONA_MAX) pontos++;
            if (pos <= 0) { estado = 2; motivo = "A roda nao subiu. Segura o A!"; }
            if (pos >= POS_MAX) { estado = 2; motivo = "Empinou demais e caiu de costas!"; }
            if (estado == 2 && pontos > recorde) recorde = pontos;
        } else {
            if (c.a && !p.a) { estado = 1; pos = 120; pontos = 0; }
        }

        console_clrscr();
        printf("=== JOGO DO GRAU ===\n\n");
        if (estado == 0) {
            printf("Segure A para empinar a moto.\n");
            printf("Solte para abaixar. Fique na zona '-'.\n\n");
            printf("Aperte A para comecar!\n");
        } else {
            barra(pos);
            printf("\n");
            if (pos < ZONA_MIN) printf("Roda quase no chao... acelera!\n");
            else if (pos <= ZONA_MAX) printf("GRAU PERFEITO!\n");
            else printf("Cuidado, empinando demais!\n");
            printf("\nDistancia: %d m   Recorde: %d m\n", pontos / 5, recorde / 5);
            if (estado == 2) {
                printf("\nVOCE CAIU: %s\n", motivo);
                printf("Aperte A para tentar de novo.\n");
            }
        }
        printf("\n[teste] a=%d x0=%d x1=%d x2=%d x3=%d\n", c.a, dbg_x[0], dbg_x[1], dbg_x[2], dbg_x[3]);
        udelay(50000);
    }
    return 0;
}
