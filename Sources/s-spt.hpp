#include <m_pd.h>
#include <math.h>
#include <mutex>
#include <vector>

#ifndef SPT_HPP
#define SPT_HPP

#define SPT_SIGTOTAL(s) ((t_int)((s)->s_length * (s)->s_nchans))

const double Pi = 3.14159265358979324;
const double TwoPi = 2 * Pi;

struct Peak {
    unsigned int index;

    float Freq;
    float Mag;
    float Phase;

    float prevFreq;
    float prevMag;
    float prevPhase;

    bool prevPartial;

    Peak *previous;
};

typedef std::vector<Peak *> Peaks;

struct PartialTracking {
    Peaks *Partials;
    float sampleRate;
    float FrameSize;
    unsigned int partialCount;
    std::mutex mtx;
};

#endif
