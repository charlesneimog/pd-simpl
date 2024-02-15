#include "pd-simpl.hpp"

static t_class *mqpeak_class;

// ==============================================
typedef struct _mqpeaks { // It seems that all the objects are some kind of
                          // class.
    t_object xObj;
    t_float xSample; // audio to fe used in CLASSMAINSIGIN
    t_int n;

    // multitreading
    std::vector<simpl::s_sample> audioIn;
    t_sample *previousOut;
    t_int warning;

    // Peak Detection Parameters
    t_int FrameSize;
    t_int HopSize;
    simpl::MQPeakDetection *MQ;

    // Frames
    simpl::Frames frames;

    // Outlet/Inlet
    t_outlet *sigOut;

    // Partial Tracking
    simpl::Peaks *pPeaks; // previous List Peak

} t_mqpeaks;

// ==============================================
static t_int *AudioPerform(t_int *w) {
    t_mqpeaks *x = (t_mqpeaks *)(w[1]);
    t_sample *audioIn = (t_sample *)(w[2]);
    int n = (int)(w[3]);
    x->n = n;

    if (n < 128 && !x->warning) {
        pd_error(NULL,
                 "Frame size is very small, use [switch~ 2048] to change it!");
    }

    std::vector<simpl::s_sample> audio(audioIn, audioIn + n);

    x->frames = x->MQ->find_peaks(x->n, audio.data());

    t_atom args[1];
    SETPOINTER(&args[0], (t_gpointer *)&x->frames);
    outlet_anything(x->sigOut, gensym("Peaks"), 1, args);
    return (w + 4);
}
// ==============================================
static void SetHopSize(t_mqpeaks *x, t_float f) {
    x->MQ->hop_size(f);
    x->HopSize = f;
}

// ==============================================
static void SetMaxPeaks(t_mqpeaks *x, t_float f) { x->MQ->max_peaks(f); }

// ==============================================
static void AddDsp(t_mqpeaks *x, t_signal **sp) {
    x->MQ->frame_size(sp[0]->s_n);
    if (x->HopSize == -1) {
        x->MQ->hop_size(sp[0]->s_n / 4);
    }
    dsp_add(AudioPerform, 3, x, sp[0]->s_vec, sp[0]->s_n);
}

// ==============================================
static void *NewMQPeaks(void) {
    int major, minor, micro;
    sys_getversion(&major, &minor, &micro);
    if (major < 0 && minor < 54) {
        pd_error(NULL, "[simpl] You need to use Pd 0.54 or higher");
        return NULL;
    }

    post("");
    post("[mqpeaks~] by John Glover");
    post("[mqpeaks~] ported by Charles K. Neimog");
    post("[mqpeaks~] mqpeaks~ version %d.%d.%d", 0, 0, 1);
    post("");

    t_mqpeaks *x = (t_mqpeaks *)pd_new(mqpeak_class);
    x->MQ = new simpl::MQPeakDetection();
    x->MQ->sampling_rate(sys_getsr());
    x->frames = simpl::Frames(); // TODO: Fix this
    x->sigOut = outlet_new(&x->xObj, &s_anything);
    x->HopSize = -1;

    return x;
}

// ==============================================
void mqpeaks_tilde_setup(void) {
    mqpeak_class = class_new(gensym("mqpeaks~"), (t_newmethod)NewMQPeaks, NULL,
                             sizeof(t_mqpeaks), CLASS_DEFAULT, A_GIMME, 0);
    //
    class_addmethod(mqpeak_class, (t_method)AddDsp, gensym("dsp"), A_CANT, 0);
    CLASS_MAINSIGNALIN(mqpeak_class, t_mqpeaks, xSample);

    class_addmethod(mqpeak_class, (t_method)SetHopSize, gensym("hopsize"),
                    A_FLOAT, 0);

    // class_addmethod(mqpeak_class, (t_method)SetFrameSize,
    // gensym("framesize"),
    //                 A_FLOAT, 0);
}
