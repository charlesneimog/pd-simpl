#include "pd-simpl.hpp"

static t_class *S_get;

// ─────────────────────────────────────
typedef struct _Sget {
    t_object xObj;

    t_outlet *out;

} Sget;

// ─────────────────────────────────────
static void GetFrames(Sget *x, t_gpointer *p) {
    simpl::Frames *framesPtr = (simpl::Frames *)(p);
    simpl::Frames Frames = *framesPtr;
    for (int i = 0; i < Frames.size(); i++) {
        simpl::Frame *Frame = Frames[i];
        t_atom args[1];
        SETPOINTER(&args[0], (t_gpointer *)Frame);
        outlet_anything(x->out, gensym("Frame"), 1, args);
    }
    return;
}

// ─────────────────────────────────────
static void GetPeaks(Sget *x, t_gpointer *p) {
    simpl::Frame *Frame = (simpl::Frame *)(p);
    for (int i = 0; i < Frame->num_partials(); i++) {
        simpl::Peak *Peak = Frame->partial(i);
        if (Peak != nullptr) {
            t_atom args[4];
            if (Peak->frequency != 0 && Peak->amplitude != 0) {
                SETFLOAT(&args[0], Peak->frequency);
                SETFLOAT(&args[1], Peak->amplitude);
                SETFLOAT(&args[2], Peak->phase);
                SETFLOAT(&args[3], Peak->bandwidth);
                outlet_list(x->out, &s_list, 4, args);
            }
        }
    }

    return;
}

// ─────────────────────────────────────
static void *New_Sget(t_symbol *s, int argc, t_atom *argv) {
    Sget *x = (Sget *)pd_new(S_get);
    x->out = outlet_new(&x->xObj, &s_anything);
    return x;
}

// ─────────────────────────────────────
void s_get_setup(void) {
    S_get = class_new(gensym("s-get"), (t_newmethod)New_Sget, NULL,
                      sizeof(Sget), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(S_get, (t_method)GetFrames, gensym("Frames"), A_POINTER, 0);
    class_addmethod(S_get, (t_method)GetPeaks, gensym("Frame"), A_POINTER, 0);
}
