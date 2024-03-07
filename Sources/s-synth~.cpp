#include "pd-simpl.hpp"

static t_class *Synth;

// ==============================================
typedef struct _Synth { // It seems that all the objects are some kind of
                        // class.
    t_object xObj;

    t_sample *previousOut;
    t_sample xSample; // audio to fe used in CLASSMAINSIGIN

    // Peak Detection Parameters
    t_int SynthMethodSet;
    t_int SynthBlockSize;
    t_int running;
    unsigned int synthDone;
    simpl::Synthesis *Synthesis;

    // Frames
    t_pdsimpl *Simpl;

    // Audio
    t_sample *out;
    int blockPosition;
    int whichBlock;
    int frameIndex;
    int perform;
    int bufferFull;

    // Outlet/Inlet
    t_outlet *sigOut;

} t_Synth;

// ==============================================
static void SetSynthesisMethod(t_Synth *x, t_symbol *s, t_symbol *argv) {
    std::string method = argv->s_name;
    if (method == "loris") {
        x->Synthesis = new simpl::LorisSynthesis();
    } else if (method == "sms") {
        x->Synthesis = new simpl::SMSSynthesis();
        ((simpl::SMSPeakDetection *)x->Synthesis)->realtime(1);
    } else if (method == "mq") {
        x->Synthesis = new simpl::MQSynthesis();
    } else if (method == "snd") {
        x->Synthesis = new simpl::SndObjSynthesis();
    } else {
        pd_error(NULL, "[peaks~] Unknown method");
        return;
    }
    x->Synthesis->sampling_rate(sys_getsr());
    x->SynthMethodSet = 1;
}

// ==============================================
static void Synthesis(t_Synth *x, t_gpointer *p) {
    DEBUG_PRINT("[synth~] Starting Synthesis");
    AnalysisData *Anal = (AnalysisData *)p;

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
    x->whichBlock = 0;
    dsp_add(SynthAudioPerform, 3, x, sp[0]->s_vec, sp[0]->s_n);
    DEBUG_PRINT("[synth~] Dsp routine added");
}

// ==============================================
static void *NewSynth(t_symbol *synth) {
    t_Synth *x = (t_Synth *)pd_new(Synth);
    x->sigOut = outlet_new(&x->xObj, &s_signal);
    x->frameIndex = 0;
    DEBUG_PRINT("[synth~] New Synth");
    return x;
}

// ==============================================
void s_synth_tilde_setup(void) {
    Synth = class_new(gensym("s-synth~"), (t_newmethod)NewSynth, NULL,
                      sizeof(t_Synth), CLASS_DEFAULT, A_DEFSYMBOL, 0);
    class_addmethod(Synth, (t_method)SynthAddDsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(Synth, (t_method)Synthesis, gensym("simplObj"), A_POINTER,
                    0);
    class_addmethod(Synth, (t_method)SetSynthesisMethod, gensym("set"),
                    A_SYMBOL, 0);
}
