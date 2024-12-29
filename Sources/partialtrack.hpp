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

#include "manipulations.hpp"

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
    AnalysisData(int sr, int frame_size, int hop_size)
        : max_peaks(256), Frame(frame_size, true) {

        HopSize = hop_size;
        BufferSize = frame_size;

        audio.resize(hop_size);

        Frame.synth_size(hop_size);
        Frame.max_peaks(max_peaks);
        Frame.max_partials(max_peaks);

        PdLoris.sampling_rate(sr);
        PdLoris.frame_size(frame_size);
        PdLoris.hop_size(hop_size);
        PdLoris.max_peaks(max_peaks);

        PdSMS.sampling_rate(sr);
        PdSMS.frame_size(frame_size);
        PdSMS.hop_size(hop_size);
        PdSMS.max_peaks(max_peaks);

        PdSnd.sampling_rate(sr);
        PdSnd.frame_size(frame_size);
        PdSnd.hop_size(hop_size);
        PdSnd.max_peaks(max_peaks);

        PdMQ.sampling_rate(sr);
        PdMQ.frame_size(frame_size);
        PdMQ.hop_size(hop_size);
        PdMQ.max_peaks(max_peaks);

        // Partial Tracking
        PtLoris.sampling_rate(sr);
        PtLoris.max_partials(max_peaks);

        PtSnd.sampling_rate(sr);
        PtSnd.max_partials(max_peaks);

        PtMQ.sampling_rate(sr);
        PtMQ.max_partials(max_peaks);

        PtSMS.sampling_rate(sr);
        PtSMS.max_partials(max_peaks);
        PtSMS.realtime(true);

        SynthSMS.sampling_rate(sr);
        SynthSMS.det_synthesis_type(0);
        SynthSMS.hop_size(hop_size);
        SynthSMS.max_partials(frame_size);
        SynthSMS.reset();

        SynthLoris.sampling_rate(sr);
        SynthLoris.hop_size(hop_size);
        SynthLoris.max_partials(frame_size);
        SynthLoris.reset();

        SynthSnd.sampling_rate(sr);
        SynthSnd.hop_size(hop_size);
        SynthSnd.max_partials(frame_size);
        SynthSnd.reset();

        SynthMQ.sampling_rate(sr);
        SynthMQ.hop_size(hop_size);
        SynthMQ.max_partials(frame_size);
        SynthMQ.reset();
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
