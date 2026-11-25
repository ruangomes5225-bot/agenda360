#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <xenos/xenos.h>
#include <console/console.h>
#include <input/input.h>
#include <usb/usbmain.h>
#include <diskio/ata.h>
#include <time/time.h>
#include <xenon_soc/xenon_power.h>

void kmem_init(void);

#define MAX_NOTES 200
#define TEXT_LEN 100
#define SAVE_FILE "uda:/agenda360.txt"

typedef struct { int y, m, d; char text[TEXT_LEN]; } Note;

static Note notes[MAX_NOTES];
static int count = 0;
static int cy = 2026, cm = 11, cd = 25;
static const char *status = "";
static struct controller_data_s c, p;

static const char *KEYS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,!?-:";
#define COLS 10

static int dim(int y, int m) {
    static const int t[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0)) return 29;
    return t[m - 1];
}

static void add_days(int n) {
    while (n > 0) { cd++; if (cd > dim(cy, cm)) { cd = 1; cm++; if (cm > 12) { cm = 1; cy++; } } n--; }
    while (n < 0) { cd--; if (cd < 1) { cm--; if (cm < 1) { cm = 12; cy--; } cd = dim(cy, cm); } n++; }
}

static int weekday(int y, int m, int d) {
    static const int t[] = {0,3,2,5,0,3,5,1,4,6,2,4};
    if (m < 3) y -= 1;
    return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
}

static int find_note(void) {
    for (int i = 0; i < count; i++)
        if (notes[i].y == cy && notes[i].m == cm && notes[i].d == cd) return i;
    return -1;
}

static void save_notes(void) {
    FILE *f = fopen(SAVE_FILE, "w");
    if (!f) { status = "Nao foi possivel salvar (sem pendrive?)"; return; }
    for (int i = 0; i < count; i++)
        fprintf(f, "%04d-%02d-%02d %s\n", notes[i].y, notes[i].m, notes[i].d, notes[i].text);
    fclose(f);
    status = "Salvo no pendrive.";
}

static void load_notes(void) {
    FILE *f = fopen(SAVE_FILE, "r");
    if (!f) return;
    char line[TEXT_LEN + 16];
    while (count < MAX_NOTES && fgets(line, sizeof line, f)) {
        int y, m, d;
        if (sscanf(line, "%d-%d-%d", &y, &m, &d) == 3 && strlen(line) > 11) {
            char *t = line + 11;
            t[strcspn(t, "\r\n")] = 0;
            notes[count].y = y; notes[count].m = m; notes[count].d = d;
            strncpy(notes[count].text, t, TEXT_LEN - 1);
            notes[count].text[TEXT_LEN - 1] = 0;
            count++;
        }
    }
    fclose(f);
}

static int dbg_x[4];

static void poll(void) {
    struct controller_data_s t;
    p = c;
    usb_do_poll();
    memset(&c, 0, sizeof c);
    for (int i = 0; i < 4; i++) {
        memset(&t, 0, sizeof t);
        get_controller_data(&t, i);
        dbg_x[i] = t.s1_x;
        c.a |= t.a; c.b |= t.b; c.x |= t.x; c.y |= t.y;
        c.up |= t.up; c.down |= t.down; c.left |= t.left; c.right |= t.right;
        c.start |= t.start; c.back |= t.back;
    }
}
#define PRESSED(b) (c.b && !p.b)

static void edit_note(void) {
    char buf[TEXT_LEN];
    int i = find_note();
    if (i >= 0) strncpy(buf, notes[i].text, TEXT_LEN - 1); else buf[0] = 0;
    buf[TEXT_LEN - 1] = 0;
    int len = strlen(buf), sel = 0, nkeys = strlen(KEYS);

    for (;;) {
        poll();
        if (PRESSED(left) && sel > 0) sel--;
        if (PRESSED(right) && sel < nkeys - 1) sel++;
        if (PRESSED(up) && sel >= COLS) sel -= COLS;
        if (PRESSED(down) && sel + COLS < nkeys) sel += COLS;
        if (PRESSED(a) && len < TEXT_LEN - 1) { buf[len++] = KEYS[sel]; buf[len] = 0; }
        if (PRESSED(x) && len < TEXT_LEN - 1) { buf[len++] = ' '; buf[len] = 0; }
        if (PRESSED(b) && len > 0) { buf[--len] = 0; }
        if (PRESSED(back)) { status = "Cancelado."; return; }
        if (PRESSED(start)) {
            i = find_note();
            if (len == 0) {
                if (i >= 0) { notes[i] = notes[count - 1]; count--; }
            } else {
                if (i < 0 && count < MAX_NOTES) { i = count++; notes[i].y = cy; notes[i].m = cm; notes[i].d = cd; }
                if (i >= 0) strcpy(notes[i].text, buf);
            }
            save_notes();
            return;
        }

        console_clrscr();
        printf("EDITAR %02d/%02d/%04d\n\n", cd, cm, cy);
        printf("%s_\n\n", buf);
        for (int k = 0; k < nkeys; k++) {
            if (k == sel) printf("[%c]", KEYS[k]); else printf(" %c ", KEYS[k]);
            if ((k + 1) % COLS == 0) printf("\n");
        }
        printf("\n\nDirecional: mover | A: letra | X: espaco | B: apagar\n");
        printf("START: salvar | BACK: cancelar\n");
        udelay(30000);
    }
}

int main(void) {
    xenon_make_it_faster(XENON_SPEED_FULL);
    xenos_init(VIDEO_MODE_AUTO);
    console_init();
    kmem_init();
    usb_init();
    usb_do_poll();
    xenon_ata_init();
    memset(&c, 0, sizeof c);
    memset(&p, 0, sizeof p);
    for (int w = 0; w < 100; w++) {
        console_clrscr();
        printf("Procurando controle USB... %d\n", 100 - w);
        poll();
        udelay(50000);
    }
    load_notes();

    static const char *dias[] = {"Dom","Seg","Ter","Qua","Qui","Sex","Sab"};
    for (;;) {
        poll();
        if (PRESSED(left))  { add_days(-1); status = ""; }
        if (PRESSED(right)) { add_days(1);  status = ""; }
        if (PRESSED(up))    { add_days(-7); status = ""; }
        if (PRESSED(down))  { add_days(7);  status = ""; }
        if (PRESSED(a)) edit_note();

        console_clrscr();
        printf("=== AGENDA 360 ===\n\n");
        printf("%s %02d/%02d/%04d\n\n", dias[weekday(cy, cm, cd)], cd, cm, cy);
        int i = find_note();
        printf("%s\n\n", i >= 0 ? notes[i].text : "(sem anotacao neste dia)");
        printf("Proximas anotacoes:\n");
        int shown = 0;
        for (int k = 0; k < count && shown < 5; k++) {
            if (notes[k].y * 10000 + notes[k].m * 100 + notes[k].d >= cy * 10000 + cm * 100 + cd) {
                printf(" %02d/%02d  %.40s\n", notes[k].d, notes[k].m, notes[k].text);
                shown++;
            }
        }
        printf("\nEsq/Dir: dia | Cima/Baixo: semana | A: editar\n");
        printf("%s\n", status);
        printf("\n[teste] a=%d up=%d dir=%d x0=%d x1=%d x2=%d x3=%d\n",
               c.a, c.up, c.right, dbg_x[0], dbg_x[1], dbg_x[2], dbg_x[3]);
        udelay(30000);
    }
    return 0;
}
