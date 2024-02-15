// clang-format off

#include <m_pd.h>
#include <simpl.h>
#include <thread>

#define SIMPL_SIGTOTAL(s) ((t_int)((s)->s_length * (s)->s_nchans))

// ╭─────────────────────────────────────╮
// │           Peak Detection            │
// ╰─────────────────────────────────────╯
void mqpeaks_tilde_setup(); 


// ╭─────────────────────────────────────╮
// │               Helpers               │
// ╰─────────────────────────────────────╯
void getAmps_setup();
void getFreqs_setup();


