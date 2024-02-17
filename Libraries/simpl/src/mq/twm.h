#ifndef SIMPL_TWM_H
#define SIMPL_TWM_H

#include <map>
#include <vector>

#include "base.h"

namespace simpl {

int best_match(s_sample freq, std::vector<s_sample> candidates);

s_sample twm(Peaks peaks, s_sample f_min = 20.0, s_sample f_max = 3000.0,
             s_sample f_step = 10.0);

} // namespace simpl

#endif
