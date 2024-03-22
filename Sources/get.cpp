#include "pd-partialtrack.hpp"

static t_class *S_get;

// ─────────────────────────────────────
typedef struct _Sget {
    t_object xObj;

    t_int PeaksOut;
    t_outlet *simplOut;

    t_outlet *f;
    t_outlet *a;
    t_outlet *p;
    t_outlet *b;

} Sget;

// ─────────────────────────────────────
static void GetValues(Sget *x, t_gpointer *p) {
    t_pdsimpl *Simpl = (t_pdsimpl *)p;
    for (int i = 0; i < Simpl->Frame->num_partials(); i++) {
        simpl::Peak *Peak = Simpl->Frame->partial(i);
        if (Peak != nullptr) {
            t_atom args[4];
            if (x->PeaksOut) {
                Simpl->peakIndex = i;
                outlet_float(x->b, Peak->bandwidth);
                outlet_float(x->p, Peak->phase);
                outlet_float(x->a, Peak->amplitude);
                outlet_float(x->f, Peak->frequency);
                t_atom args[1];
                SETPOINTER(&args[0], (t_gpointer *)Simpl);
                outlet_anything(x->simplOut, gensym("simplObj"), 1, args);
            }
        }
    }
    return;
}

// ─────────────────────────────────────
static void *New_Sget(t_symbol *s, int argc, t_atom *argv) {
    Sget *x = (Sget *)pd_new(S_get);
    for (int i = 0; i < argc; i++) {
        if (argv[i].a_type == A_SYMBOL) {
            std::string what = atom_getsymbol(&argv[i])->s_name;
            if (what == "Peaks") {
                x->PeaksOut = 1;
                x->simplOut = outlet_new(&x->xObj, &s_anything);
                x->f = outlet_new(&x->xObj, &s_float);
                x->a = outlet_new(&x->xObj, &s_float);
                x->p = outlet_new(&x->xObj, &s_float);
                x->b = outlet_new(&x->xObj, &s_float);
            } else {
                pd_error(nullptr, "[s-get] %s is an invalid argument",
                         what.c_str());
            }
        }
    }
    return x;
}

// ─────────────────────────────────────
void s_get_setup(void) {
    S_get = class_new(gensym("s-get"), (t_newmethod)New_Sget, NULL,
                      sizeof(Sget), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(S_get, (t_method)GetValues, gensym("simplObj"), A_POINTER,
                    0);
}
