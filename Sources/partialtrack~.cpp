#include "partialtrack.hpp"

static t_class *partialtrack_tilde_class;
#define MAX_SILENCED_PARTIALS 127

// ─────────────────────────────────────
typedef struct _partialtrack_tilde {
    t_object xObj;
    t_sample xSample;

    // Multitreading
    bool detached;
    t_sample *audioIn;
    t_int running;
    t_int done;
    t_int firstblock;
    t_int n;

    // Manipulations
    TransParams *Params;

    // Config
    t_int fr_resources = 0;
    bool offline;

    // clock
    t_clock *x_clock;

    // AnalysisData
    AnalysisData *RealTimeData;
    t_int Sr;
    t_int FFTSize;
    t_int HopSize;
    t_int MaxPeaks;
    t_symbol *AnalPtrStr;
    t_PtrPartialAnalysis *DataObj;

    t_sample *in;
    t_sample *out;

    std::vector<double> outBuffer;
    float BlockSize;
    unsigned BlockIndex;

    // Outlet/Inlet
    t_outlet *sigOut;
    t_outlet *info;

} t_partialtrack_tilde;

// ─────────────────────────────────────
static void partialtrack_manipulations(t_partialtrack_tilde *x, t_symbol *s,
                                       int argc, t_atom *argv) {

    std::string method = atom_getsymbolarg(0, argc, argv)->s_name;

    // silence partials
    if (method == "silence") {
        if (x->Params->sIndex < MAX_SILENCED_PARTIALS) {
            float centerFreq = atom_getfloatarg(1, argc, argv);
            float variation = atom_getfloatarg(2, argc, argv);
            x->Params->sMidi[x->Params->sIndex] = ftom(centerFreq);
            x->Params->sVariation[x->Params->sIndex] = variation;
            x->Params->sIndex++;
        } else {
            pd_error(x, "[peaks~] Maximum number of silenced partials reached");
        }
    } else if (method == "reset") {
        x->Params->sIndex = 0;
        x->Params->aIndex = 0;
        x->Params->tIndex = 0;
    }
}

// ─────────────────────────────────────
static void partialtrack_set(t_partialtrack_tilde *x, t_symbol *s, int argc,
                             t_atom *argv) {
    std::string method = atom_getsymbolarg(0, argc, argv)->s_name;

    return;
}

// ─────────────────────────────────────
static void partialtrack_execute(t_partialtrack_tilde *x) {
    AnalysisData *Anal = (AnalysisData *)x->RealTimeData;
    float *in = (float *)x->in;
    std::copy(in, in + x->HopSize, Anal->audio.begin());
    Anal->Frame.audio(&(Anal->audio[0]), x->HopSize);
    Anal->PeakDectection();
    Anal->PartialTracking();

    // Manipulations
    for (int i = 0; i < Anal->Frame.num_partials(); i++) {
        simpl::Frame *Frame = &Anal->Frame;
        simpl::Peak *Peak = Frame->partial(i);
        x->Params->silence(Peak);
    }

    // Synth
    Anal->Synth();
    int size = Anal->Frame.synth_size();
    std::copy(Anal->Frame.synth(), Anal->Frame.synth() + size, x->out);

    // Clear
    x->RealTimeData->Frame.clear_peaks();
    x->RealTimeData->Frame.clear_partials();
    x->RealTimeData->Frame.clear_synth();
}

// ─────────────────────────────────────
static t_int *partialtrack_perform(t_int *w) {
    t_partialtrack_tilde *x = (t_partialtrack_tilde *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    x->n = n;

    for (int i = 0; i < n; i++) {
        x->in[x->BlockIndex] = (double)in[i];
        out[i] = x->out[x->BlockIndex];

        x->BlockIndex++;
        if (x->BlockIndex == x->HopSize) {
            x->BlockIndex = 0;
            partialtrack_execute(x);
        }
    }

    return (w + 5);
}

// ─────────────────────────────────────
static void partialtrack_dsp(t_partialtrack_tilde *x, t_signal **sp) {
    dsp_add(partialtrack_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec,
            (t_int)sp[0]->s_n);
}

// ─────────────────────────────────────
static void *partialtrack_new(t_symbol *s, int argc, t_atom *argv) {
    t_partialtrack_tilde *x =
        (t_partialtrack_tilde *)pd_new(partialtrack_tilde_class);
    x->sigOut = outlet_new(&x->xObj, &s_signal);
    x->Sr = sys_getsr();
    x->FFTSize = 2048;
    x->HopSize = 512;
    x->MaxPeaks = 50;

    std::string pd = "loris";
    std::string pt = "sms";
    std::string sy = "sms";

    // search for -pd, -pt, -sy -hop
    for (int i = 0; i < argc; i++) {
        if (argv[i].a_type == A_SYMBOL) {
            std::string arg = atom_getsymbolarg(i, argc, argv)->s_name;
            if (arg == "-m") {
                i++;
                pd = atom_getsymbolarg(i, argc, argv)->s_name;
                i++;
                pt = atom_getsymbolarg(i, argc, argv)->s_name;
                i++;
                sy = atom_getsymbolarg(i, argc, argv)->s_name;
                post("[partialtrack~] Using %s %s %s PartialTracking",
                     pd.c_str(), pt.c_str(), sy.c_str());
            }
        } else if (argv[i].a_type == A_FLOAT) {
            x->FFTSize = atom_getfloatarg(i, argc, argv);
            i++;
            if (i < argc) {
                x->HopSize = atom_getfloatarg(i, argc, argv);
            } else {
                pd_error(x, "[partialtrack~] Hopsize not found. Use "
                            "[partialtrack~ 2048 512] for example");
                return nullptr;
            }
        }
    }

    if (x->FFTSize < x->HopSize) {
        pd_error(NULL, "[partialtrack~] Frame size [-fr] must be greater than "
                       "the hopsize");
        return NULL;
    }

    x->RealTimeData = new AnalysisData(x->Sr, x->FFTSize, x->HopSize);
    if (x->RealTimeData == nullptr) {
        pd_error(x, "[partialtrack~] Pointer not found");
        return nullptr;
    }

    x->RealTimeData->PdMethod = pd;
    x->RealTimeData->PtMethod = pt;
    x->RealTimeData->SyMethod = sy;
    x->RealTimeData->set_max_peaks(x->MaxPeaks);
    int size = x->RealTimeData->Frame.synth_size();

    x->Params = new TransParams;
    x->Params->reset();
    x->in = new t_sample[x->FFTSize];
    x->out = new t_sample[size];

    return x;
}

// ─────────────────────────────────────
void partialtrack_free(t_partialtrack_tilde *x) {
    killAnalisysPtr(x->DataObj);
    delete x->DataObj;
    delete x->RealTimeData;
    delete x->in;
    delete x->out;
    delete x->Params;
}

// ─────────────────────────────────────
extern "C" void partialtrack_tilde_setup(void) {
    partialtrack_tilde_class =
        class_new(gensym("partialtrack~"), (t_newmethod)partialtrack_new, NULL,
                  sizeof(t_partialtrack_tilde), CLASS_DEFAULT, A_GIMME, 0);
    CLASS_MAINSIGNALIN(partialtrack_tilde_class, t_partialtrack_tilde, xSample);
    class_addmethod(partialtrack_tilde_class, (t_method)partialtrack_dsp,
                    gensym("dsp"), A_CANT, 0);
    class_addmethod(partialtrack_tilde_class, (t_method)partialtrack_set,
                    gensym("set"), A_GIMME, 0);
    class_addmethod(partialtrack_tilde_class,
                    (t_method)partialtrack_manipulations, gensym("man"),
                    A_GIMME, 0);
}
