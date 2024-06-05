#include "partialtrack.hpp"

static t_class *Synth;

// ==============================================
typedef struct _Synth { // It seems that all the objects are some kind of
                        // class.
    t_object xObj;
    t_sample *out;

    // Synth Detection Parameters
    t_int SynthBlockSize;
    unsigned int synthDone;
    std::string SyMethod;
    bool updateConfig;

    // SMS config
    unsigned int det_synthesis_type;
    bool residual;
    // Loris config
    float bandwidth;

    // Offline Mode
    bool offline;
    t_symbol *ArrayName;

    // freeze
    bool freeze;
    unsigned int freezeFrame;
    std::vector<float> freezeAudio;

    simpl::Frame PreviousFrame;
    simpl::Frame CurrentFrame;

    // Frames
    AnalysisData *RealTimeData;
    int blockPosition;

    // Options
    float speed;

    // Outlet/Inlet
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
static void OfflineMode(t_Synth *x, t_float f) {
    if (f == 0) {
        x->offline = false;
    } else {
        x->offline = true;
    }
}

// ==============================================
static void FreezeMode(t_Synth *x, t_float f) {
    if (f == 0) {
        x->freeze = false;
    } else {
        x->freeze = true;
        post("[synth~] Freeze Synth Mode enabled");
    }
}

// ==============================================
static void ProcessOffline(t_Synth *x, t_symbol *p) {
    AnalysisData *Anal = getAnalisysPtr(p);
    if (Anal == nullptr) {
        pd_error(NULL, "[synth~] Pointer not found");
        return;
    }
    if (Anal->SyMethod != x->SyMethod) {
        Anal->SyMethod = x->SyMethod;
        Anal->error = false;
    }

    if (x->speed != 1) {
        Anal->SetHopSize(Anal->HopSize / x->speed);
    }

    Anal->SynthFrames();

    std::vector<float> audio;
    int FramesSize = Anal->Frames.size();

    for (int i = 0; i < FramesSize; i++) {
        for (int j = 0; j < Anal->Frame.synth_size(); j++) {
            audio.push_back(Anal->Frames[i]->synth()[j]);
        }
    }

    // write in array the audio
    if (x->ArrayName != nullptr) {
        t_garray *array;
        int vecsize;
        t_word *vec;
        if (!(array = (t_garray *)pd_findbyclass(x->ArrayName, garray_class))) {
            pd_error(NULL, "[synth~] Array [%s] not found",
                     x->ArrayName->s_name);
            return;
        } else if (!garray_getfloatwords(array, &vecsize, &vec)) {
            pd_error(x, "[synth~] Bad template for tabwrite '%s'.",
                     x->ArrayName->s_name);
            return;
        }
        if (vecsize < audio.size()) {
            garray_resize_long(array, audio.size());
            garray_getfloatwords(array, &vecsize, &vec);
        }
        for (int i = 0; i < audio.size(); i++) {
            vec[i].w_float = audio[i];
        }
        garray_redraw(array);
    }
    post("[synth~] Finished Synthesis");
}

// ==============================================
static void SynthesisSymbol(t_Synth *x, t_symbol *p) {
    AnalysisData *Anal = getAnalisysPtr(p);
    if (Anal == nullptr) {
        pd_error(NULL, "[synth~] Pointer not found");
        return;
    }
    int size = Anal->Frame.synth_size();

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

    Anal->Synth();

    size = Anal->Frame.synth_size();

    if (x->out == nullptr) {
        x->out = new t_sample[size];
        x->SynthBlockSize = size;
    }

    std::copy(Anal->Frame.synth(), Anal->Frame.synth() + size, x->out);

    x->synthDone = 1;
    DEBUG_PRINT("[synth~] Finished Synthesis\n"); // NOTE: End of the cycle
}

// ==============================================
static void SetMethods(t_Synth *x, t_symbol *s, int argc, t_atom *argv) {
    if (argc < 2) {
        pd_error(NULL, "[peaks~] Missing arguments");
        return;
    }

    if (argv[0].a_type != A_SYMBOL) {
        pd_error(NULL, "[peaks~] Invalid arguments");
        return;
    }

    std::string method = atom_getsymbolarg(0, argc, argv)->s_name;

    if (method == "synth") {
        std::string name = atom_getsymbolarg(1, argc, argv)->s_name;
        std::string validMethods[] = {"sms", "loris", "mq", "sndobj"};
        if (std::find(std::begin(validMethods), std::end(validMethods), name) ==
            std::end(validMethods)) {
            pd_error(NULL, "[peaks~] Unknown method");
            return;
        }
        x->SyMethod = name;
    } else if (method == "speed") {
        x->speed = atom_getfloatarg(1, argc, argv);
    } else if (method == "freezedframe") {
        x->freezeFrame = atom_getfloatarg(1, argc, argv);
    } else {
        pd_error(NULL, "[s-synth~] This object just define the 'synth' method");
        return;
    }
}

// ==============================================
void FreezeSynth(t_Synth *x) {
    simpl::Frame *Frame = x->RealTimeData->Frames[x->freezeFrame];
    simpl::Frame *NextFrame = x->RealTimeData->Frames[x->freezeFrame];

    simpl::Frames Frames(2);
    Frames.push_back(Frame);
    Frames.push_back(NextFrame);

    x->RealTimeData->SynthFreezedFrames(Frames);

    std::vector<float> audio;
}

// ==============================================
static t_int *SynthAudioPerform(t_int *w) {
    t_Synth *x = (t_Synth *)(w[1]);
    t_sample *out = (t_sample *)(w[2]);
    int n = (int)(w[3]);
    int i;

    if (x->freeze) {
        if (x->freezeFrame == -1 || x->RealTimeData == nullptr) {
            return (w + 4);
        }
        if (x->RealTimeData->Frames.size() < x->freezeFrame) {
            return (w + 4);
        }
        post("here");
    }

    if (x->offline) {
        return (w + 4);
    }

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
    x->synthDone = 0;

    dsp_add(SynthAudioPerform, 3, x, sp[0]->s_vec, sp[0]->s_n);
    DEBUG_PRINT("[synth~] Dsp routine added");
}

// ==============================================
static void *NewSynth(t_symbol *s, int argc, t_atom *argv) {
    t_Synth *x = (t_Synth *)pd_new(Synth);
    x->outlet = outlet_new(&x->xObj, &s_signal);
    x->SyMethod = "sms";

    for (int i = 0; i < argc; i++) {
        if (argv[i].a_type == A_SYMBOL) {
            std::string arg = atom_getsymbolarg(i, argc, argv)->s_name;
            t_symbol *s = atom_getsymbolarg(i, argc, argv);
            if (arg == "-offline") {
                x->offline = true;
                i++;
                if (i < argc) {
                    x->ArrayName = atom_getsymbolarg(i, argc, argv);
                } else {
                    pd_error(NULL, "[synth~] No array name defined");
                }
            }
        }
    }

    // default config
    x->speed = 1;
    x->freeze = false;
    x->freezeFrame = -1;

    DEBUG_PRINT("[synth~] New Synth");
    return x;
}

// ==============================================
void SynthSetup(void) {
    Synth = class_new(gensym("pt-synth~"), (t_newmethod)NewSynth, NULL,
                      sizeof(t_Synth), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(Synth, (t_method)SynthAddDsp, gensym("dsp"), A_CANT, 0);

    class_addmethod(Synth, (t_method)SetMethods, gensym("set"), A_GIMME, 0);

    class_addmethod(Synth, (t_method)ConfigSynth, gensym("cfg"), A_GIMME, 0);
    class_addmethod(Synth, (t_method)OfflineMode, gensym("offline"), A_FLOAT,
                    0);
    class_addmethod(Synth, (t_method)FreezeMode, gensym("freeze"), A_FLOAT, 0);

    class_addmethod(Synth, (t_method)SynthesisSymbol, gensym("PtObj"), A_SYMBOL,
                    0);
    class_addmethod(Synth, (t_method)ProcessOffline, gensym("PtObjFrames"),
                    A_SYMBOL, 0);
}
