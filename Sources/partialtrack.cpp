#include "partialtrack.hpp"

#if defined(_LANGUAGE_C_PLUS_PLUS) || defined(__cplusplus)
extern "C" {
void partialtrack_setup(void);
}
#endif

// ╭─────────────────────────────────────╮
// │         AnalysisData Method         │
// ╰─────────────────────────────────────╯
void AnalysisData::PeakDectection() {
    if (error) {
        return;
    }
    if (PdMethod == "loris") {
        PdLoris.find_peaks_in_frame(&Frame);
    } else if (PdMethod == "sndobj") {
        PdSnd.find_peaks_in_frame(&Frame);
    } else if (PdMethod == "sms") {
        PdSMS.find_peaks_in_frame(&Frame);
    } else if (PdMethod == "mq") {
        PdMQ.find_peaks_in_frame(&Frame);
    } else {
        pd_error(NULL, "[partialtrack] Unknown Peak Detection method");
        error = true;
    }
}

// ==============================================
void AnalysisData::PeakDectectionFrames(int audioSize, double *audio) {
    if (error) {
        return;
    }

    if (PdMethod == "loris") {
        Frames = PdLoris.find_peaks(audioSize, audio);
    } else if (PdMethod == "sndobj") {
        Frames = PdSnd.find_peaks(audioSize, audio);
    } else if (PdMethod == "sms") {
        Frames = PdSMS.find_peaks(audioSize, audio);
    } else if (PdMethod == "mq") {
        Frames = PdMQ.find_peaks(audioSize, audio);
    } else {
        pd_error(NULL, "[partialtrack] Unknown Peak Detection method");
        error = true;
    }
}

// ==============================================
void AnalysisData::PartialTracking() {
    if (error) {
        return;
    }

    if (PtMethod == "loris") {
        PtLoris.update_partials(&Frame);
    } else if (PtMethod == "sndobj") {
        PtSnd.update_partials(&Frame);
    } else if (PtMethod == "sms") {
        PtSMS.update_partials(&Frame);
    } else if (PtMethod == "mq") {
        PtMQ.update_partials(&Frame);
    } else {
        pd_error(NULL, "[partialtrack] Unknown PartialTracking method");
        error = true;
    }
}

// ==============================================
void AnalysisData::PartialTrackingFrames() {
    if (error) {
        return;
    }

    if (Frames.size() == 0) {
        pd_error(NULL, "[partialtrack] No frames to track");
        error = true;
        return;
    }

    if (PtMethod == "loris") {
        Frames = PtLoris.find_partials(Frames);
    } else if (PtMethod == "sndobj") {
        Frames = PtSnd.find_partials(Frames);
    } else if (PtMethod == "sms") {
        Frames = PtSMS.find_partials(Frames);
    } else if (PtMethod == "mq") {
        Frames = PtMQ.find_partials(Frames);
    } else {
        pd_error(NULL, "[partialtrack] Unknown PartialTracking method");
        error = true;
    }
}

// ==============================================
void AnalysisData::Synth() {
    if (error) {
        return;
    }
    if (SyMethod == "loris") {
        SynthLoris.synth_frame(&Frame);
    } else if (SyMethod == "sndobj") {
        SynthSnd.synth_frame(&Frame);
    } else if (SyMethod == "sms") {
        SynthSMS.synth_frame(&Frame);
    } else if (SyMethod == "mq") {
        SynthMQ.synth_frame(&Frame);
    } else {
        pd_error(NULL, "[partialtrack] Unknown Synth method");
        error = true;
    }
    if (residual) {
        Residual.residual_frame(&Frame);
    }
}

// ==============================================
void AnalysisData::SynthFrames() {
    if (error) {
        return;
    }

    if (SyMethod == "loris") {
        SynthLoris.synth(Frames);
    } else if (SyMethod == "sndobj") {
        SynthSnd.synth(Frames);
    } else if (SyMethod == "sms") {
        SynthSMS.synth(Frames);
    } else if (SyMethod == "mq") {
        SynthMQ.synth(Frames);
    } else {
        pd_error(NULL, "[partialtrack] Unknown Synth method");
        error = true;
        return;
    }
}

// ==============================================
void AnalysisData::set_max_peaks(int max_peaks) {
    max_peaks = max_peaks;
    Frame.max_peaks(max_peaks);
    Frame.max_partials(max_peaks);
}

// ==============================================
void partialtrack_setup(void) {

    int major, minor, micro;
    sys_getversion(&major, &minor, &micro);
    if (major < 0 && minor < 54) {
        pd_error(NULL, "[partialtrack] You need to use Pd 0.54 or higher");
        return;
    }

#ifndef NEIMOG_LIBRARY
    post("");
    post("[partialtrack] by Charles K. Neimog");
    post("[partialtrack] it uses simpl by John Glover");
    post("[partialtrack] version %d.%d", 0, 2);
    post("");
#endif

    PeakSetup();
    SynthSetup();
    TransformationsSetup();
    // SimpleSynthSetup();
}
