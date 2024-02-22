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
    t_int SynthBlockSize;
    t_int FrameSize;
    t_int HopSize;
    t_int running;
    t_int block;
    t_int blockIndex;
    simpl::Synthesis *Synthesis;

    // Frames
    t_int accumFrames;
    simpl::Frames *PreviousFrames;
    simpl::Frames Frames;
    t_pdsimpl *Simpl;

    // Audio
    t_sample *altOut1;
    t_sample *altOut2;
    int blockPosition;
    int whichBlock;
    int frameIndex;
    int perform;

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
static void DetachedSynth(t_Synth *x) {
    x->running = 1;
    simpl::Frames *FramesPtr = new simpl::Frames;
    FramesPtr = x->Simpl->Frames;
    simpl::Frames Frames = *FramesPtr;
    Frames = x->Synthesis->synth(Frames);

    if (x->frameIndex) {
        for (int i = 0; i < Frames.size(); i++) {
            for (int j = 0; j < Frames[i]->synth_size(); j++) {
                x->altOut1[i * Frames.size() + j] = Frames[i]->synth()[j];
            }
        }
        x->frameIndex = 0;

    } else {
        for (int i = 0; i < Frames.size(); i++) {
            for (int j = 0; j < Frames[i]->synth_size(); j++) {
                x->altOut2[i * Frames.size() + j] = Frames[i]->synth()[j];
            }
        }
        x->frameIndex = 1;
    }
    x->running = 0;
    x->perform = 1;
    return;
}

// ==============================================
static void Synthesis(t_Synth *x, t_gpointer *p) {
    if (x->Synthesis == nullptr) {
        pd_error(0, "[s-synth~] No method set");
        return;
    }
    if (x->running == 1) {
        pd_error(0, "[s-synth~] Synthesis already running");
        return;
    }
    t_pdsimpl *Simpl = (t_pdsimpl *)p;
    x->Synthesis->reset();
    x->Synthesis->frame_size(Simpl->analWindow);
    x->Synthesis->hop_size(Simpl->hopSize);
    x->Synthesis->max_partials(Simpl->maxP);
    x->Simpl = Simpl;
    std::thread t(DetachedSynth, x);
    t.detach();
}

// ==============================================
static t_int *SynthAudioPerform(t_int *w) {
    t_Synth *x = (t_Synth *)(w[1]);
    t_sample *out = (t_sample *)(w[2]);
    int n = (int)(w[3]);
    if (!x->perform) {
        return (w + 4);
    }

    if (x->Simpl == nullptr) {
        return (w + 4);
    }

    int start_index;
    int end_index;
    start_index = x->blockPosition * n;
    end_index = start_index + n;
    int index;
    for (int i = start_index; i < end_index; i++) {
        index = i % n;
        if (x->whichBlock) {
            out[index] = x->altOut1[i];
        } else {
            out[index] = x->altOut2[i];
        }
    }

    x->blockPosition += 1;

    if (x->blockPosition * n >= 4096) {
        x->blockPosition = 0;
        if (x->whichBlock) {
            x->whichBlock = 0;
        } else {
            x->whichBlock = 1;
        }
    }

    return (w + 4);
}

// ==============================================
static void SynthAddDsp(t_Synth *x, t_signal **sp) {
    x->blockPosition = 0;
    x->whichBlock = 0;
    x->perform = 1;
    x->FrameSize = sp[0]->s_n;
    int n = sp[0]->s_n;
    dsp_add(SynthAudioPerform, 3, x, sp[0]->s_vec, sp[0]->s_n);
}

// ==============================================
static void *NewSynth(t_float f) {
    t_Synth *x = (t_Synth *)pd_new(Synth);
    x->sigOut = outlet_new(&x->xObj, &s_signal);
    x->Synthesis = nullptr;
    x->SynthMethodSet = 0; // pleonasmo
    x->frameIndex = 0;
    x->altOut1 = new t_sample[4096];
    x->altOut2 = new t_sample[4096];
    return x;
}

// ==============================================
void s_synth_tilde_setup(void) {
    Synth = class_new(gensym("s-synth~"), (t_newmethod)NewSynth, NULL,
                      sizeof(t_Synth), CLASS_DEFAULT, A_DEFFLOAT, 0);
    class_addmethod(Synth, (t_method)SynthAddDsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(Synth, (t_method)Synthesis, gensym("simplObj"), A_POINTER,
                    0);
    class_addmethod(Synth, (t_method)SetSynthesisMethod, gensym("set"),
                    A_SYMBOL, 0);
}
