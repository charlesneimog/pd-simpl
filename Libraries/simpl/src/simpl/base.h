#ifndef BASE_H
#define BASE_H

#include "stdio.h"
#include "string.h"

#include <string>
#include <vector>

#include "exceptions.h"

namespace simpl {

typedef double s_sample;

// ---------------------------------------------------------------------------
// Peak
//
// A spectral Peak
// ---------------------------------------------------------------------------
class Peak {
  public:
    s_sample amplitude;
    s_sample frequency;
    s_sample phase;
    s_sample bandwidth;

    Peak();
    Peak(s_sample new_amplitude, s_sample new_frequency, s_sample new_phase,
         s_sample new_bandwidth);
    ~Peak();
    void reset();
};

typedef std::vector<Peak *> Peaks;

// ---------------------------------------------------------------------------
// Frame
//
// Represents a frame of audio information.
// This can be: - raw audio samples
//              - an unordered list of sinusoidal peaks
//              - an ordered list of partials
//              - synthesised audio samples
//              - residual samples
//              - synthesised residual samples
// ---------------------------------------------------------------------------
class Frame {
  private:
    int _size;
    int _synth_size;
    int _max_peaks;
    int _num_peaks;
    int _max_partials;
    int _num_partials;
    Peaks _peaks;
    Peaks _partials;
    s_sample *_audio;
    s_sample *_synth;
    s_sample *_residual;
    s_sample *_synth_residual;
    void init();
    bool _alloc_memory;
    void create_arrays();
    void destroy_arrays();
    void create_synth_arrays();
    void destroy_synth_arrays();
    void resize_peaks(int new_num_peaks);
    void resize_partials(int new_num_partials);

  public:
    Frame();
    Frame(int frame_size, bool alloc_memory = false);
    ~Frame();
    void clear();
    void clear_peaks();
    void clear_partials();
    void clear_synth();

    // peaks
    int num_peaks();
    void num_peaks(int new_num_peaks);
    int max_peaks();
    void max_peaks(int new_max_peaks);
    void add_peak(s_sample amplitude, s_sample frequency, s_sample phase,
                  s_sample bandwidth);
    Peak *peak(int peak_number);
    void peak(int peak_number, s_sample amplitude, s_sample frequency,
              s_sample phase, s_sample bandwidth);

    // partials
    int num_partials();
    void num_partials(int new_num_partials);
    int max_partials();
    void max_partials(int new_max_partials);
    void add_partial(s_sample amplitude, s_sample frequency, s_sample phase,
                     s_sample bandwidth);
    Peak *partial(int partial_number);
    void partial(int partial_number, s_sample amplitude, s_sample frequency,
                 s_sample phase, s_sample bandwidth);

    // audio buffers
    int size();
    void size(int new_size);
    int synth_size();
    void synth_size(int new_size);
    void audio(s_sample *new_audio);
    void audio(s_sample *new_audio, int size);
    s_sample *audio();
    void synth(s_sample *new_synth);
    void synth(s_sample *new_synth, int size);
    s_sample *synth();
    void residual(s_sample *new_residual);
    void residual(s_sample *new_residual, int size);
    s_sample *residual();
    void synth_residual(s_sample *new_synth_residual);
    void synth_residual(s_sample *new_synth_residual, int size);
    s_sample *synth_residual();
};

typedef std::vector<Frame *> Frames;

} // end of namespace simpl

#endif
