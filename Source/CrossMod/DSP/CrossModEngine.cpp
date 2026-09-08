#include "CrossModEngine.h"

void CrossModEngine::voiceSetEnvelopes(CrossModVoice& voice,
                                       float ampA, float ampD, float ampS, float ampR,
                                       float filtA, float filtD, float filtS, float filtR,
                                       float modA, float modD, float modS, float modR)
{
    voice.setEnvelopes(ampA, ampD, ampS, ampR, filtA, filtD, filtS, filtR, modA, modD, modS, modR);
}
