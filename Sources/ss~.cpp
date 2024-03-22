#include "pd-partialtrack.hpp"
#include "spt.hpp"

t_class *sptsynth_class;

// ─────────────────────────────────────
typedef struct _spt {
    t_object xObj;
    t_sample sample;
    bool running;
    bool thereisPrevious;
    unsigned int FrameSize;
    unsigned int sampleRate;
    t_sample *real_out;
    t_int audioInBlockIndex;
    unsigned int synthDone;

    int blockPosition;
    int whichBlock;
    int frameIndex;
    int perform;
    int bufferFull;

    float *prevFreq;
    float *prevAmp;
    float *prevPhase;
    bool prevPeak;

    t_outlet *out;
    t_sample *outSignal;

} t_spt;

static inline double m2pi(double x) {
    using namespace std; // floor should be in std
#define ROUND(x) (floor(.5 + (x)))
    return x + (TwoPi * ROUND(-x / TwoPi));
}

// ─────────────────────────────────────
static float cubicInterpolation(float y0, float y1, float y2, float y3,
                                float mu) {
    float mu2 = mu * mu;
    float a0 = -0.5 * y0 + 1.5 * y1 - 1.5 * y2 + 0.5 * y3;
    float a1 = y0 - 2.5 * y1 + 2 * y2 - 0.5 * y3;
    float a2 = -0.5 * y0 + 0.5 * y2;
    float a3 = y1;
    return (a0 * mu * mu2 + a1 * mu2 + a2 * mu + a3);
}

// ─────────────────────────────────────
static void oscillate(float *out, float FrameSize, Peak *peak) {
    float prevFreq = peak->previous->Freq * TwoPi / sys_getsr();
    float prevAmp = peak->previous->Mag;
    float prevPhase = peak->previous->Phase;

    float freq = peak->Freq * TwoPi / sys_getsr();
    float amp = peak->Mag;
    float phase = peak->Phase;

    if (prevAmp == 0) {
        prevFreq = freq;
        prevPhase = peak->Phase - (freq * FrameSize);
        while (prevPhase >= M_PI) {
            prevPhase -= TwoPi;
        }
        while (prevPhase < -M_PI) {
            prevPhase += TwoPi;
        }
    }

    // Calculando os coeficientes de interpolação
    float fTmp1 = freq - prevFreq;
    float fTmp2 =
        ((prevPhase + prevFreq * FrameSize - phase) + fTmp1 * FrameSize / 2.0) /
        TwoPi;
    int iM = (int)(fTmp2 + .5);
    fTmp2 = phase - prevPhase - prevFreq * FrameSize + TwoPi * iM;
    float fAlpha = (3.0 / (FrameSize * FrameSize)) * fTmp2 - fTmp1 / FrameSize;
    float fBeta = (-2.0 / (FrameSize * FrameSize * FrameSize)) * fTmp2 +
                  fTmp1 / (FrameSize * FrameSize);

    float fMagIncr = (amp - prevAmp) / FrameSize;
    float fInstMag, fInstPhase;
    fInstMag = prevAmp;

    for (int i = 0; i < FrameSize; i++) {
        fInstMag += fMagIncr;
        fInstPhase =
            prevPhase + prevFreq * i + fAlpha * i * i + fBeta * i * i * i;
        out[i] += fInstMag * sinf(fInstPhase + TwoPi);
    }
}

// ==============================================
static void SinusoidalSynth(t_spt *x, simpl::Peak *p, int index) {

    if (x->prevFreq[index] == 0 && p->frequency == 0) {
        return;
    }

    int FrameSize = x->FrameSize;

    float prevFreq = x->prevFreq[index] * TwoPi / sys_getsr();
    float prevAmp = x->prevAmp[index];
    float prevPhase = x->prevPhase[index];

    float freq = p->frequency * TwoPi / sys_getsr();
    float amp = p->amplitude;
    float phase = p->phase;

    if (prevAmp == 0) {
        prevFreq = freq;
        prevPhase = phase - (freq * FrameSize);
        while (prevPhase >= M_PI) {
            prevPhase -= TwoPi;
        }
        while (prevPhase < -M_PI) {
            prevPhase += TwoPi;
        }
    }

    // Calculando os coeficientes de interpolação
    float fTmp1 = freq - prevFreq;
    float fTmp2 =
        ((prevPhase + prevFreq * FrameSize - phase) + fTmp1 * FrameSize / 2.0) /
        TwoPi;
    int iM = (int)(fTmp2 + .5);
    fTmp2 = phase - prevPhase - prevFreq * FrameSize + TwoPi * iM;
    float fAlpha = (3.0 / (FrameSize * FrameSize)) * fTmp2 - fTmp1 / FrameSize;
    float fBeta = (-2.0 / (FrameSize * FrameSize * FrameSize)) * fTmp2 +
                  fTmp1 / (FrameSize * FrameSize);

    float fMagIncr = (amp - prevAmp) / FrameSize;
    float fInstMag, fInstPhase;
    fInstMag = prevAmp;

    for (int i = 0; i < FrameSize; i++) {
        fInstMag += fMagIncr;
        fInstPhase =
            prevPhase + prevFreq * i + fAlpha * i * i + fBeta * i * i * i;
        x->outSignal[i] += fInstMag * sinf(fInstPhase + TwoPi);
    }

    // Calculating interpolation coefficients for frequency
    // float fTmp1 = freq - prevFreq;
    // float fTmp2 =
    //     ((prevPhase + prevFreq * FrameSize - phase) + fTmp1 * FrameSize
    //     / 2.0) / TwoPi;
    // int iM = (int)(fTmp2 + .5);
    // fTmp2 = phase - prevPhase - prevFreq * FrameSize + TwoPi * iM;
    // float a = prevFreq;
    // float d = freq;
    // float blockSizePlusOne = FrameSize + 1;
    // float b = (d - a) / blockSizePlusOne + (d - a) / blockSizePlusOne;
    // float c = ((d - a) / blockSizePlusOne) - (d - a) / blockSizePlusOne;
    //
    // float fMagIncr = (amp - prevAmp) / FrameSize;
    // float fInstMag, fInstPhase;
    // fInstMag = prevAmp;
    //
    // for (int i = 0; i < FrameSize; i++) {
    //     fInstMag += fMagIncr;
    //     float iSquared = i * i;
    //     float iCubed = iSquared * i;
    //     fInstPhase =
    //         prevPhase + prevFreq * i + a * iCubed + b * iSquared + c * i;
    //     x->outSignal[i] += fInstMag * sinf(fInstPhase + TwoPi);
    // }
}

// ==============================================
static void InterpolateSynthesis(t_spt *x, t_gpointer *p) {
    x->running = true;
    AnalysisData *Anal = (AnalysisData *)p;

    int size = Anal->Frame.num_partials();

    for (int i = 0; i < size; i++) {
        simpl::Peak *peak = Anal->Frame.partial(i);
        if (peak == nullptr || peak->frequency == 0) {
            continue;
        }

        if (x->prevPeak) {
            SinusoidalSynth(x, peak, i);
        }

        x->prevFreq[i] = peak->frequency;
        x->prevAmp[i] = peak->amplitude;
        x->prevPhase[i] = peak->phase;
    }

    Anal->Frame.clear_peaks();
    Anal->Frame.clear_partials();
    Anal->Frame.clear_synth();
    x->running = false;
    if (x->prevPeak) {
        x->synthDone = 1;
    }
    x->prevPeak = true;
}

// ─────────────────────────────────────
static void do_Synthesis(t_spt *x, t_gpointer *p) {
    x->running = true;
    PartialTracking *pt = (PartialTracking *)p;

    for (int i = 0; i < pt->Partials->size(); i++) {
        Peak *peak = pt->Partials->at(i);
        if (peak == nullptr) {
            continue;
        }
        if (peak->previous == nullptr) {
            continue;
        }
        oscillate(x->outSignal, pt->FrameSize, peak);
    }
    x->running = false;
    x->synthDone = 1;
}

// ─────────────────────────────────────
static t_int *AudioPerform(t_int *w) {
    t_spt *x = (t_spt *)(w[1]);
    t_sample *out = (t_sample *)(w[2]);
    int n = (int)(w[3]);

    int i;

    if (!x->synthDone) {
        for (i = 0; i < n; i++) {
            out[i] = 0;
        }
        return (w + 4);
    }

    for (i = 0; i < n; i++) {
        out[i] = x->outSignal[x->blockPosition + i];
        x->outSignal[x->blockPosition + i] = 0;
    }

    x->blockPosition = x->blockPosition + n;
    if (x->blockPosition >= x->FrameSize) {
        x->blockPosition = 0;
    }
    return (w + 4);
}

// ─────────────────────────────────────
static void AddDsp(t_spt *x, t_signal **sp) {
    dsp_add(AudioPerform, 3, x, sp[0]->s_vec, SPT_SIGTOTAL(sp[0]));
}

// ─────────────────────────────────────
static void *SPT_New(t_symbol *s, t_float f) {
    t_spt *x = (t_spt *)pd_new(sptsynth_class);
    x->FrameSize = 512;
    x->sampleRate = sys_getsr();
    x->outSignal = new t_sample[x->FrameSize];

    x->prevFreq = new float[x->FrameSize];
    x->prevAmp = new float[x->FrameSize];
    x->prevPhase = new float[x->FrameSize];

    x->prevPeak = false;

    x->out = outlet_new(&x->xObj, &s_signal);

    return x;
}

// ─────────────────────────────────────
void s_ss_tilde(void) {
    sptsynth_class = class_new(gensym("s-ss~"), (t_newmethod)SPT_New, NULL,
                               sizeof(t_spt), CLASS_DEFAULT, A_NULL, 0);

    class_addmethod(sptsynth_class, (t_method)AddDsp, gensym("dsp"), A_CANT, 0);

    class_addmethod(sptsynth_class, (t_method)do_Synthesis, gensym("sptObj"),
                    A_POINTER, 0);
    class_addmethod(sptsynth_class, (t_method)InterpolateSynthesis,
                    gensym("simplObj"), A_POINTER, 0);
    // outlet_anything(x->sigOut, gensym("simplObj"), 1, args);
}
