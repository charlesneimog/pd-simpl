#include "pd-simpl.hpp"

t_class *simpl_class;

// ==============================================
typedef struct _simpl { // It seems that all the objects are some kind of class.
    t_object xObj;      // convensao no puredata source code
    t_float xSample;    // audio to fe used in CLASSMAINSIGIN
    t_int nChs;

    // multitreading
    std::vector<simpl::s_sample> audioIn;
    t_sample *previousOut;
    t_int running;

    // Peak Detection Parameters
    t_int pdFrameSize;
    t_int pdHopSize;

    // Partial Tracking Parameters
    t_int ptFrameSize;
    t_int ptHopSize;

    // Frames
    simpl::Frames frames;

    // Outlet/Inlet
    t_outlet *sigOut;
    t_int n;

    // Partial Tracking
    simpl::Peaks *pPeaks; // previous List Peak

} t_simpl;

// ==============================================
static void ThreadAudioProcessor(t_simpl *x) {
    x->running = 1;

    // Audio
    std::vector<simpl::s_sample> audio(x->audioIn);
    //
    // // Peak Detection
    int frame_size = x->pdFrameSize / 4;
    int hop_size = x->pdHopSize / 4;
    //
    // // Definitions
    simpl::MQPeakDetection MQ;
    simpl::LorisPartialTracking pt;
    simpl::SMSSynthesis synth;
    //
    x->frames = MQ.find_peaks(x->n, audio.data());
    x->frames = pt.find_partials(x->frames);
    x->frames = synth.synth(x->frames);
    //
    for (int i = 0; i < x->frames.size(); i++) {
        for (int j = 0; j < synth.hop_size(); j++) {
            x->previousOut[i * synth.hop_size() + j] = x->frames[i]->synth()[j];
        }
    }

    // AudioFrames.clear();
    // MQ.clear();
    // pt.clear();
    // synth.reset();

    x->running = 0;
}

// ==============================================
static t_int *AudioPerform(t_int *w) {
    t_simpl *x = (t_simpl *)(w[1]);
    t_sample *audioIn = (t_sample *)(w[2]);
    t_sample *audioOut = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    x->n = n;

    if (x->running) {
        pd_error(NULL, "[simpl~] previous block was not finished yet.");
        return (w + 5);
    }

    for (int i = 0; i < n; i++) {
        audioOut[i] = x->previousOut[i];
    }

    // Do the processing
    x->audioIn = std::vector<simpl::s_sample>(audioIn, audioIn + n);

    // run ThreadAudioProcessor in another detached thread
    std::thread audioThread(ThreadAudioProcessor, x);
    audioThread.detach();

    return (w + 5);
}

// ==============================================
void AddDsp(t_simpl *x, t_signal **sp) {
    if (x->previousOut != NULL) {
        free(x->previousOut);
    }
    x->previousOut = (t_sample *)malloc(sizeof(t_sample) * sp[0]->s_n);

    x->pdHopSize = sp[0]->s_n;
    x->pdFrameSize = sp[0]->s_n;

    x->ptHopSize = sp[0]->s_n;
    x->ptFrameSize = sp[0]->s_n;

    signal_setmultiout(&sp[1], sp[0]->s_nchans);
    dsp_add(AudioPerform, 4, x, sp[0]->s_vec, sp[1]->s_vec,
            SIMPL_SIGTOTAL(sp[0]));
}

// ==============================================
static void *New(void) {
    int major, minor, micro;
    sys_getversion(&major, &minor, &micro);
    if (major < 0 && minor < 54) {
        pd_error(NULL, "[simpl] You need to use Pd 0.54 or higher");
        return NULL;
    }

    t_simpl *x = (t_simpl *)pd_new(simpl_class);
    x->sigOut = outlet_new(&x->xObj, &s_signal);

    return x;
}

// ==============================================
#if defined(_LANGUAGE_C_PLUS_PLUS) || defined(__cplusplus)
extern "C" {
void simpl_setup(void);
}
#endif

// ==============================================
void simpl_setup(void) {
    post("");
    post("[simpl] by John Glover");
    post("[simpl] ported by Charles K. Neimog");
    post("[simpl] simpl version %d.%d.%d", 0, 0, 1);
    post("");

    simpl_class = class_new(gensym("simpl"), (t_newmethod)New, NULL,
                            sizeof(t_simpl), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(simpl_class, (t_method)AddDsp, gensym("dsp"), A_CANT, 0);
    CLASS_MAINSIGNALIN(simpl_class, t_simpl, xSample);

    // Peaks
    mqpeaks_tilde_setup();

    // Helpers
    getAmps_setup();
    getFreqs_setup();
}
