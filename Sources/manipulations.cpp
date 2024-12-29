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
        for (int i = 0; i < tIndex; i++) {
            if (Peak->frequency >= tCenterFreq[i] - tVariation[i] &&
                Peak->frequency <= tCenterFreq[i] + tVariation[i]) {
                float newFreq = transFreq(Peak->frequency, tCents[i]);
                Peak->frequency = newFreq;
            }
        }
    }
}

// ─────────────────────────────────────
void TransParams::changeamps(simpl::Peak *Peak) {
    if (Peak != nullptr) {
        if (Peak->frequency == 0) {
            return;
        }
        for (int i = 0; i < aIndex; i++) {
            if (Peak->frequency >= aCenterFreq[i] - aVariation[i] &&
                Peak->frequency <= aCenterFreq[i] + aVariation[i]) {
                float newFreq = transFreq(Peak->frequency, tCents[i]);
                Peak->amplitude *= aAmpsFactor[i];
            }
        }
    }
}

// ─────────────────────────────────────
void TransParams::silence(simpl::Peak *Peak) {
    for (int j = 0; j < sIndex; j++) {
        float variance = sVariation[j];
        float lowFreq = m2f(sMidi[j] - variance * 0.01);
        float highFreq = m2f(sMidi[j] + variance * 0.01);
        if (Peak->frequency >= lowFreq && Peak->frequency <= highFreq) {
            Peak->amplitude = 0;
        }
    }
}

// ─────────────────────────────────────
void TransParams::expand(simpl::Peak *Peak) {
    if (Peak != nullptr) {
        if (Peak->frequency == 0 || ePrevFreq == 0) {
            return;
        }
        float d = (Peak->frequency - ePrevFreq) * eFactor;
        float newFreq = m2f(f2m(ePrevFreq) + d);
        Peak->frequency = m2f(f2m(ePrevFreq) + d);
    }
}

// ─────────────────────────────────────
void TransParams::reset() {
    sIndex = 0;
    aIndex = 0;
    tIndex = 0;
}
