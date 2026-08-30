/* romcard.c — M6.2/M6.5 asset reader: NitroFS FNT/FAT con DUE backend:
 *
 *  1) FILE via DLDI (SD con TWiLightMenu++/nds-bootstrap, flashcart):
 *     il loader DLDI-patcha lo stub dentro libnds9; il gioco si ritrova
 *     da solo: prima argv[0] (nds-bootstrap passa il path), poi scansione
 *     delle cartelle comuni. Identita' verificata dal CONTENUTO nitrofs
 *     ("amb/ZONE1_1_ENE.amb" presente), non dall'header generico.
 *  2) CARD BUS 0xB7 (emulatori / cartucce reali): uguale a prima ma con
 *     TIMEOUT anti-hang (slot-1 vuoto su DSi non risponde mai).
 *  3) Nessun backend -> il chiamante usa il fallback baked.
 *
 * Card bus: costanti da libnds card.c — command scritto INVERTITO a
 * 0x040001A8, ROMCTRL 0x040001A4, data 0x04100010; polled transfer.
 * FNT: dir main table 8B {u32 sub_off, u16 first_file_id, u16 parent};
 *      subtable: {u8 len(bit7=dir), name[len&7F], (u16 subdir se dir)}, 0=fine.
 * FAT: entry 8B {u32 top, u32 bottom} offset assoluti ROM.
 * Validato offline (python) contro ndstool 2.0.3: lookup byte-perfect.
 */
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <fat.h>
#include "romcard.h"

/* ---------------- card bus (backend 1) ---------------- */
#define REG_CARD_DATA_RD   (*(vu32*)0x04100010)
#define REG_ROMCTRL        (*(vu32*)0x040001A4)
#define REG_AUXSPICNTH     (*(vu8*)0x040001A1)
#define CARD_CMD_BUF       ((vu8*)0x040001A8)

#ifndef CARD_ACTIVATE     /* gia' definite in nds/card.h via nds.h */
#define CARD_ACTIVATE   (1u<<31)
#define CARD_nRESET     (1u<<29)
#define CARD_CLK_SLOW   (1u<<27)
#define CARD_BLK_SIZE(n) (((n)&7u)<<24)
#define CARD_DELAY2(n)  (((n)&0x3Fu)<<16)
#define CARD_DELAY1(n)  ((n)&0x1FFFu)
#define CARD_BUSY       (1u<<31)
#define CARD_DATA_READY (1u<<23)
#endif

/* flags identici a cardReadHeader (lettura plain, area non cifrata) */
#define RD_FLAGS (CARD_ACTIVATE|CARD_nRESET|CARD_CLK_SLOW|CARD_BLK_SIZE(1)| \
                  CARD_DELAY1(0x1FFF)|CARD_DELAY2(0x3F))

/* ---------------- stato ---------------- */
static int  g_mode;                    /* 0=nessuno 1=card 2=file(DLDI) */
static FILE *g_file;                   /* backend 2 */
static u32  g_fnt, g_fat, g_romsize;
static u32  g_hdr[128];                /* header 512B */
static int  card_dead;                 /* timeout bus card */
static int  g_fatok;                  /* diagnostico: DLDI montato */

int romcard_mode(void) { return g_mode; }
int romcard_fat_ok(void) { return g_fatok; } /* diagnostica */
int romcard_find_probe(void);   /* fwd: probe identita nitrofs */

static void card_polled(u32 flags, u32 *dst, int nwords, const u8 cmd[8]) {
    REG_AUXSPICNTH = 0xC0;                    /* enable|irq */
    for (int i = 0; i < 8; i++) CARD_CMD_BUF[7-i] = cmd[i];
    REG_ROMCTRL = flags;
    u32 *end = dst + nwords;
    u32 guard = 0;
    while (REG_ROMCTRL & CARD_BUSY) {
        if (++guard > 4000000u) { card_dead = 1; break; }  /* slot vuoto */
        if (REG_ROMCTRL & CARD_DATA_READY) {
            u32 v = REG_CARD_DATA_RD;
            if (dst && dst < end) *dst++ = v;
        }
    }
}

/* un blocco da 0x200 byte all'indirizzo rom `pos` (multiplo di 0x200) */
static void card_read_block(u32 pos, void *dst) {
    u8 cmd[8] = {0,0,0,0,0,0,0,0};
    cmd[7] = 0xB7;                       /* DATA READ */
    cmd[6] = (u8)(pos >> 24);
    cmd[5] = (u8)(pos >> 16);
    cmd[4] = (u8)(pos >> 8);
    cmd[3] = (u8)pos;
    card_polled(RD_FLAGS, (u32*)dst, 0x200/4, cmd);
}

/* ---------------- file DLDI (backend 2) ---------------- */
static u32 file_read_rom(u32 pos, void *dst, u32 len) {
    if (!g_file) return 0;
    if (fseek(g_file, (long)pos, SEEK_SET)) return 0;
    return fread(dst, 1, len, g_file);
}

/* ---------------- dispatch lettura ROM ---------------- */
static u32 rom_read(u32 pos, void *dst, u32 len) {
    if (g_mode == 2) return file_read_rom(pos, dst, len);
    if (g_mode != 1 || card_dead) return 0;
    static u32 blk[0x200/4];
    u8 *p = (u8*)dst;
    u32 done = 0;
    if (pos & 0x1FF) {
        u32 base = pos & ~(u32)0x1FF;
        u32 amt = 0x200 - (pos & 0x1FF);
        if (amt > len) amt = len;
        card_read_block(base, blk);
        memcpy(p, ((u8*)blk) + (pos & 0x1FF), amt);
        p += amt; done += amt; pos += amt; len -= amt;
    }
    while (len >= 0x200) {
        card_read_block(pos, p);
        p += 0x200; done += 0x200; pos += 0x200; len -= 0x200;
    }
    if (len) {
        card_read_block(pos, blk);
        memcpy(p, blk, len);
        done += len;
    }
    return done;
}

/* ---------------- validazione: siamo NOI? ---------------- */
static int probe_core(void) {
    if (rom_read(0, g_hdr, 0x200) != 0x200) return -1;
    u32 fnt  = g_hdr[0x40/4];
    u32 fat  = g_hdr[0x48/4];
    u32 fatn = g_hdr[0x4C/4];
    u32 rs   = g_hdr[0x80/4];
    if (fnt < 0x200 || fat < 0x200 || !fatn) return -1;
    if (rs < 0x4000 || rs > 0x10000000u) return -1;
    if ((u64)fat + 8ull*(u64)fatn > (u64)rs) return -1;
    g_fnt = fnt; g_fat = fat; g_romsize = rs;
    /* identita': il nostro nitrofs ha questo file (definito altrove, ma
     * romcard_find e' piu' avanti nel file — fwd decl) */
    return romcard_find_probe();
}

/* ---------------- argv (nds-bootstrap/TWLMenu passano il path) -------- */
static const char *argv0(void) {
    vu32 *m = (vu32*)0x02FFFE70;            /* cookie argv devkitARM */
    if (m[0] != 0x020F2020u) return 0;
    vu32 *as = (vu32*)m[1];
    if ((u32)as < 0x02000000u || (u32)as > 0x04000000u) return 0;
    if ((s32)as[0] < 1) return 0;           /* argc */
    const char *cl = (const char*)as[1];    /* commandLine: "path\0..." */
    if ((u32)cl < 0x02000000u || (u32)cl > 0x04000000u || !cl[0]) return 0;
    return cl;
}

static int try_path(const char *p) {
    FILE *f = fopen(p, "rb");
    if (!f) return 0;
    g_file = f; g_mode = 2;          /* la probe deve LEGGERE via file! */
    if (probe_core() == 0) return 1;
    fclose(f); g_file = 0; g_mode = 0;
    return 0;
}

/* argv puo' arrivare come "sd:/..." o "/..." oltre a "fat:/..." */
static int try_argv(const char *a0) {
    if (try_path(a0)) return 1;
    if (!strncmp(a0, "sd:", 3)) a0 += 3;
    if (a0[0] == '/') {
        char buf[192];
        snprintf(buf, sizeof buf, "fat:%s", a0);
        return try_path(buf);
    }
    return 0;
}

/* ---------------- scansione cartelle comuni ---------------- */
static int is_nds_ext(const char *e, size_t L) {
    if (L < 5) return 0;
    const char *ext = e + L - 4;
    return !strcasecmp(ext, ".nds") || !strcasecmp(ext, ".srl")
        || !strcasecmp(ext, ".ids");
}

static int scan_rom(void) {
    static const char *dirs[] = { "", "_nds", "roms", "games", "nds", "apps", 0 };
    for (int di = 0; dirs[di]; di++) {
        char dp[96];
        snprintf(dp, sizeof dp, "fat:/%s", dirs[di]);
        DIR *d = opendir(dp);
        if (!d) continue;
        struct dirent *de;
        int n = 0;
        while ((de = readdir(d)) != 0 && n++ < 300) {
            if (!is_nds_ext(de->d_name, strlen(de->d_name))) continue;
            char fp[192];
            if (dp[strlen(dp)-1] == '/')
                 snprintf(fp, sizeof fp, "%s%s", dp, de->d_name);
            else snprintf(fp, sizeof fp, "%s/%s", dp, de->d_name);
            if (try_path(fp)) { closedir(d); return 1; }
        }
        closedir(d);
    }
    return 0;
}

/* ---------------- init ---------------- */
int romcard_init(void) {
    g_mode = 0;
    /* 1) SD/flashcart via DLDI (patchato dal loader: TWLMenu++ ecc.) */
    if (fatInitDefault()) {
        g_fatok = 1;
        const char *a0 = argv0();
        if ((a0 && try_argv(a0)) || scan_rom()) { g_mode = 2; return 0; }
    }
    /* 2) bus card = cartuccia con DENTRO noi (emulatori) */
    REG_EXMEMCNT = (REG_EXMEMCNT & ~ARM7_OWNS_CARD);
    g_mode = 1; g_file = 0;
    if (!card_dead && probe_core() == 0) return 0;
    g_mode = 0;
    return -1;
}

/* --- FNT --- */
#define NAMELEN 128

static int fnt_dir(u32 dir_idx, u32 *sub_off, u16 *first_id) {
    /* entry 8B: {u32 sub_off (rel FNT), u16 first_file_id, u16 parent} */
    u32 ent[2];
    if (rom_read(g_fnt + 8*dir_idx, ent, 8) != 8) return -1;
    *sub_off = g_fnt + ent[0];
    *first_id = (u16)(ent[1] & 0xFFFF);
    return 0;
}

static int fnt_scan(u32 dir_idx, int (*cb)(const char*, int, void*), void *ctx) {
    u32 sub; u16 fid;
    if (fnt_dir(dir_idx, &sub, &fid)) return -1;
    static u8 buf[0x200];
    if (rom_read(sub, buf, 0x200) != 0x200) return -1;
    u32 off = 0;
    while (off < 0x200 - 3) {
        u8 l = buf[off];
        if (l == 0) break;
        char name[NAMELEN];
        int nl = l & 0x7F;
        if (nl >= NAMELEN) nl = NAMELEN - 1;
        memcpy(name, &buf[off+1], nl);
        name[nl] = 0;
        if (l & 0x80) {
            int ident = 0xF000 | (buf[off+1+nl] | (buf[off+2+nl] << 8));
            cb(name, ident, ctx);
            off += 1 + nl + 2;
        } else {
            cb(name, fid, ctx);
            fid++;
            off += 1 + nl;
        }
    }
    return 0;
}

struct find_ctx { const char *want; int found; };
static int find_cb(const char *name, int ident, void *ctx) {
    struct find_ctx *f = (struct find_ctx*)ctx;
    if (strcmp(name, f->want) == 0) f->found = ident;
    return 0;
}

int romcard_find(const char *path) {
    u32 cur = 0xF000;
    char comp[NAMELEN];
    while (*path) {
        while (*path == '/') path++;
        int n = 0;
        while (*path && *path != '/' && n < NAMELEN-1) comp[n++] = *path++;
        comp[n] = 0;
        if (!n) break;
        struct find_ctx fc = { comp, -1 };
        if (fnt_scan(cur & 0xFFF, find_cb, &fc)) return -1;
        if (fc.found < 0) return -1;
        cur = (u32)fc.found;
    }
    if (cur & 0xF000) return -1;
    return (int)cur;
}

/* probe: il nitrofs del candidato contiene il nostro file piu' raro */
int romcard_find_probe(void) {
    return romcard_find("amb/ZONE1_1_ENE.amb") >= 0 ? 0 : -1;
}

u32 romcard_file_size(int id) {
    u32 fat[2];
    if (rom_read(g_fat + 8*id, fat, 8) != 8) return 0;
    return fat[1] - fat[0];
}

u32 romcard_read(int id, u32 pos, void *dst, u32 len) {
    u32 fat[2];
    if (rom_read(g_fat + 8*id, fat, 8) != 8) return 0;
    u32 top = fat[0], size = fat[1] - fat[0];
    if (pos >= size) return 0;
    if (pos + len > size) len = size - pos;
    return rom_read(top + pos, dst, len);
}

u32 romcard_read_file(const char *path, void *dst, u32 maxlen) {
    int id = romcard_find(path);
    if (id < 0) return 0;
    u32 sz = romcard_file_size(id);
    if (sz == 0 || sz > maxlen) return 0;
    return romcard_read(id, 0, dst, sz);
}
