#include "pd-simpl.hpp"

static t_class *PeaksDetection;

// ==============================================
typedef struct _Peaks { // It seems that all the objects are some kind of
                        // class.
    t_object xObj;
    t_sample xSample; // audio to fe used in CLASSMAINSIGIN

    // multitreading
    t_sample *audioIn;
    t_sample *previousOut;
    t_int warning;
    t_int running;
    t_int n;

    // Peak Detection Parameters
    t_int FrameSize;
    t_int HopSize;
    t_int maxPeaks;
    simpl::PeakDetection *PeakDetection;
    t_int PeakDefined;

    // Frames
    simpl::Frames *Frames;

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
static void SetPeakDetectionMethod(t_Peaks *x, t_symbol *s) {
    x->PeakDefined = 0;

    std::string method = s->s_name;
    if (method == "loris") {
        x->PeakDetection = new simpl::LorisPeakDetection();
    } else if (method == "sms") {
        x->PeakDetection = new simpl::SMSPeakDetection();
        ((simpl::SMSPeakDetection *)x->PeakDetection)->realtime(true);
    } else if (method == "mq") {
        x->PeakDetection = new simpl::MQPeakDetection();
    } else if (method == "snd") {
        x->PeakDetection = new simpl::SndObjPeakDetection();
    } else {
        pd_error(NULL, "[peaks~] Unknown method");
        return;
    }
    x->PeakDetection->sampling_rate(sys_getsr());
    x->PeakDetection->max_peaks(x->maxPeaks);
    x->PeakDetection->frame_size(512);
    x->PeakDetection->hop_size(256);

    x->PeakDefined = 1;
}

// ==============================================
static void ThreadAudioProcessor(t_Peaks *x) {
    x->running = 1;
    if (x->PeakDefined == 0) {
        return;
    }
    x->PeakDetection->frame_size(x->FrameSize);
    x->PeakDetection->hop_size(x->FrameSize / 4);
    x->PeakDetection->max_peaks(128);

    simpl::s_sample *in = new simpl::s_sample[x->FrameSize];
    std::copy(x->in, x->in + x->FrameSize, in);
    simpl::Frames *Frames = new simpl::Frames;
    *Frames = x->PeakDetection->find_peaks(x->FrameSize, in);

    // NOTE: Creationg of pdsimpl
    t_pdsimpl *pdsimpl = (t_pdsimpl *)malloc(sizeof(t_pdsimpl));
    pdsimpl->Frames = Frames;
    pdsimpl->hopSize = x->FrameSize / 4;
    pdsimpl->maxP = 128;
    pdsimpl->analWindow = x->FrameSize;
    pdsimpl->pdDetection = 1;
    pdsimpl->PD = x->PeakDetection;

    t_atom args[1];
    SETPOINTER(&args[0], (t_gpointer *)pdsimpl);
    outlet_anything(x->sigOut, gensym("simplObj"), 1, args);
    delete[] in;
    x->running = 0;
}

// ==============================================
static t_int *PeaksAudioPerform(t_int *w) {
    t_Peaks *x = (t_Peaks *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    int n = (int)(w[3]);
    x->n = n;
    for (int i = 0; i < n; i++) {
        x->in[x->audioInBlockIndex] = in[i];
        x->audioInBlockIndex++;
        if (x->audioInBlockIndex == x->FrameSize) {
            x->audioInBlockIndex = 0;
            std::thread audioThread(ThreadAudioProcessor, x);
            audioThread.detach();
        }
    }
    return (w + 4);
}

// ==============================================
static void PeaksAddDsp(t_Peaks *x, t_signal **sp) {
    if (x->FrameSize < 512) {
        pd_error(NULL, "[peaks~] The block size must be at least 512 samples");
        dsp_add_zero(sp[0]->s_vec, sp[0]->s_n);
        return;
    }
    dsp_add(PeaksAudioPerform, 3, x, sp[0]->s_vec, (t_int)sp[0]->s_n);
}

// ==============================================
static void *NewPeaks(t_symbol *s, t_float f) {

    t_Peaks *x = (t_Peaks *)pd_new(PeaksDetection);
    x->sigOut = outlet_new(&x->xObj, &s_anything);
    x->HopSize = -1;
    x->maxPeaks = 100; // TODO: No default
    x->FrameSize = f;
    x->in = new t_sample[x->FrameSize];
    x->PeakDefined = 0;

    return x;
}

// ==============================================
void s_peaks_tilde_setup(void) {
    PeaksDetection =
        class_new(gensym("s-peaks~"), (t_newmethod)NewPeaks, NULL,
                  sizeof(t_Peaks), CLASS_DEFAULT, A_SYMBOL, A_DEFFLOAT, 0);
    //
    CLASS_MAINSIGNALIN(PeaksDetection, t_Peaks, xSample);
    class_addmethod(PeaksDetection, (t_method)PeaksAddDsp, gensym("dsp"),
                    A_CANT, 0);

    class_addanything(PeaksDetection, (t_method)SetComumVariables);
    class_addmethod(PeaksDetection, (t_method)SetPeakDetectionMethod,
                    gensym("set"), A_SYMBOL, 0);
}
