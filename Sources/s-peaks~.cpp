#include "pd-simpl.hpp"
#include "synthesis.h"

static t_class *PeaksDetection;
#define MAX_SILENCED_PARTIALS 127

// ==============================================
typedef struct _Peaks { // It seems that all the objects are some kind of
                        // class.
    t_object xObj;
    t_sample xSample; // audio to be used in CLASSMAINSIGIN
    bool detached;
    // method

    // silence partials
    t_float low[MAX_SILENCED_PARTIALS];
    t_float high[MAX_SILENCED_PARTIALS];
    t_int silenceIndex;

    // multitreading
    t_sample *audioIn;
    t_sample *previousOut;
    t_int warning;
    t_int running;
    t_int done;
    t_int firstblock;
    t_pdsimpl *Simpl;
    t_int n;

    // String
    std::mutex mtx;

    // AnalysisData
    AnalysisData *RealTimeData;

    // Peak Detection Parameters
    t_int FrameSize;
    t_int BufferSize;
    t_int maxPeaks;
    simpl::PeakDetection *PeakDetection;
    simpl::PartialTracking *PT;
    simpl::Synthesis *Synth;
    t_int PeakDefined;
    t_int PartialTrackingDefined;

    // Frames
    simpl::Frames *Frames;
    simpl::Frame *PdFrame;

    t_sample *in;
    t_int audioInBlockIndex;

    // Outlet/Inlet
    t_outlet *sigOut;

    // Partial Tracking
    simpl::Peaks *pPeaks; // previous List Peak

} t_Peaks;

// ==============================================
static void SetComumVariables(t_Peaks *x, t_symbol *s, int argc, t_atom *argv) {
    std::string method = s->s_name;
    if (x->PeakDetection == NULL) {
        pd_error(NULL, "[peaks~] You need to choose a Peak Detection method");
        return;
    }

    if (method == "sr") {
        x->PeakDetection->sampling_rate(atom_getfloat(argv));
    }
}

// ==============================================
static void SetMethods(t_Peaks *x, t_symbol *sMethod, t_symbol *sName) {
    std::string method = sMethod->s_name;
    std::string name = sName->s_name;
    std::string validMethods[] = {"sms", "loris", "mq", "sndobj"};
    AnalysisData *Anal = (AnalysisData *)x->RealTimeData;

    // check if the method is valid
    if (std::find(std::begin(validMethods), std::end(validMethods), name) ==
        std::end(validMethods)) {
        pd_error(NULL, "[peaks~] Unknown method");
        return;
    }

    if (method == "partial") {
        Anal->PtMethod = name;
    } else if (method == "peak") {
        Anal->PdMethod = name;
    } else if (method == "synth") {
        Anal->SyMethod = name;
    } else {
        pd_error(NULL, "[peaks~] Unknown method");
        return;
    }
    Anal->error = false;
}

// ==============================================
static void SetDetached(t_Peaks *x, t_float f) {
    if (f == 0) {
        x->detached = false;
    } else {
        x->detached = true;
    }
}

// ==============================================
static void SetMaxPartials(t_Peaks *x, t_float f) {
    AnalysisData *Anal = (AnalysisData *)x->RealTimeData;
    Anal->set_max_peaks(f);
}

// ==============================================
static void PartialTrackingProcessor(t_Peaks *x) {
    DEBUG_PRINT("[peaks~] Start PartialTracking");
    x->running = 1;
    AnalysisData *Anal = (AnalysisData *)x->RealTimeData;

    Anal->mtx.lock();

    float *in = (float *)x->in;
    std::copy(in, in + x->BufferSize, Anal->audio.begin());
    Anal->Frame.audio(&(Anal->audio[0]), x->BufferSize);
    Anal->PeakDectection();
    Anal->PartialTracking();

    Anal->mtx.unlock();

    x->done = 1;
    x->firstblock = 0;
    DEBUG_PRINT("[peaks~] Finished PartialTracking");
}

// ==============================================
static t_int *PeaksAudioPerform(t_int *w) {
    t_Peaks *x = (t_Peaks *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    int n = (int)(w[3]);
    x->n = n;
    for (int i = 0; i < n; i++) {
        x->in[x->audioInBlockIndex] = (double)in[i];
        x->audioInBlockIndex++;
        if (x->audioInBlockIndex == x->BufferSize) {
            x->audioInBlockIndex = 0;
            if (x->done && x->firstblock != 1) {
                t_atom args[1];
                SETPOINTER(&args[0], (t_gpointer *)x->RealTimeData);
                outlet_anything(x->sigOut, gensym("simplObj"), 1, args);
                x->done = 0;
            }
            if (x->detached) {
                std::thread t(PartialTrackingProcessor, x);
                t.detach();
            } else {
                PartialTrackingProcessor(x);
            }
            x->firstblock = 0;
        }
    }
    return (w + 4);
}

// ==============================================
static void PeaksAddDsp(t_Peaks *x, t_signal **sp) {
    DEBUG_PRINT("[peaks~] Adding Dsp");
    if (x->FrameSize < 512) {
        pd_error(NULL, "[peaks~] The block size must be at least 512 samples");
        dsp_add_zero(sp[0]->s_vec, sp[0]->s_n);
        return;
    }
    x->firstblock = 1;
    x->running = 0;
    dsp_add(PeaksAudioPerform, 3, x, sp[0]->s_vec, (t_int)sp[0]->s_n);
    DEBUG_PRINT("[peaks~] Dsp Rotine added");
}

// ==============================================
static void *NewPeaks(t_symbol *s, int argc, t_atom *argv) {
    t_Peaks *x = (t_Peaks *)pd_new(PeaksDetection);
    x->sigOut = outlet_new(&x->xObj, &s_anything);
    x->maxPeaks = 127; // TODO: No default
    int hopSize = 1024;

    x->FrameSize = 2048;
    x->BufferSize = 512;

    bool hopDefined = false;
    bool methodDefined = false;

    std::string pd = "";
    std::string pt = "";
    std::string sy = "";

    // search for -pd, -pt, -sy -hop
    for (int i = 0; i < argc; i++) {
        if (argv[i].a_type == A_SYMBOL) {
            std::string arg = atom_getsymbolarg(i, argc, argv)->s_name;
            t_symbol *s = atom_getsymbolarg(i, argc, argv);
            if (arg == "-pd") {
                i++;
                pd = atom_getsymbolarg(i, argc, argv)->s_name;
            } else if (arg == "-pt") {
                i++;
                pt = atom_getsymbolarg(i, argc, argv)->s_name;
            } else if (arg == "-sy") {
                i++;
                sy = atom_getsymbolarg(i, argc, argv)->s_name;
            } else if (arg == "-fr") {
                i++;
                x->FrameSize = atom_getfloatarg(i, argc, argv);
                hopDefined = true;
            } else if (arg == "-hop") {
                i++;
                x->BufferSize = atom_getfloatarg(i, argc, argv);
                hopDefined = true;
            }
        }
    }

    if (x->FrameSize < x->BufferSize) {
        pd_error(NULL,
                 "[peaks~] Frame size [-fr] must be greater than the hopsize");
        return NULL;
    }

    if (!hopDefined) {
        post("[peaks~] Using default hop size of 512 samples");
        x->FrameSize = 1024;
        x->BufferSize = 512;
    }
    // return NULL;

    // partials
    x->in = new t_sample[x->BufferSize];
    x->done = 0;
    x->silenceIndex = 0;

    // analisys
    static AnalysisData Anal(x->FrameSize, x->BufferSize);
    Anal.PdMethod = pd;
    Anal.PtMethod = pt;
    Anal.SyMethod = sy;

    x->RealTimeData = &Anal;
    DEBUG_PRINT("[peaks~] Object created");

    return (void *)x;
}

// ==============================================
void s_peaks_tilde_setup(void) {
    PeaksDetection = class_new(gensym("s-peaks~"), (t_newmethod)NewPeaks, NULL,
                               sizeof(t_Peaks), CLASS_DEFAULT, A_GIMME, 0);

    CLASS_MAINSIGNALIN(PeaksDetection, t_Peaks, xSample);
    class_addmethod(PeaksDetection, (t_method)PeaksAddDsp, gensym("dsp"),
                    A_CANT, 0);
    class_addmethod(PeaksDetection, (t_method)SetDetached, gensym("detached"),
                    A_FLOAT, 0);
    class_addmethod(PeaksDetection, (t_method)SetMaxPartials,
                    gensym("maxpartials"), A_FLOAT, 0);
    class_addmethod(PeaksDetection, (t_method)SetMethods, gensym("set"),
                    A_SYMBOL, A_SYMBOL, 0);
}
