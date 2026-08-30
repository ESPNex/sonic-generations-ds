/* romcard.h — M6.2: lettura file direttamente dalla ROM (bus card NDS).
 * NitroFS (FNT/FAT) costruito da ndstool -d; accesso raw dal ARM9.
 */
#ifndef ROMCARD_H
#define ROMCARD_H
#include <nds.h>

/* inizializza (owner ARM9, header, FNT/FAT). 0 = ok */
int  romcard_init(void);
/* backend attivo: 1=card bus (emu) 2=file DLDI (SD/flashcart) 0=nessuno */
int  romcard_mode(void);
int  romcard_fat_ok(void);   /* 1 = DLDI montato (diagnostica) */
/* path tipo "amb/PLAYER_CLS.amb" -> id, oppure -1 */
int  romcard_find(const char *path);
u32  romcard_file_size(int id);
/* legge (porzioni di) file; ritorna bytes letti, 0 = errore */
u32  romcard_read(int id, u32 pos, void *dst, u32 len);
/* comodo: file intero in buf (max maxlen); ritorna size o 0 */
u32  romcard_read_file(const char *path, void *dst, u32 maxlen);

#endif
