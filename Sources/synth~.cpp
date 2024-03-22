#include "pd-partialtrack.hpp"

static t_class *Synth;

// ==============================================
typedef struct _Synth { // It seems that all the objects are some kind of
                        // class.
    t_object xObj;

    t_sample *previousOut;
    t_sample xSample; // audio to fe used in CLASSMAINSIGIN

    // residual
    bool residual;

    // Synth Detection Parameters
    t_int SynthBlockSize;
    unsigned int synthDone;
    std::string SyMethod;
    bool updateConfig;

    // SMS config
    unsigned int det_synthesis_type;
    // Loris config
    float bandwidth;

    // Frames
    AnalysisData *RealTimeData;
    int blockPosition;

    // Outlet/Inlet
    t_sample *out;
    t_outlet *outlet;

} t_Synth;

// ==============================================
static void UpdateSynthConfig(t_Synth *x, AnalysisData *Anal) {
    if (x->SyMethod == "sms") {
        Anal->SynthSMS.det_synthesis_type(x->det_synthesis_type);
    } else if (x->SyMethod == "loris") {
        Anal->SynthLoris.bandwidth(x->bandwidth);
    }

    Anal->residual = x->residual;
}

// ==============================================
static void ConfigSynth(t_Synth *x, t_symbol *s, int argc, t_atom *argv) {
    // Sms
    std::string method = x->SyMethod;
    x->updateConfig = true;
    std::string configWhat = atom_getsymbolarg(0, argc, argv)->s_name;
    if (configWhat == "residual") {
        x->residual = atom_getfloatarg(1, argc, argv);
        if (x->residual) {
            post("Residual Synthesis enabled");
        } else {
            post("Residual Synthesis disabled");
        }
    }

    if (method == "sms") {
        if (configWhat == "type") {
            int type = atom_getintarg(1, argc, argv);
            x->det_synthesis_type = type;
            if (type == 0) {
                post("[s-synth~] Using Inverse Fast Fourier Transform (IFFT)");
            } else if (type == 1) {
                post("[s-synth~] Using Sinusoidal Table Lookup (SIN)");
            } else {
                pd_error(x, "[s-synth~] Unknown method for %s Synth",
                         method.c_str());
            }
        } else if (configWhat == "hop_size") {
            int hop_size = atom_getintarg(1, argc, argv);
            x->SynthBlockSize = hop_size;
        } else if (configWhat == "stochastic") {
            pd_error(NULL, "[s-synth~] Stochastic not implemented yet");
        }

    } else if (method == "loris") {
        if (configWhat == "bandwidth") {
            x->bandwidth = atom_getfloatarg(1, argc, argv);
        }

    } else {
        pd_error(NULL, "[s-synth~] This object just define the 'synth' method");
        return;
    }
}

// ==============================================
static void SetMethods(t_Synth *x, t_symbol *sMethod, t_symbol *sName) {
    std::string method = sMethod->s_name;
    std::string name = sName->s_name;
    std::string validMethods[] = {"sms", "loris", "mq", "sndobj"};

    // check if the method is valid
    if (std::find(std::begin(validMethods), std::end(validMethods), name) ==
        std::end(validMethods)) {
        pd_error(NULL, "[peaks~] Unknown method");
        return;
    }
    if (method == "synth") {
        x->SyMethod = name;
    } else {
        pd_error(NULL, "[s-synth~] This object just define the 'synth' method");
        return;
    }
}

// ==============================================
static void Synthesis(t_Synth *x, t_gpointer *p) {
    DEBUG_PRINT("[synth~] Starting Synthesis");
    AnalysisData *Anal = (AnalysisData *)p;

    if (x->RealTimeData == nullptr) {
        x->RealTimeData = Anal;
    }
    if (x->updateConfig) {
        UpdateSynthConfig(x, Anal);
        x->updateConfig = false;
    }

    if (Anal->SyMethod != x->SyMethod) {
        Anal->SyMethod = x->SyMethod;
        Anal->error = false;
    }

    Anal->mtx.lock();
    Anal->Synth();

    int size = Anal->Frame.synth_size();
    if (x->out == nullptr) {
        x->out = new t_sample[size];
        x->SynthBlockSize = size;
    }

    for (unsigned int i = 0; i < size; i++) {
        x->out[i] = Anal->Frame.synth()[i];
    }

    Anal->Frame.clear_peaks();
    Anal->Frame.clear_partials();
    Anal->Frame.clear_synth();
    Anal->mtx.unlock();

    x->synthDone = 1;
    DEBUG_PRINT("[synth~] Finished Synthesis\n"); // NOTE: End of the cycle
}

// ==============================================
static t_int *SynthAudioPerform(t_int *w) {
    t_Synth *x = (t_Synth *)(w[1]);
    t_sample *out = (t_sample *)(w[2]);
    int n = (int)(w[3]);
    int i;

    // error
    if (!x->synthDone) {
        for (i = 0; i < n; i++) {
            out[i] = 0;
        }
        return (w + 4);
    }

    // copy synth for output
    for (i = 0; i < n; i++) {
        out[i] = x->out[x->blockPosition + i];
        x->out[x->blockPosition + i] = 0;
    }

    x->blockPosition = x->blockPosition + n;
    if (x->blockPosition >= x->SynthBlockSize) {
        x->blockPosition = 0;
    }

    return (w + 4);
}

// ==============================================
static void SynthAddDsp(t_Synth *x, t_signal **sp) {
    x->blockPosition = 0;
    dsp_add(SynthAudioPerform, 3, x, sp[0]->s_vec, sp[0]->s_n);
    DEBUG_PRINT("[synth~] Dsp routine added");
}

// ==============================================
static void *NewSynth(t_symbol *synth) {
    t_Synth *x = (t_Synth *)pd_new(Synth);
    x->outlet = outlet_new(&x->xObj, &s_signal);
    x->SyMethod = "sms";
    DEBUG_PRINT("[synth~] New Synth");
    return x;
}

// ==============================================
void SynthSetup(void) {
    Synth = class_new(gensym("pt-synth~"), (t_newmethod)NewSynth, NULL,
                      sizeof(t_Synth), CLASS_DEFAULT, A_DEFSYMBOL, 0);
    class_addmethod(Synth, (t_method)SynthAddDsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(Synth, (t_method)Synthesis, gensym("simplObj"), A_POINTER,
                    0);

    class_addmethod(Synth, (t_method)SetMethods, gensym("set"), A_SYMBOL,
                    A_SYMBOL, 0);

    class_addmethod(Synth, (t_method)ConfigSynth, gensym("cfg"), A_GIMME, 0);
}
