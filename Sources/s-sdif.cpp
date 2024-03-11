#include "pd-simpl.hpp"

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <loris.h>
#pragma GCC diagnostic warning "-Wdeprecated-declarations"

static t_class *sdif_class;
#define MAX_SILENCED_PARTIALS 127

// ==============================================
typedef struct _Sdif {
    t_object xObj;
    t_sample xSample;

    t_outlet *outlet;

} t_Sdif;

// ==============================================
static void LorisAnalyzeExport(t_Sdif *x, t_symbol *s) {
    int vecsize;
    t_word *vec;
    const double FUNDAMENTAL = 415.0;
    const unsigned long BUFSZ = 44100 * 4;

    Loris::PartialList *partials = NULL;
    double sr = sys_getsr();
    unsigned int nsamps;

    /* import the clarinet samples */
    t_garray *tab = (t_garray *)pd_findbyclass(gensym("sound"), garray_class);
    garray_getfloatwords(tab, &vecsize, &vec);

    //
    double samples[vecsize];
    for (int i = 0; i < vecsize; i++) {
        samples[i] = (double)vec[i].w_float;
    }

    analyzer_configure(.8 * FUNDAMENTAL, FUNDAMENTAL);
    analyzer_setFreqDrift(.2 * FUNDAMENTAL);
    partials = createPartialList();
    analyze(samples, vecsize, sr, partials);
    // exportSdif("/home/neimog/Documents/Git/pd-simpl/simpl-help.sdif",
    // partials); post("ok");

    return;
}

// ==============================================
static void *NewSdif(t_symbol *s, int argc, t_atom *argv) {
    t_Sdif *x = (t_Sdif *)pd_new(sdif_class);
    x->outlet = outlet_new(&x->xObj, &s_anything);
    return (void *)x;
}

// ==============================================
void s_sdif_setup(void) {
    sdif_class = class_new(gensym("s-sdif"), (t_newmethod)NewSdif, NULL,
                           sizeof(t_Sdif), CLASS_DEFAULT, A_GIMME, 0);

    class_addmethod(sdif_class, (t_method)LorisAnalyzeExport, gensym("export"), 
                    A_SYMBOL,  0);
}
