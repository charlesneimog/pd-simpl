#include "partialtrack.hpp"

// ╭─────────────────────────────────────╮
// │           Process Helpers           │
// ╰─────────────────────────────────────╯
double f2m(double f) { return 12 * log(f / 220) / log(2) + 57; }
double m2f(double m) { return 220 * exp(log(2) * ((m - 57) / 12)); }
double transFreq(double originalFrequency, double cents) {
    return originalFrequency * std::pow(2.0, cents / 1200.0);
}

// ─────────────────────────────────────
void TransParams::transpose(simpl::Peak *Peak) {
    if (Peak != nullptr && tIndex > 0 && Peak->frequency != 0) {
        for (int j = 0; j < tIndex; j++) {
            float variance = tVariation[j];
            float lowFreq = m2f(tMidi[j] - variance * 0.01);
            float highFreq = m2f(tMidi[j] + variance * 0.01);
            if (Peak->frequency >= lowFreq && Peak->frequency <= highFreq) {
                Peak->frequency = m2f(tMidi[j] + (tCents[j] * 0.01));
            }
        }
    }
}

// ─────────────────────────────────────
void TransParams::transposeall(simpl::Peak *Peak) {
    if (Peak != nullptr && all_exec) {
        float midi = f2m(Peak->frequency);
        Peak->frequency = m2f(midi + (all_cents * 0.01));
    }
}

// ─────────────────────────────────────
void TransParams::changeamps(simpl::Peak *Peak) {}

// ─────────────────────────────────────
void TransParams::silence(simpl::Peak *Peak) {
    if (Peak != nullptr && sIndex > 0 && Peak->frequency != 0) {
        for (int j = 0; j < sIndex; j++) {
            float variance = sVariation[j];
            float lowFreq = m2f(sMidi[j] - variance * 0.01);
            float highFreq = m2f(sMidi[j] + variance * 0.01);
            if (Peak->frequency >= lowFreq && Peak->frequency <= highFreq) {
                Peak->amplitude = 0;
            }
        }
    }
}

// ─────────────────────────────────────
void TransParams::expand(simpl::Peak *Peak) {}

// ─────────────────────────────────────
void TransParams::reset() {
    sIndex = 0;
    aIndex = 0;
    tIndex = 0;
}
