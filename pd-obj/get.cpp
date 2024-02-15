#include "pd-simpl.hpp"

static t_class *getAmpClass;
static t_class *getFreqClass;

// ==============================================
typedef struct __getAmp { // It seems that all the objects are some kind of
                          // class.
    t_object xObj;
    t_outlet *sigOut;

} t_getAmp;

// ==============================================
typedef struct __getFrq {
    t_object xObj;
    t_outlet *sigOut;
} t_getFrq;

// ==============================================
static void GetAmp(t_getAmp *x, t_gpointer *p) {
    simpl::Frames *frames = (simpl::Frames *)p;
    for (int i = 0; i < frames->size(); i++) {
        simpl::Frame *frame = frames->at(i);
        t_atom *amps = new t_atom[frame->num_peaks()];
        for (int j = 0; j < frame->num_peaks(); j++) {
            if (frame->peak(j) != NULL) {
                float amp = frame->peak(j)->amplitude;
                SETFLOAT(&amps[j], amp);
            }
        }
        outlet_list(x->sigOut, gensym("amps"), frames->size(), amps);
    }
    return;
}

// ==============================================
static void GetFrq(t_getAmp *x, t_gpointer *p) {
    simpl::Frames *frames = (simpl::Frames *)p;
    for (int i = 0; i < frames->size(); i++) {
        simpl::Frame *frame = frames->at(i);
        t_atom *amps = new t_atom[frame->num_peaks()];
        for (int j = 0; j < frame->num_peaks(); j++) {
            if (frame->peak(j) != NULL) {
                float amp = frame->peak(j)->frequency;
                SETFLOAT(&amps[j], amp);
            }
        }
        outlet_list(x->sigOut, gensym("freqs"), frames->size(), amps);
    }
    return;
}

// ==============================================
static void *NewGetAmp(void) {
    t_getAmp *x = (t_getAmp *)pd_new(getAmpClass);
    x->sigOut = outlet_new(&x->xObj, &s_anything);
    return x;
}

// ==============================================
static void *NewGetFrq(void) {
    t_getFrq *x = (t_getFrq *)pd_new(getFreqClass);
    x->sigOut = outlet_new(&x->xObj, &s_anything);
    return x;
}

// ==============================================
void getAmps_setup(void) {
    getAmpClass = class_new(gensym("getAmps"), (t_newmethod)NewGetAmp, NULL,
                            sizeof(t_getAmp), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(getAmpClass, (t_method)GetAmp, gensym("Peaks"), A_POINTER,
                    0);
}

// ==============================================
void getFreqs_setup(void) {
    getFreqClass = class_new(gensym("getFreqs"), (t_newmethod)NewGetFrq, NULL,
                             sizeof(t_getFrq), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(getFreqClass, (t_method)GetFrq, gensym("Peaks"), A_POINTER,
                    0);
}
