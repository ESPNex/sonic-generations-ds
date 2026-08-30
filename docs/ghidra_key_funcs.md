# Ghidra: funzioni chiave individuate (code.bin @ 0x00100000, ARM:LE:32:v6)

Progetto Ghidra sul box: /tmp/gproj (11.287 funzioni, 633 taggate via xref stringhe).
Mappa completa: docs/ghidra_func_map.csv

| indirizzo | sorgente | ruolo |
|-----------|----------|-------|
| 0x002f4a88 | gmPly.cpp | player: carica sonic_c.bprm + sonic_m.bprm (init fisica) |
| 0x002d06f8 | Boss/gmPlayerControllerBase.cpp | controller player |
| 0x00111dd0 | gmGmkSpring.cpp | molla |
| 0x0011ee20 | gmGmkDashRing.cpp | dash ring |
| 0x0011f278 | gmGmkMushJump.cpp | fungo salto |
| 0x00124e28 | gmGmkDashPanel.cpp | dash panel |
| 0x0012513c | gmGmkJumpStand.cpp | piattaforma salto |
| 0x0012ae34 | gmGmkSpringPole.cpp | palo-molla |
| 0x00136008 | gmGmkChargeSpring.cpp | molla caricabile |
| 0x0010a35c, 0x002d7c90 | gmCamera.cpp | camera |
| 0x002fdf64 | objCollision.cpp | sistema collisioni |
| 0x0029baa4 | CTRFramework/amCamera.cpp | camera framework |

Nemici gmEne* e boss gmBoss*/gmFB* nello CSV. Prossimo passo:
decompilare il cluster attorno a 0x002f4a88 (loop fisica player) e
0x002fdf64 (query collisioni) per confermare la semantica dei float bprm.
