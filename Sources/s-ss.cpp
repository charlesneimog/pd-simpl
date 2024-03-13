#include "s-spt.hpp"

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

    t_outlet *out;
    t_sample *outSignal;

} t_spt;

static inline double m2pi(double x) {
    using namespace std; // floor should be in std
#define ROUND(x) (floor(.5 + (x)))
    return x + (TwoPi * ROUND(-x / TwoPi));
}

// ─────────────────────────────────────

static void oscillate(float *out, float FrameSize, Peak *peak) {
    float prevFreq = peak->previous->Freq * 2 * M_PI / sys_getsr();
    float prevAmp = peak->previous->Mag;
    float prevPhase = peak->previous->Phase;

    float freq = peak->Freq * TwoPi / sys_getsr();
    float amp = peak->Mag;
    float phase = peak->Phase;

    if (prevAmp == 0) {
        prevFreq = freq;
        prevPhase = peak->Phase - (freq * FrameSize);
        while (prevPhase >= M_PI) {
            prevPhase -= (2.0 * M_PI);
        }
        while (prevPhase < -M_PI) {
            prevPhase += (2.0 * M_PI);
        }
    }

    float ampStep = (amp - prevAmp) / FrameSize;
    float freqStep = (freq - prevFreq) / FrameSize;

    // Cubic interpolation for frequency
    float a, b, c, d;
    a = prevFreq;
    d = freq;

    b = (d - a) / (FrameSize + 1) + (d - a) / (FrameSize + 1);
    c = (d - a) / (FrameSize + 1) - (d - a) / (FrameSize + 1);

    // make cubic interpolation of hte frequency
    for (int i = 0; i < FrameSize; i++) {
        float t = (float)i / FrameSize;
        float t2 = t * t;
        float t3 = t2 * t;
        float f = a + b * t + c * t2 + d * t3;
        prevPhase += f;
        while (prevPhase >= M_PI) {
            prevPhase -= (2.0 * M_PI);
        }
        while (prevPhase < -M_PI) {
            prevPhase += (2.0 * M_PI);
        }
        out[i] += (prevAmp + ampStep * i) * sin(prevPhase);
    }
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
        oscillate(x->outSignal, pt->FrameSize, peak);
        x->running = false;
    }
    x->synthDone = 1;
}

// ─────────────────────────────────────
static t_int *AudioPerform(t_int *w) {
    t_spt *x = (t_spt *)(w[1]);
    t_sample *out = (t_sample *)(w[2]);
    int n = (int)(w[3]);

    int i;
    for (i = 0; i < n; i++) {
        out[i] = 0;
    }

    if (!x->synthDone) {
        for (i = 0; i < n; i++) {
            out[i] = 0;
        }
        return (w + 4);
    }

    // synth
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
    x->FrameSize = 2048;
    x->real_out = new t_sample[x->FrameSize];
    x->thereisPrevious = false;
    x->sampleRate = sys_getsr();
    x->outSignal = new t_sample[x->FrameSize];
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
}
