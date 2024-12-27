#ifndef PARTIALTRACK_H
#define PARTIALTRACK_H

// check if it is a windows build
#ifdef _WIN32
#define PI = M_PI
#endif

#include <m_pd.h>

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <simpl.h> // TODO: Need to fix this
#pragma GCC diagnostic warning "-Wdeprecated-declarations"

// #include <sdif.h>

#define SIMPL_SIGTOTAL(s) ((t_int)((s)->s_length * (s)->s_nchans))
#ifdef DEBUG_MODE
#define DEBUG_PRINT(message) printf("[DEBUG] %s\n", message)
#else
#define DEBUG_PRINT(message) // Define como vazio em builds sem debug
#endif

class AnalysisData {
  public:
    bool configured = false;
    bool error = false;
    const int max_peaks;
    std::vector<double> audio;
    unsigned HopSize;
    unsigned BufferSize;

    simpl::Frames Frames;
    simpl::Frame Frame;

    std::string PdMethod;
    std::string PtMethod;
    std::string SyMethod;
    bool residual = false;

    // ────────────── Offline ───────────
    bool offline = false;

    // ─────────── PeakDetection ────────
    simpl::SndObjPeakDetection PdSnd;
    simpl::LorisPeakDetection PdLoris;
    simpl::SMSPeakDetection PdSMS;
    simpl::MQPeakDetection PdMQ;

    // ────────── PartialTracking ───────
    simpl::SndObjPartialTracking PtSnd;
    simpl::LorisPartialTracking PtLoris;
    simpl::SMSPartialTracking PtSMS;
    simpl::MQPartialTracking PtMQ;

    // ───────────── Synthesis ──────────
    simpl::SndObjSynthesis SynthSnd;
    simpl::LorisSynthesis SynthLoris;
    simpl::SMSSynthesis SynthSMS;
    simpl::MQSynthesis SynthMQ;

    // ────────────── Residual ──────────────
    simpl::Residual Residual;

    // ────────────── Methods ───────────
    void set_max_peaks(int max_peaks);
    void PeakDectection();
    void PeakDectectionFrames(int audioSize, double *audio);

    // ------
    void PartialTracking();
    void PartialTrackingFrames();

    // ------
    void Synth();
    void SynthFrames();
    void SynthFreezedFrames(simpl::Frames Frames);

    // ------
    void SetHopSize(unsigned int HopSize);

    // ──────────── Initializer ─────────
    AnalysisData(int frame_size, int hop_size)
        : max_peaks(256), Frame(frame_size, true) {

        HopSize = hop_size;
        BufferSize = frame_size;

        audio.resize(hop_size);

        Frame.synth_size(hop_size);
        Frame.max_peaks(max_peaks);
        Frame.max_partials(max_peaks);

        PdLoris.frame_size(frame_size);
        PdLoris.hop_size(hop_size);
        PdLoris.max_peaks(max_peaks);
        PdSMS.frame_size(frame_size);
        PdSMS.hop_size(hop_size);
        PdSMS.max_peaks(max_peaks);
        PdSnd.frame_size(frame_size);
        PdSnd.hop_size(hop_size);
        PdSnd.max_peaks(max_peaks);
        PdMQ.frame_size(frame_size);
        PdMQ.hop_size(hop_size);
        PdMQ.max_peaks(max_peaks);

        PtLoris.max_partials(max_peaks);
        PtSnd.max_partials(max_peaks);
        PtMQ.max_partials(max_peaks);
        PtSMS.max_partials(max_peaks);
        PtSMS.realtime(true);

        SynthSMS.det_synthesis_type(0);
        SynthSMS.hop_size(hop_size);
        SynthSMS.max_partials(frame_size);

        SynthLoris.hop_size(hop_size);
        SynthLoris.max_partials(frame_size);

        SynthSnd.hop_size(hop_size);
        SynthSnd.max_partials(frame_size);

        SynthMQ.hop_size(hop_size);
        SynthMQ.max_partials(frame_size);
    }
};

// ╭─────────────────────────────────────╮
// │            AnalData Ptr             │
// ╰─────────────────────────────────────╯

typedef struct _PtrPartialAnalysis {
    t_pd x_pd;
    t_symbol *x_sym;
    AnalysisData *x_data;
} t_PtrPartialAnalysis;

t_PtrPartialAnalysis *newAnalisysPtr(int frameSize, int bufferSize);
void killAnalisysPtr(t_PtrPartialAnalysis *x);
AnalysisData *getAnalisysPtr(t_symbol *s);

#endif // PARTIALTRACK_H
