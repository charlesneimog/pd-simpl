#include "pd-simpl.hpp"

#if defined(_LANGUAGE_C_PLUS_PLUS) || defined(__cplusplus)
extern "C" {
void simpl_setup(void);
}
#endif

// ==============================================
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
        pd_error(NULL, "[simpl] Unknown Peak Detection method");
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
        pd_error(NULL, "[simpl] Unknown PartialTracking method");
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
        pd_error(NULL, "[simpl] Unknown Synth method");
        error = true;
    }
    if (residual) {
        Residual.residual_frame(&Frame);
    }
}

// ==============================================
void AnalysisData::set_max_peaks(int max_peaks) {
    max_peaks = max_peaks;
    Frame.max_peaks(max_peaks);
    Frame.max_partials(max_peaks);
}

// ==============================================
void simpl_setup(void) {

    int major, minor, micro;
    sys_getversion(&major, &minor, &micro);
    if (major < 0 && minor < 54) {
        pd_error(NULL, "[simpl] You need to use Pd 0.54 or higher");
        return;
    }

    post("");
    post("[simpl] by John Glover");
    post("[simpl] ported by Charles K. Neimog");
    post("[simpl] simpl version %d.%d.%d", 0, 0, 1);
    post("");

    // Global
    s_peaks_tilde_setup();
    s_synth_tilde_setup();
    s_get_setup();
    s_create_setup();
    s_trans_setup();

    // SimplPartial
    s_spt_tilde_setup();
    s_ss_tilde();
}
