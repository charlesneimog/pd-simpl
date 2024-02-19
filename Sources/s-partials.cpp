#include "pd-simpl.hpp"

static t_class *PartialTracking;

// ==============================================
typedef struct _PartialTracking { // It seems that all the objects are some kind
    t_object xObj;
    t_sample xSample; // audio to fe used in CLASSMAINSIGIN
    std::vector<simpl::s_sample> audioIn;
    t_int hopSize;
    t_int maxPeaks;
    t_int running;
    simpl::PartialTracking *PT;
    simpl::Frames Frames;
    t_outlet *sigOut;

} t_PartialTracking;

// ==============================================
static void DetachedPartials(t_PartialTracking *x) {
    x->running = 1;
    simpl::Frames Frames;
    Frames = x->PT->find_partials(x->Frames);
    t_atom args[1];
    SETPOINTER(&args[0], (t_gpointer *)&Frames);
    outlet_anything(x->sigOut, gensym("Frames"), 1, args);
}

// ==============================================
static void ExecutePartialTracking(t_PartialTracking *x, t_gpointer *p) {
    if (x->PT == NULL) {
        pd_error(NULL, "[s-partials] You need to choose a Partial Tracking "
                       "method");
        return;
    }
    simpl::Frames Frames = *(simpl::Frames *)p;
    x->Frames = Frames;

    std::thread partialDetector(DetachedPartials, x);
    partialDetector.detach();
}

// ==============================================
static void SetPartialTrackingMethod(t_PartialTracking *x, t_symbol *s) {
    std::string method = s->s_name;
    if (method == "loris") {
        x->PT = new simpl::LorisPartialTracking();
    } else if (method == "sms") {
        x->PT = new simpl::SMSPartialTracking();
        ((simpl::SMSPartialTracking *)x->PT)->realtime(true);
    } else if (method == "mq") {
        x->PT = new simpl::MQPartialTracking();
    } else if (method == "snd") {
        x->PT = new simpl::SndObjPartialTracking();
    } else {
        pd_error(NULL, "[peaks~] Unknown method");
        return;
    }
    x->PT->sampling_rate(sys_getsr());
    x->PT->max_partials(x->maxPeaks);
}

// ==============================================
static void *NewPartialTracking(void) {

    t_PartialTracking *x = (t_PartialTracking *)pd_new(PartialTracking);
    x->sigOut = outlet_new(&x->xObj, &s_anything);
    x->hopSize = -1;
    x->maxPeaks = 100;

    return x;
}

// ==============================================
void s_partials_setup(void) {
    PartialTracking =
        class_new(gensym("s-partials"), (t_newmethod)NewPartialTracking, NULL,
                  sizeof(t_PartialTracking), CLASS_DEFAULT, A_GIMME, 0);

    class_addmethod(PartialTracking, (t_method)SetPartialTrackingMethod,
                    gensym("set"), A_SYMBOL, 0);

    class_addmethod(PartialTracking, (t_method)ExecutePartialTracking,
                    gensym("Frames"), A_POINTER, 0);
}
