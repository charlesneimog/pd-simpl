#include "partialtrack.hpp"

static t_class *Transformations;

#define MAX_SILENCED_PARTIALS 127

// ==============================================
typedef struct _Trans {
    t_object xObj;

    // silence partial
    t_float sLowNote[MAX_SILENCED_PARTIALS];
    t_float sHighNote[MAX_SILENCED_PARTIALS];
    unsigned int sIndex;

    // Transpose Partials
    t_float tCenterFreq[MAX_SILENCED_PARTIALS];
    t_float tVariation[MAX_SILENCED_PARTIALS]; // cents
    t_float tCents[MAX_SILENCED_PARTIALS];
    unsigned int tIndex;

    // Transpose Partials
    t_float aCenterFreq[MAX_SILENCED_PARTIALS];
    t_float aVariation[MAX_SILENCED_PARTIALS]; // cents
    t_float aAmpsFactor[MAX_SILENCED_PARTIALS];
    unsigned int aIndex;

    // Expand Partials
    t_float eFactor;
    t_float eFundFreq;
    t_float ePrevFreq;

    t_outlet *out;

} t_Trans;

// ==============================================
static double f2m(double f) { return 12 * log(f / 220) / log(2) + 57; }
static double m2f(double m) { return 220 * exp(log(2) * ((m - 57) / 12)); }

// ==============================================
static double transFreq(double originalFrequency, double cents) {
    return originalFrequency * std::pow(2.0, cents / 1200.0);
}

// =======================================
// ╭─────────────────────────────────────╮
// │           Process Helpers           │
// ╰─────────────────────────────────────╯
//  ======================================
static void Transpose(t_Trans *x, simpl::Frame *Frame, int i) {
    simpl::Peak *Peak = Frame->partial(i);
    if (Peak != nullptr) {
        if (Peak->frequency == 0) {
            return;
        }
        for (int i = 0; i < x->tIndex; i++) {
            if (Peak->frequency >=
                    x->tCenterFreq[i] - x->tVariation[i] && // TODO: Fix this
                Peak->frequency <= x->tCenterFreq[i] + x->tVariation[i]) {
                float newFreq = transFreq(Peak->frequency, x->tCents[i]);
                Peak->frequency = newFreq;
            }
        }
    }
}

//  ======================================
static void ChangeAmps(t_Trans *x, simpl::Frame *Frame, int i) {
    simpl::Peak *Peak = Frame->partial(i);
    if (Peak != nullptr) {
        if (Peak->frequency == 0) {
            return;
        }
        for (int i = 0; i < x->aIndex; i++) {
            if (Peak->frequency >= x->aCenterFreq[i] - x->aVariation[i] &&
                Peak->frequency <= x->aCenterFreq[i] + x->aVariation[i]) {
                float newFreq = transFreq(Peak->frequency, x->tCents[i]);
                Peak->amplitude *= x->aAmpsFactor[i];
            }
        }
    }
}
// ==============================================
static void Expand(t_Trans *x, simpl::Frame *Frame, int i) {
    simpl::Peak *Peak = Frame->partial(i);
    if (i == 0) {
        if (Peak != nullptr) {
            x->ePrevFreq = Peak->frequency;
            return;
        }
        return;
    }

    if (Peak != nullptr) {
        if (Peak->frequency == 0 || x->ePrevFreq == 0) {
            return;
        }
        float d = (Peak->frequency - x->ePrevFreq) * x->eFactor;
        float newFreq = m2f(f2m(x->ePrevFreq) + d);
        Peak->frequency = m2f(f2m(x->ePrevFreq) + d);
    }
}

// ==============================================
static void SilencePartials(t_Trans *x, simpl::Frame *Frame, int i) {
    // TODO: Fix
    simpl::Peak *Peak = Frame->partial(i);
    if (Peak != nullptr) {
        if (Peak->frequency >= x->sLowNote[i] &&
            Peak->frequency <= x->sHighNote[i]) {
            Peak->amplitude = 0;
        }
    }
}

// =======================================
// ╭─────────────────────────────────────╮
// │            Set Functions            │
// ╰─────────────────────────────────────╯
//  ======================================
static void SetTransposedPartial(t_Trans *x, t_float centerFreq,
                                 t_float variation, t_float cents) {
    // args: centerFreq, cents variation, cents to transpose
    for (int i = 0; i < x->tIndex; i++) {
        if (centerFreq == x->tCenterFreq[i]) {
            x->tCents[i] = cents;
            return;
        }
    }

    if (x->tIndex < MAX_SILENCED_PARTIALS) {
        x->tCenterFreq[x->tIndex] = centerFreq;
        x->tVariation[x->tIndex] = variation;
        x->tCents[x->tIndex] = cents;
        x->tIndex++;
    } else {
        pd_error(NULL, "[peaks~] Maximum number of silenced partials reached");
    }
}

//  ======================================
static void ResetAll(t_Trans *x) {
    x->sIndex = 0;
    x->aIndex = 0;
    x->tIndex = 0;
}
//  ======================================
static void SetAmplitudeForPartial(t_Trans *x, t_float centerFreq,
                                   t_float variation, t_float amp) {
    // args: centerFreq, cents variation, cents to transpose
    for (int i = 0; i < x->aIndex; i++) {
        if (centerFreq == x->aCenterFreq[i]) {
            x->aAmpsFactor[i] = amp;
            return;
        }
    }

    if (x->aIndex < MAX_SILENCED_PARTIALS) {
        x->aCenterFreq[x->aIndex] = centerFreq;
        x->aVariation[x->aIndex] = variation;
        x->aAmpsFactor[x->aIndex] = amp;
        x->aIndex++;
    } else {
        pd_error(NULL, "[peaks~] Maximum number of silenced partials reached");
    }
}
// ==============================================
static void SetSilencePartial(t_Trans *x, t_float centerFreq,
                              t_float variation) {
    if (x->sIndex < MAX_SILENCED_PARTIALS) {
        x->sLowNote[x->sIndex] = centerFreq - variation;
        x->sHighNote[x->sIndex] = centerFreq + variation;
        x->sIndex++;
    } else {
        pd_error(NULL, "[peaks~] Maximum number of silenced partials reached");
    }
}

// ==============================================
static void SetExpantionPartials(t_Trans *x, t_float factor) {
    x->eFactor = factor;
}

// =======================================
static void ProcessOffline(t_Trans *x, t_symbol *p) {
    AnalysisData *Anal = getAnalisysPtr(p);
    if (Anal == nullptr) {
        pd_error(NULL, "[trans] Pointer not found");
        return;
    }

    simpl::Frames Frames = Anal->Frames;
    for (int i = 0; i < Frames.size(); i++) {
        simpl::Frame *Frame = Frames[i];
        for (int i = 0; i < Frame->num_partials(); i++) {
            if (x->sIndex != 0)
                SilencePartials(x, Frame, i);
            if (x->tIndex != 0)
                Transpose(x, Frame, i);
            if (x->eFactor != 1.0)
                Expand(x, Frame, i);
            if (x->aIndex != 0)
                ChangeAmps(x, Frame, i);
        }
    }

    t_atom args[1];
    SETSYMBOL(&args[0], p);
    outlet_anything(x->out, gensym("PtObjFrames"), 1, args);

    return;
}

// =======================================
// ╭─────────────────────────────────────╮
// │        Partial Manipulations        │
// ╰─────────────────────────────────────╯
// =======================================
static void Process(t_Trans *x, t_symbol *p) {
    AnalysisData *Anal = getAnalisysPtr(p);
    if (Anal == nullptr) {
        pd_error(NULL, "[trans] Pointer not found");
        return;
    }

    if (Anal->offline) {
        pd_error(nullptr, "[trans] Offline mode");
        return;
    }

    for (int i = 0; i < Anal->Frame.num_partials(); i++) {
        if (x->sIndex != 0)
            SilencePartials(x, &Anal->Frame, i);
        if (x->tIndex != 0)
            Transpose(x, &Anal->Frame, i);
        if (x->eFactor != 1.0)
            Expand(x, &Anal->Frame, i);
        if (x->aIndex != 0)
            ChangeAmps(x, &Anal->Frame, i);
    }

    t_atom args[1];
    char ptr[MAXPDSTRING];
    SETSYMBOL(&args[0], p);
    outlet_anything(x->out, gensym("PtObj"), 1, args);
}

// ==============================================
static void *NewTransform(t_symbol *synth) {
    t_Trans *x = (t_Trans *)pd_new(Transformations);
    x->eFactor = 1.0;
    x->out = outlet_new(&x->xObj, &s_anything);
    DEBUG_PRINT("[synth~] New Synth");
    return x;
}

// ==============================================
extern "C" void trans_setup(void) {
    Transformations =
        class_new(gensym("trans"), (t_newmethod)NewTransform, NULL,
                  sizeof(t_Trans), CLASS_DEFAULT, A_GIMME, 0);

    class_addmethod(Transformations, (t_method)SetSilencePartial,
                    gensym("silence"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(Transformations, (t_method)SetExpantionPartials,
                    gensym("expand"), A_FLOAT, 0);
    class_addmethod(Transformations, (t_method)SetTransposedPartial,
                    gensym("trans"), A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(Transformations, (t_method)SetAmplitudeForPartial,
                    gensym("amps"), A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(Transformations, (t_method)ResetAll, gensym("reset"),
                    A_NULL, 0);
    class_addmethod(Transformations, (t_method)Process, gensym("PtObj"),
                    A_SYMBOL, 0);
    class_addmethod(Transformations, (t_method)ProcessOffline,
                    gensym("PtObjFrames"), A_SYMBOL, 0);
}
