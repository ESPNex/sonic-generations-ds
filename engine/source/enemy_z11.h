/* enemy_z11.h — spawn E_BAT ORIGINALI da z11evt.bin (type 14, stream C) */
/* 3 nemici; x evt [100..3028] -> corso [0..120] */
#include <nds.h>

#define Z11_NENE 3
const s32 z11_enemies[Z11_NENE][3] = {  /* x(f16), hover, f3 */
  { 349167, 14 << 16, 164 },  /* evt X=230 Y=100.5 f3=164 C */
  { 3201595, 14 << 16, 140 },  /* evt X=1292 Y=100.5 f3=140 C */
  { 5605477, 10 << 16, 164 },  /* evt X=2187 Y=0.5 f3=164 C */
};
