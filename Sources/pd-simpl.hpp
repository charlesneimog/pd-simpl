#include <m_pd.h>
#include <thread>

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <simpl.h> // TODO: Someday I will need to fix this
#pragma GCC diagnostic warning "-Wdeprecated-declarations"

#define SIMPL_SIGTOTAL(s) ((t_int)((s)->s_length * (s)->s_nchans))

// ╭─────────────────────────────────────╮
// │            Main Objects             │
// ╰─────────────────────────────────────╯
void s_peaks_tilde_setup();
void s_partials_setup();
void s_synth_tilde_setup();
void s_test_tilde_setup();

// ╭─────────────────────────────────────╮
// │                 Get                 │
// ╰─────────────────────────────────────╯
void s_get_setup();

// ╭─────────────────────────────────────╮
// │               Create                │
// ╰─────────────────────────────────────╯
void s_create_setup();
