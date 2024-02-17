#include "pd-simpl.hpp"
#include <m_pd.h>

static t_class *S_create;

// ─────────────────────────────────────
typedef struct _Screate {
    t_object xObj;

    std::string createWhat;
    simpl::Frame *Frame;
    simpl::Frames *Frames;

    t_int maxP;
    t_int pAdded;

    t_outlet *out;

} Screate;

// ─────────────────────────────────────
static void CreatePeak(Screate *x, t_symbol *s, int argc, t_atom *argv) {
    if (x->Frame == NULL) {
        x->Frame = new simpl::Frame(); // TODO: Need to delete
        x->Frame->num_partials(x->maxP);
    }

    if (x->pAdded > x->maxP) {
        post("[s-create] The maximum number of partials is %d", x->maxP);
        return;
    }

    double frequency = atom_getfloat(argv);
    double amplitude = atom_getfloat(argv + 1);
    double phase = atom_getfloat(argv + 2);
    double bandwidth = atom_getfloat(argv + 3);
    x->Frame->add_peak(amplitude, frequency, phase, bandwidth);
    x->pAdded += 1;
}

// ──────────────────────────────────────────
static void AddFrame(Screate *x, t_gpointer *p) {
    if (x->Frames == NULL) {
        x->Frames = new simpl::Frames();
    }

    simpl::Frame *Frame = (simpl::Frame *)p;
    x->Frames->push_back(Frame);
}
// ─────────────────────────────────────
static void Output(Screate *x, t_symbol *s, int argc, t_atom *argv) {
    t_atom args[1];

    if (x->createWhat == "Frame") {
        SETPOINTER(&args[0], (t_gpointer *)&x->Frame);
        outlet_anything(x->out, gensym("Frame"), 1, args);
        x->Frame = new simpl::Frame(); // TODO: Need to delete
        x->Frame->num_partials(x->maxP);
        x->pAdded = 0;
    }

    if (x->createWhat == "Frames") {
        SETPOINTER(&args[0], (t_gpointer *)x->Frames);
        outlet_anything(x->out, gensym("Frames"), 1, args);
        x->Frames = NULL;
    }
}

// ─────────────────────────────────────
static void *New_Screate(t_symbol *s, int argc, t_atom *argv) {
    std::string createWhat = atom_getsymbol(argv)->s_name;
    Screate *x = (Screate *)pd_new(S_create);
    x->createWhat = createWhat;
    x->out = outlet_new(&x->xObj, &s_anything);
    x->maxP = 128;

    return x;
}

// ─────────────────────────────────────
void s_create_setup(void) {
    S_create = class_new(gensym("s-create"), (t_newmethod)New_Screate, NULL,
                         sizeof(Screate), CLASS_DEFAULT, A_GIMME, 0);

    class_addlist(S_create, CreatePeak);
    class_addbang(S_create, Output);
    class_addmethod(S_create, (t_method)AddFrame, gensym("Frame"), A_POINTER,
                    0);

    // TODO: set method for max peaks
}
