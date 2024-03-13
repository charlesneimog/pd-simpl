#include "s-spt.hpp"

t_class *sptpartials_class;

// ─────────────────────────────────────
typedef struct _spt {
    t_object xObj;
    t_sample sample;
    bool running;
    bool thereisPrevious;
    unsigned int FrameSize;
    unsigned int sampleRate;

    Peaks *curPeaks;
    Peaks *prevPeaks;
    t_sample *real_in;
    t_int audioInBlockIndex;

    // params
    float minDistance;
    float minAmp;

    t_outlet *out;

} t_spt;

// ─────────────────────────────────────
static void do_PT(t_spt *x) {
    x->running = true;
    if (x->thereisPrevious) {
        x->prevPeaks = x->curPeaks;
    }

    t_sample *imag_in = new t_sample[x->FrameSize];
    mayer_fft(x->FrameSize, x->real_in, imag_in);

    float *mag = new float[x->FrameSize / 2]; // TODO: Clear
    float *phase = new float[x->FrameSize / 2];

    for (int i = 0; i < x->FrameSize / 2; i++) {
        x->real_in[i] /= x->FrameSize;
        imag_in[i] /= x->FrameSize;
        mag[i] = sqrt(x->real_in[i] * x->real_in[i] + imag_in[i] * imag_in[i]);
        mag[i] = 20 * log10(mag[i]);
        phase[i] = atan2(imag_in[i], x->real_in[i]);
    }

    delete x->curPeaks;
    x->curPeaks = new Peaks(x->FrameSize / 2);
    for (int i = 1; i < x->FrameSize / 2 - 1; i++) {
        float a = mag[i - 1];
        float b = mag[i];
        float c = mag[i + 1];
        if (a < b && b > c && b > x->minAmp) {
            float p = 0.5 * (a - c) / (a - 2 * b + c);
            float new_a = x->real_in[i] -
                          0.25 * (x->real_in[i - 1] - x->real_in[i + 1]) * p;
            float new_b =
                imag_in[i] - 0.25 * (imag_in[i - 1] - imag_in[i + 1]) * p;
            Peak *peak = new Peak();
            peak->index = i;
            peak->Freq = (float)x->sampleRate / x->FrameSize * i + p;
            peak->Mag = sqrt(new_a * new_a + new_b * new_b);
            peak->Phase = atan2(new_b, new_a);
            x->curPeaks->push_back(peak);
        }
    }

    if (!x->thereisPrevious) {
        x->thereisPrevious = true;
        return;
    } else {
        // maybe clear
    }

    PartialTracking *pt = new PartialTracking();
    pt->Partials = new Peaks(x->FrameSize / 2);
    pt->partialCount = 0;
    pt->FrameSize = x->FrameSize;

    int last = 0;
    for (int i = 0; i < x->prevPeaks->size(); i++) {
        Peak *prevPeak = x->prevPeaks->at(i);
        if (prevPeak == nullptr) {
            continue;
        }
        for (int j = 0; j < x->curPeaks->size(); j++) {
            Peak *curPeak = x->curPeaks->at(j);
            if (curPeak == nullptr) {
                continue;
            }
            float oldDistance;
            float distance = abs(prevPeak->Freq - curPeak->Freq);
            if (distance < x->minDistance) {
                if (curPeak->previous != nullptr) {
                    oldDistance =
                        abs(curPeak->previous->Freq - curPeak->previous->Freq);
                } else {
                    oldDistance = x->minDistance;
                }
                if (distance < oldDistance) {
                    last = j;
                    curPeak->previous = prevPeak;
                    pt->partialCount++;
                }
            }
        }
    }

    pt->Partials = x->curPeaks;

    t_atom args[1];
    SETPOINTER(&args[0], (t_gpointer *)pt);
    outlet_anything(x->out, gensym("sptObj"), 1, args);

    x->running = false;
}

// ─────────────────────────────────────
static t_int *AudioPerform(t_int *w) {
    t_spt *x = (t_spt *)(w[1]);
    int n = (int)(w[3]);

    t_sample *real = (t_sample *)(w[2]);

    for (int i = 0; i < n; i++) {
        x->real_in[x->audioInBlockIndex] =
            real[i]; // TODO: apply windowing funciton
        x->audioInBlockIndex++;
        if (x->audioInBlockIndex == x->FrameSize) {
            x->audioInBlockIndex = 0;
            do_PT(x);
        }
    }

    return (w + 4);
}

// ─────────────────────────────────────
static void AddDsp(t_spt *x, t_signal **sp) {
    dsp_add(AudioPerform, 3, x, sp[0]->s_vec, SPT_SIGTOTAL(sp[0]));
}

// ─────────────────────────────────────
static void *SPT_New(t_symbol *s, t_float f) {
    t_spt *x = (t_spt *)pd_new(sptpartials_class);
    x->FrameSize = 2048;
    x->real_in = new t_sample[x->FrameSize];
    x->curPeaks = new Peaks(x->FrameSize / 2);
    x->prevPeaks = new Peaks(x->FrameSize / 2);
    x->minDistance = 10;
    x->minAmp = -80;

    x->thereisPrevious = false;
    x->sampleRate = sys_getsr();
    x->out = outlet_new(&x->xObj, &s_anything);
    return x;
}

// ─────────────────────────────────────
void s_spt_tilde_setup(void) {

    sptpartials_class = class_new(gensym("s-spt~"), (t_newmethod)SPT_New, NULL,
                                  sizeof(t_spt), CLASS_DEFAULT, A_NULL, 0);

    CLASS_MAINSIGNALIN(sptpartials_class, t_spt, sample);
    class_addmethod(sptpartials_class, (t_method)AddDsp, gensym("dsp"), A_CANT,
                    0);
}
