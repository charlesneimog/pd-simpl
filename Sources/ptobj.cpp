#include "pd-partialtrack.hpp"

// ╭─────────────────────────────────────╮
// │      Pointers between Objects       │
// ╰─────────────────────────────────────╯

static t_class *AnalysisPtr;

t_symbol *newAnalisysPtr(int frameSize, int bufferSize) {
    t_PtrPartialAnalysis *x =
        (t_PtrPartialAnalysis *)getbytes(sizeof(t_PtrPartialAnalysis));
    x->x_pd = AnalysisPtr;
    static AnalysisData Anal(frameSize, bufferSize);
    x->x_data = &Anal;
    std::string PointerStr = std::to_string((long long)x->x_data);
    x->x_sym = gensym(PointerStr.c_str());
    pd_bind((t_pd *)x, x->x_sym);
    return x->x_sym;
}

void killAnalisysPtr(t_PtrPartialAnalysis *x) {
    pd_unbind((t_pd *)x, x->x_sym);
}

AnalysisData *getAnalisysPtr(t_symbol *s) {
    t_PtrPartialAnalysis *x =
        (t_PtrPartialAnalysis *)pd_findbyclass(s, AnalysisPtr);
    return x ? x->x_data : 0;
}
