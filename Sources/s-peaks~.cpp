#include "pd-simpl.hpp"

static t_class *PeaksDetection;

// ==============================================
typedef struct _Peaks { // It seems that all the objects are some kind of
                        // class.
    t_object xObj;
    t_sample xSample; // audio to fe used in CLASSMAINSIGIN

    // multitreading
    std::vector<simpl::s_sample> audioIn;
    t_sample *previousOut;
    t_int warning;

    // Peak Detection Parameters
    t_int FrameSize;
    t_int HopSize;
    t_int maxPeaks;
    simpl::PeakDetection *PeakDetection;

    // Frames
    simpl::Frames Frames;

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
}

// ==============================================
static t_int *PeaksAudioPerform(t_int *w) {
    t_Peaks *x = (t_Peaks *)(w[1]);
    t_sample *audioIn = (t_sample *)(w[2]);
    int n = (int)(w[3]);

    if (n < 128 && !x->warning) {
        pd_error(NULL,
                 "Frame size is very small, use [switch~ 2048] to change it!");
    }

    std::vector<simpl::s_sample> audio(audioIn, audioIn + n);
    x->PeakDetection->frame_size(n);
    x->PeakDetection->hop_size(n);
    x->PeakDetection->max_peaks(x->maxPeaks);
    x->Frames = x->PeakDetection->find_peaks(n, audio.data());
    t_atom args[1];
    SETPOINTER(&args[0], (t_gpointer *)&x->Frames);
    outlet_anything(x->sigOut, gensym("Frames"), 1, args);
    return (w + 4);
}

// ==============================================
static void PeaksAddDsp(t_Peaks *x, t_signal **sp) {
    if (x->PeakDetection == NULL) {
        pd_error(NULL, "[peaks~] You need to choose a Peak Detection method!");
        return;
    }
    dsp_add(PeaksAudioPerform, 3, x, sp[0]->s_vec, (t_int)sp[0]->s_n);
}

// ==============================================
static void *NewPeaks(void) {

    t_Peaks *x = (t_Peaks *)pd_new(PeaksDetection);
    x->sigOut = outlet_new(&x->xObj, &s_anything);
    x->HopSize = -1;
    x->maxPeaks = 100; // TODO: No default

    return x;
}

// ==============================================
void s_peaks_tilde_setup(void) {
    PeaksDetection = class_new(gensym("s-peaks~"), (t_newmethod)NewPeaks, NULL,
                               sizeof(t_Peaks), CLASS_DEFAULT, A_GIMME, 0);
    //
    CLASS_MAINSIGNALIN(PeaksDetection, t_Peaks, xSample);
    class_addmethod(PeaksDetection, (t_method)PeaksAddDsp, gensym("dsp"),
                    A_CANT, 0);

    class_addanything(PeaksDetection, (t_method)SetComumVariables);
    class_addmethod(PeaksDetection, (t_method)SetPeakDetectionMethod,
                    gensym("set"), A_SYMBOL, 0);
}
