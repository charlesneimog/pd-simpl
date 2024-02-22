#include "pd-simpl.hpp"

static t_class *S_create;

// ─────────────────────────────────────
typedef struct _Screate {
    t_object xObj;

    t_pdsimpl *Simpl;
    float freq;
    float amp;
    float phase;
    float bandwidth;

    t_inlet *f;
    t_inlet *a;
    t_inlet *p;
    t_inlet *b;

    t_outlet *out;

} Screate;

// ─────────────────────────────────────
static void newF(Screate *x, t_floatarg f) { x->freq = f; }
static void newA(Screate *x, t_floatarg f) { x->amp = f; }
static void newP(Screate *x, t_floatarg f) { x->phase = f; }
static void newB(Screate *x, t_floatarg f) { x->bandwidth = f; }

// ─────────────────────────────────────
static void Modify(Screate *x, t_gpointer *p) {
    t_pdsimpl *Simpl = (t_pdsimpl *)p;
    int sFrames = Simpl->Frames->size() - 1;
    int fIndex = Simpl->frameIndex;
    int pLength = Simpl->Frames->at(fIndex)->num_partials();

    if (Simpl->frameIndex >= 0 && Simpl->frameIndex < Simpl->Frames->size()) {
        simpl::Frame *Frame = Simpl->Frames->at(Simpl->frameIndex);
        simpl::Peak *Peak = Frame->partial(Simpl->peakIndex);
        Peak->frequency = x->freq;
        Peak->amplitude = x->amp;
        Peak->phase = x->phase;
        Peak->bandwidth = x->bandwidth;
        Simpl->PT->update_partials(Frame);
        if (sFrames == fIndex && (pLength - 1) == Simpl->peakIndex) {
            t_atom args[1];
            SETPOINTER(&args[0], (t_gpointer *)Simpl);
            outlet_anything(x->out, gensym("simplObj"), 1, args);
        }
    }

    // NOTE: So output caso o PeakIndex for igual ao numeros de parciais
}

// ─────────────────────────────────────
static void *New_Screate(t_symbol *s, int argc, t_atom *argv) {
    Screate *x = (Screate *)pd_new(S_create);
    x->f = inlet_new(&x->xObj, &x->xObj.ob_pd, &s_float, gensym("_freq"));
    x->a = inlet_new(&x->xObj, &x->xObj.ob_pd, &s_float, gensym("_amp"));
    x->p = inlet_new(&x->xObj, &x->xObj.ob_pd, &s_float, gensym("_phase"));
    x->b = inlet_new(&x->xObj, &x->xObj.ob_pd, &s_float, gensym("_bandwidth"));
    x->out = outlet_new(&x->xObj, &s_anything);

    return x;
}

// ─────────────────────────────────────
void s_create_setup(void) {
    S_create = class_new(gensym("s-create"), (t_newmethod)New_Screate, NULL,
                         sizeof(Screate), CLASS_DEFAULT, A_GIMME, 0);

    class_addmethod(S_create, (t_method)newF, gensym("_freq"), A_FLOAT, 0);
    class_addmethod(S_create, (t_method)newA, gensym("_amp"), A_FLOAT, 0);
    class_addmethod(S_create, (t_method)newP, gensym("_phase"), A_FLOAT, 0);
    class_addmethod(S_create, (t_method)newB, gensym("_bandwidth"), A_FLOAT, 0);
    class_addmethod(S_create, (t_method)Modify, gensym("simplObj"), A_POINTER,
                    0);

    // TODO: set method for max peaks
}
