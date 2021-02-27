#include "game/effects.h"
#include "game/vars.h"
#include "config.h"
#include "util.h"

void FxChainBlock(ITEM_INFO* item)
{
#ifdef T1M_FEAT_OG_FIXES
    if (T1MConfig.fix_tihocan_secret_sound) {
        SoundEffect(33, NULL, 0);
        FlipEffect = -1;
    } else {
#else
    {
#endif
        if (FlipTimer == 0) {
            SoundEffect(173, NULL, 0);
        }

        FlipTimer++;
        if (FlipTimer == 55) {
            SoundEffect(33, NULL, 0);
            FlipEffect = -1;
        }
    }
}

void T1MInjectGameEffects()
{
    INJECT(0x0041AD00, FxChainBlock);
}
