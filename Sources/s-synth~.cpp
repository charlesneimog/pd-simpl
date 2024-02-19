
#include "pd-simpl.hpp"

static t_class *Synth;

// ==============================================
typedef struct _Synth { // It seems that all the objects are some kind of
                        // class.
    t_object xObj;

    // multitreading
    std::vector<simpl::s_sample> audioOut;
    std::vector<simpl::s_sample> audioIn;
    t_sample *previousOut;
    t_int warning;
    t_int process;
    t_sample xSample; // audio to fe used in CLASSMAINSIGIN

    // Peak Detection Parameters
    t_int SynthMethodSet;
    t_int FrameSize;
    t_int HopSize;
    t_int running;
    simpl::Synthesis *Synthesis;

    // Frames
    t_int accumFrames;
    simpl::Frames *PreviousFrames;
    simpl::Frames Frames;

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
        ((simpl::SMSPeakDetection *)x->Synthesis)->realtime(true);
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
static void AddFrames(t_Synth *x, t_gpointer *p) {
    simpl::Frames &Frames =
        *(simpl::Frames *)p; // Get a reference to the Frames object
    x->Frames = simpl::Frames();
    for (int i = 0; i < Frames.size(); i++) {
        simpl::Frame *Frame = Frames[i];
        x->Frames.push_back(Frame);
    }
    x->process = 1;
    return;
}

// ==============================================
static void ThreadAudioProcessor(t_Synth *x) {
    x->running = 1;
    // x->Frames = x->Synthesis->synth(x->Frames);
    // for (int j = 0; j < x->Synthesis->hop_size(); j++) {
    //     x->previousOut[x->Synthesis->hop_size() + j] =
    //     x->Frames[0]->synth()[j];
    // }
    x->running = 0;
}

// ==============================================
static t_int *SynthAudioPerform(t_int *w) {
    t_Synth *x = (t_Synth *)(w[1]);
    t_sample *out = (t_sample *)(w[2]);
    int n = (int)(w[3]);

    if (!x->SynthMethodSet || !x->process) {
        while (n--)
            *out++ = 0;
        return (w + 4);
    }

    if (x->running) {
        pd_error(NULL, "[s-synth~] previous block was not finished yet.");
        while (n--)
            *out++ = 0;
        return (w + 4);
    }

    x->Synthesis->hop_size(x->FrameSize);
    x->Synthesis->frame_size(x->FrameSize);
    std::thread audioThread(ThreadAudioProcessor, x);
    audioThread.detach();

    while (n--) // Silencio
        *out++ = 0;

    return (w + 4);
}

// ==============================================
static void SynthAddDsp(t_Synth *x, t_signal **sp) {
    x->FrameSize = sp[0]->s_n;
    if (sp[0]->s_n < 512) {
        pd_error(NULL, "[peaks~] The block size must be at least 512 samples");
        return;
    }
    int n = sp[0]->s_n;
    x->previousOut = (t_sample *)getbytes(sizeof(t_sample) * sp[0]->s_n);
    while (n--)
        x->previousOut[n] = 0;

    dsp_add(SynthAudioPerform, 3, x, sp[0]->s_vec, sp[0]->s_n);
}

// ==============================================
static void *NewSynth(t_float f) {
    t_Synth *x = (t_Synth *)pd_new(Synth);
    x->sigOut = outlet_new(&x->xObj, &s_signal);
    x->Synthesis = nullptr;
    x->SynthMethodSet = 0; // pleonasmo
    return x;
}

// ==============================================
void s_synth_tilde_setup(void) {
    Synth = class_new(gensym("s-synth~"), (t_newmethod)NewSynth, NULL,
                      sizeof(t_Synth), CLASS_DEFAULT, A_DEFFLOAT, 0);
    class_addmethod(Synth, (t_method)SynthAddDsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(Synth, (t_method)AddFrames, gensym("Frames"), A_POINTER, 0);
    class_addmethod(Synth, (t_method)SetSynthesisMethod, gensym("set"),
                    A_SYMBOL, 0);
}
