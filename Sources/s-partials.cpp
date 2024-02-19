#include "pd-simpl.hpp"

static t_class *PartialTracking;

// ==============================================
typedef struct _PartialTracking { // It seems that all the objects are some kind
    t_object xObj;
    t_sample xSample; // audio to fe used in CLASSMAINSIGIN

    // multitreading
    std::vector<simpl::s_sample> audioIn;

    // Peak Detection Parameters
    t_int FrameSize;
    t_int HopSize;
    t_int maxPeaks;
    t_int running;

    simpl::PartialTracking *PartialTracking;

    // Frames
    simpl::Frames Frames;

    // Outlet/Inlet
    t_outlet *sigOut;

    // Partial Tracking
    simpl::Peaks *pPeaks; // previous List Peak

} t_PartialTracking;

// ==============================================
static void DetachedPartials(t_PartialTracking *x) {
    x->running = 1;
    simpl::Frames Frames;
    Frames = x->PartialTracking->find_partials(x->Frames);
    post("Partials Peaks n is %d", Frames[0]->num_partials());
    // x->Frames = &Frames;
    // x->running = 0;
}
// ==============================================
static void ExecutePartialTracking(t_PartialTracking *x, t_gpointer *p) {
    if (x->PartialTracking == NULL) {
        pd_error(NULL,
                 "[partialtracking] You need to choose a Partial Tracking "
                 "method");
        return;
    }
    simpl::Frames Frames = *(simpl::Frames *)p;
    x->Frames = Frames;
    // p is a simpl::Frames, get it again

    std::thread partialDetector(DetachedPartials, x);
    partialDetector.detach();
    //
    // t_atom args[1];
    // SETPOINTER(&args[0], (t_gpointer *)&x->Frames);
    // outlet_anything(x->sigOut, gensym("Frames"), 1, args);
}

// ==============================================
static void SetPartialTrackingMethod(t_PartialTracking *x, t_symbol *s) {
    std::string method = s->s_name;
    if (method == "loris") {
        x->PartialTracking = new simpl::LorisPartialTracking();
    } else if (method == "sms") {
        x->PartialTracking = new simpl::SMSPartialTracking();
        ((simpl::SMSPartialTracking *)x->PartialTracking)->realtime(true);
    } else if (method == "mq") {
        x->PartialTracking = new simpl::MQPartialTracking();
    } else if (method == "snd") {
        x->PartialTracking = new simpl::SndObjPartialTracking();
    } else {
        pd_error(NULL, "[peaks~] Unknown method");
        return;
    }
    x->PartialTracking->sampling_rate(sys_getsr());
    x->PartialTracking->max_partials(x->maxPeaks);
}

// ==============================================
static void *NewPartialTracking(void) {

    t_PartialTracking *x = (t_PartialTracking *)pd_new(PartialTracking);
    x->sigOut = outlet_new(&x->xObj, &s_anything);
    x->HopSize = -1;
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
