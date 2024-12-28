#include "partialtrack.hpp"

static t_class *peaks_class;
#define MAX_SILENCED_PARTIALS 127

// ==============================================
typedef struct _Peaks {
    t_object xObj;
    t_sample xSample;

    // Multitreading
    bool detached;
    t_sample *audioIn;
    t_int running;
    t_int done;
    t_int firstblock;
    t_int n;

    // Config
    t_int fr_resources = 0;
    bool offline;

    // clock
    t_clock *x_clock;

    // AnalysisData
    AnalysisData *RealTimeData;
    t_int FrameSize;
    t_int BufferSize;
    t_int maxPeaks;
    t_symbol *AnalPtrStr;
    t_PtrPartialAnalysis *DataObj;

    t_sample *in;
    t_int audioInBlockIndex;

    // Outlet/Inlet
    t_outlet *sigOut;
    t_outlet *info;

} t_Peaks;

// ─────────────────────────────────────
static void peaks_configure(t_Peaks *x, t_symbol *s, int argc, t_atom *argv) {
    // Sms
    std::string method = x->RealTimeData->PtMethod;
    if (method == "sms") {
        std::string configWhat = atom_getsymbolarg(0, argc, argv)->s_name;

        std::string validMethods[] = {"harmonic"};
        if (std::find(std::begin(validMethods), std::end(validMethods),
                      configWhat) == std::end(validMethods)) {
            pd_error(NULL, "[peaks~] Unknown method for %s SMS PartialTracking",
                     method.c_str());
        }

        if (configWhat == "harmonic") {
            int harmonic = atom_getintarg(1, argc, argv);
            if (harmonic == 0) {
                x->RealTimeData->PtSMS.harmonic(harmonic);
                post("[peaks~] Selected Harmonic PartialTracking without "
                     "phase");
            } else if (harmonic == 1) {
                x->RealTimeData->PtSMS.harmonic(harmonic);
                post("[peaks~] Selected Inharmonic PartialTracking without "
                     "phase");
            } else if (harmonic == 2) {
                x->RealTimeData->PtSMS.harmonic(harmonic);
                post("[peaks~] Selected Harmonic PartialTracking with phase");
            } else if (harmonic == 3) {
                x->RealTimeData->PtSMS.harmonic(harmonic);
                post("[peaks~] Selected Inharmonic PartialTracking with phase");
            } else {
                pd_error(NULL,
                         "[peaks~] Unknown method for %s SMS PartialTracking",
                         method.c_str());
            }
        }
    }
}

// ==============================================
static void peaks_set(t_Peaks *x, t_symbol *sMethod, t_symbol *sName) {
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
    } else {
        pd_error(NULL, "[s-peaks~] This object just define the 'peak' and "
                       "'partial' methods");
        return;
    }
    Anal->error = false;
}

// ==============================================
static void peaks_offlinemode(t_Peaks *x, t_float f) {
    if (f == 0) {
        x->offline = false;
    } else {
        x->offline = true;
    }
}

// ==============================================
static void peaks_detached(t_Peaks *x, t_float f) {
    if (f == 0) {
        x->detached = false;
    } else {
        x->detached = true;
    }
}

// ==============================================
static void peaks_set(t_Peaks *x, t_symbol *s, int argc, t_atom *argv) {
    std::string method = atom_getsymbolarg(0, argc, argv)->s_name;
    if (method == "framesize") {
        unsigned int value = atom_getintarg(1, argc, argv);
        x->RealTimeData->set_max_peaks(value);
    } else if (method == "hopsize") {

    } else if (method == "fr_resources") {
        x->fr_resources = atom_getintarg(1, argc, argv);
        post("[peaks~] I will not process when audio is 0");
    } else if (method == "peak") {
        x->RealTimeData->PdMethod = atom_getsymbolarg(1, argc, argv)->s_name;
    } else if (method == "partial") {
        x->RealTimeData->PtMethod = atom_getsymbolarg(1, argc, argv)->s_name;
    } else if (method == "harmonic") {
        // TODO:
        pd_error(NULL, "[peaks~] This method is not implemented yet");
    } else if (method == "fundamental") {
        pd_error(NULL, "[peaks~] This method is not implemented yet");
    } else {
        pd_error(NULL, "[peaks~] Unknown configuration method");
    }

    return;
}

// ==============================================
static void peaks_setmaxpartials(t_Peaks *x, t_float f) {
    AnalysisData *Anal = (AnalysisData *)x->RealTimeData;
    Anal->set_max_peaks(f);
}

// ==============================================
static void peaks_tick(t_Peaks *x) {
    t_atom args[1];

    SETSYMBOL(&args[0], x->AnalPtrStr);
    outlet_anything(x->sigOut, gensym("PtObj"), 1, args);

    x->RealTimeData->Frame.clear_peaks();
    x->RealTimeData->Frame.clear_partials();
    x->RealTimeData->Frame.clear_synth();
}

// ==============================================
static void peaks_processor(t_Peaks *x) {

    AnalysisData *Anal = (AnalysisData *)x->RealTimeData;

    float *in = (float *)x->in;
    std::copy(in, in + x->BufferSize, Anal->audio.begin());
    Anal->Frame.audio(&(Anal->audio[0]), x->BufferSize);
    Anal->PeakDectection();
    Anal->PartialTracking();

    clock_delay(x->x_clock, 0);
    DEBUG_PRINT("[peaks~] Finished PartialTracking");
}

// ==============================================
static void peaks_processoffline(t_Peaks *x, t_symbol *s) {
    x->offline = true;
    t_garray *array;
    int vecsize;
    t_word *vec;

    if (!(array = (t_garray *)pd_findbyclass(s, garray_class))) {
        pd_error(x, "[Python] Array %s not found.", s->s_name);
        return;
    } else if (!garray_getfloatwords(array, &vecsize, &vec)) {
        pd_error(x, "[Python] Bad template for tabwrite '%s'.", s->s_name);
        return;
    }

    int nWindows = round(vecsize / x->BufferSize);

    double *in = new double[vecsize];
    for (int i = 0; i < vecsize; i++) {
        in[i] = vec[i].w_float;
    }

    x->RealTimeData->PeakDectectionFrames(vecsize, in);
    x->RealTimeData->PartialTrackingFrames();

    t_atom args[1];
    SETSYMBOL(&args[0], x->AnalPtrStr);
    outlet_anything(x->sigOut, gensym("PtObjFrames"), 1, args);

    SETFLOAT(&args[0], x->RealTimeData->Frames.size());
    outlet_anything(x->info, gensym("frames"), 1, args);

    return;
}

// ==============================================
static t_int *peaks_perform(t_int *w) {
    t_Peaks *x = (t_Peaks *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    int n = (int)(w[3]);
    x->n = n;

    if (x->offline) {
        // TODO: way to disable this
        return (w + 4);
    }

    if (x->fr_resources) {
        // check if audio sum is zero
        float sum = 0;
        for (int i = 0; i < n; i++) {
            sum += abs(in[i]);
        }
        if (sum == 0) {
            return (w + 4);
        }
    }

    for (int i = 0; i < n; i++) {
        x->in[x->audioInBlockIndex] = (double)in[i];
        x->audioInBlockIndex++;
        if (x->audioInBlockIndex == x->BufferSize) {
            x->audioInBlockIndex = 0;
            peaks_processor(x);
        }
    }
    return (w + 4);
}

// ==============================================
static void peaks_dsp(t_Peaks *x, t_signal **sp) {
    DEBUG_PRINT("[peaks~] Adding Dsp");
    dsp_add(peaks_perform, 3, x, sp[0]->s_vec, (t_int)sp[0]->s_n);
    DEBUG_PRINT("[peaks~] Dsp Rotine added");
}

// ==============================================
static void *peaks_new(t_symbol *s, int argc, t_atom *argv) {
    t_Peaks *x = (t_Peaks *)pd_new(peaks_class);
    x->sigOut = outlet_new(&x->xObj, &s_anything);
    x->x_clock = clock_new(x, (t_method)peaks_tick);

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
            } else if (arg == "-offline") {
                x->offline = true;
                x->info = outlet_new(&x->xObj, &s_anything);
            }
        }
    }

    if (x->FrameSize < x->BufferSize) {
        pd_error(NULL,
                 "[peaks~] Frame size [-fr] must be greater than the hopsize");
        return NULL;
    }

    if (!hopDefined) {
        post("[peaks~] Using default FrameSize of 1024 and HopSize of 512 "
             "samples");
        x->FrameSize = 1024;
        x->BufferSize = 512;
    }

    // partials
    x->in = new t_sample[x->BufferSize];
    x->done = 0;

    x->DataObj = newAnalisysPtr(x->FrameSize, x->BufferSize);
    x->AnalPtrStr = x->DataObj->x_sym;

    x->RealTimeData = getAnalisysPtr(x->AnalPtrStr);
    if (x->RealTimeData == nullptr) {
        pd_error(NULL, "[peaks~] Pointer not found");
        return NULL;
    }
    x->RealTimeData->PdMethod = pd;
    x->RealTimeData->PtMethod = pt;
    x->RealTimeData->SyMethod = sy;

    return x;
}
// ==============================================
void peaks_free(t_Peaks *x) {
    killAnalisysPtr(x->DataObj);
    delete x->DataObj;
    delete x->RealTimeData;
    delete x->in;
    clock_free(x->x_clock);
}

// ==============================================
extern "C" void peaks_tilde_setup(void) {
    peaks_class = class_new(gensym("peaks~"), (t_newmethod)peaks_new, NULL,
                            sizeof(t_Peaks), CLASS_DEFAULT, A_GIMME, 0);
    CLASS_MAINSIGNALIN(peaks_class, t_Peaks, xSample);
    class_addmethod(peaks_class, (t_method)peaks_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(peaks_class, (t_method)peaks_processoffline, gensym("tab"),
                    A_SYMBOL, 0);
    class_addmethod(peaks_class, (t_method)peaks_detached, gensym("detached"),
                    A_FLOAT, 0);
    class_addmethod(PeaksDetection, (t_method)peaks_set, gensym("set"), A_GIMME,
                    0);
    class_addmethod(peaks_class, (t_method)peaks_configure, gensym("ptcfg"),
                    A_GIMME, 0);
    class_addmethod(peaks_class, (t_method)peaks_offlinemode, gensym("offline"),
                    A_FLOAT, 0);
}
