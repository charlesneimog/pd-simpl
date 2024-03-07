#include "pd-simpl.hpp"

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
    t_float tLowNote[MAX_SILENCED_PARTIALS];
    t_float tHighNote[MAX_SILENCED_PARTIALS];
    t_float tCents[MAX_SILENCED_PARTIALS];
    unsigned int tIndex;

    t_outlet *out;

} t_Trans;

// ==============================================
static double midicent2freq(double originalFrequency, double cents) {
    return originalFrequency * std::pow(2.0, cents / 1200.0);
}

// ==============================================
static void Transpose(t_Trans *x, AnalysisData *Anal, int i) {
    simpl::Peak *Peak = Anal->Frame.partial(i);
    if (Peak != nullptr) {
        if (Peak->frequency >= x->sLowNote[i] &&
            Peak->frequency <= x->sHighNote[i]) {
            float cent = x->tCents[i];
            float newFreq = midicent2freq(Peak->frequency, cent);
            post("old freq: %f, new freq: %f", Peak->frequency, newFreq);
            Peak->frequency = newFreq;
        }
    }
}

// ==============================================
static void SilencePartials(t_Trans *x, AnalysisData *Anal, int i) {
    simpl::Peak *Peak = Anal->Frame.partial(i);
    if (Peak != nullptr) {
        if (Peak->frequency >= x->sLowNote[i] &&
            Peak->frequency <= x->sHighNote[i]) {
            Peak->amplitude = 0;
        }
    }
}

// ==============================================
static void Process(t_Trans *x, t_gpointer *p) {
    AnalysisData *Anal = (AnalysisData *)p;
    for (int i = 0; i < Anal->Frame.num_partials(); i++) {
        if (x->sIndex != 0)
            SilencePartials(x, Anal, i);
        if (x->tIndex != 0)
            Transpose(x, Anal, i);
    }

    // output
    t_atom args[1];
    SETPOINTER(&args[0], (t_gpointer *)Anal);
    outlet_anything(x->out, gensym("simplObj"), 1, args);
}

// ==============================================
static void SetTransposedPartial(t_Trans *x, t_float low, t_float high,
                                 t_float cents) {
    if (x->tIndex < MAX_SILENCED_PARTIALS) {
        x->tLowNote[x->tIndex] = low;
        x->tHighNote[x->tIndex] = high;
        x->tCents[x->tIndex] = cents;
        x->tIndex++;
    } else {
        pd_error(NULL, "[peaks~] Maximum number of silenced partials reached");
    }
}

// ==============================================
static void SetSilencePartial(t_Trans *x, t_float low, t_float high) {
    if (x->sIndex < MAX_SILENCED_PARTIALS) {
        x->sLowNote[x->sIndex] = low;
        x->sHighNote[x->sIndex] = high;
        x->sIndex++;
    } else {
        pd_error(NULL, "[peaks~] Maximum number of silenced partials reached");
    }
}

// ==============================================
static void *NewTransform(t_symbol *synth) {
    t_Trans *x = (t_Trans *)pd_new(Transformations);
    x->out = outlet_new(&x->xObj, &s_anything);
    DEBUG_PRINT("[synth~] New Synth");
    return x;
}

// ==============================================
void s_trans_setup(void) {
    Transformations =
        class_new(gensym("s-trans"), (t_newmethod)NewTransform, NULL,
                  sizeof(t_Trans), CLASS_DEFAULT, A_GIMME, 0);

    class_addmethod(Transformations, (t_method)SetSilencePartial,
                    gensym("silence"), A_FLOAT, A_FLOAT, 0);

    class_addmethod(Transformations, (t_method)SetTransposedPartial,
                    gensym("trans"), A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(Transformations, (t_method)Process, gensym("simplObj"),
                    A_POINTER, 0);
}
