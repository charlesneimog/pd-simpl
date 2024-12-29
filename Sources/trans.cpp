#include "partialtrack.hpp"

static t_class *trans_class;

typedef struct _Trans {
    t_object xObj;
    TransParams *Params;
    t_outlet *out;

} t_Trans;

// ─────────────────────────────────────
static void trans_setamp(t_Trans *x, t_float centerFreq, t_float variation,
                         t_float amp) {
    // args: centerFreq, cents variation, cents to transpose
    for (int i = 0; i < x->Params->aIndex; i++) {
        if (centerFreq == x->Params->aCenterFreq[i]) {
            x->Params->aAmpsFactor[i] = amp;
            return;
        }
    }

    if (x->Params->aIndex < MAX_SILENCED_PARTIALS) {
        x->Params->aCenterFreq[x->Params->aIndex] = centerFreq;
        x->Params->aVariation[x->Params->aIndex] = variation;
        x->Params->aAmpsFactor[x->Params->aIndex] = amp;
        x->Params->aIndex++;
    } else {
        pd_error(NULL, "[peaks~] Maximum number of silenced partials reached");
    }
}

// ─────────────────────────────────────
static void trans_setsilence(t_Trans *x, t_float centerFreq,
                             t_float variation) {
    if (x->Params->sIndex < MAX_SILENCED_PARTIALS) {
        x->Params->sLowNote[x->Params->sIndex] = centerFreq;
        x->Params->sHighNote[x->Params->sIndex] = centerFreq;
        x->Params->sVariation[x->Params->sIndex] = variation;
        x->Params->sIndex++;
    } else {
        pd_error(NULL, "[peaks~] Maximum number of silenced partials reached");
    }
}

// ─────────────────────────────────────
static void trans_processoffline(t_Trans *x, t_symbol *p) {
    AnalysisData *Anal = getAnalisysPtr(p);
    if (Anal == nullptr) {
        pd_error(NULL, "[trans] Pointer not found");
        return;
    }

    simpl::Frames Frames = Anal->Frames;
    for (int i = 0; i < Frames.size(); i++) {
        simpl::Frame *Frame = Frames[i];
        for (int i = 0; i < Frame->num_partials(); i++) {
            if (x->Params->sIndex != 0)
                silencepartial(x->Params, Frame, i);
            if (x->Params->tIndex != 0)
                transpose(x->Params, Frame, i);
            // if (x->Params->eFactor != 1.0)
            // trans_expand(x->Params, Frame, i);
            if (x->Params->aIndex != 0)
                changeamps(x->Params, Frame, i);
        }
    }

    t_atom args[1];
    SETSYMBOL(&args[0], p);
    outlet_anything(x->out, gensym("PtObjFrames"), 1, args);

    return;
}

// ─────────────────────────────────────
static void trans_process(t_Trans *x, t_symbol *p) {
    AnalysisData *Anal = getAnalisysPtr(p);
    if (Anal == nullptr) {
        pd_error(NULL, "[trans] Pointer not found");
        return;
    }

    if (Anal->offline) {
        pd_error(nullptr, "[trans] Offline mode");
        return;
    }

    for (int i = 0; i < Anal->Frame.num_partials(); i++) {
        if (x->Params->sIndex != 0)
            silencepartial(x->Params, &Anal->Frame, i);
        if (x->Params->tIndex != 0)
            transpose(x->Params, &Anal->Frame, i);
        // if (x->eFactor != 1.0)
        //     expand(x->Params, &Anal->Frame, i);
        if (x->Params->aIndex != 0)
            changeamps(x->Params, &Anal->Frame, i);
    }

    t_atom args[1];
    char ptr[MAXPDSTRING];
    SETSYMBOL(&args[0], p);
    outlet_anything(x->out, gensym("PtObj"), 1, args);
}

// ─────────────────────────────────────
static void *trans_new(t_symbol *synth, int argc, t_atom *argv) {
    t_Trans *x = (t_Trans *)pd_new(trans_class);
    x->out = outlet_new(&x->xObj, &s_anything);
    x->Params = new TransParams;
    DEBUG_PRINT("[synth~] New Synth");
    return x;
}

// ─────────────────────────────────────
static void trans_free(t_Trans *x) {
    delete x->Params;
    return;
}

// ─────────────────────────────────────
extern "C" void trans_setup(void) {
    trans_class =
        class_new(gensym("trans"), (t_newmethod)trans_new, (t_method)trans_free,
                  sizeof(t_Trans), CLASS_DEFAULT, A_GIMME, 0);

    class_addmethod(trans_class, (t_method)trans_setsilence, gensym("silence"),
                    A_FLOAT, A_FLOAT, 0);
    // class_addmethod(trans_class, (t_method)trans_settransposedpartial,
    //                 gensym("trans"), A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(trans_class, (t_method)trans_setamp, gensym("amps"),
                    A_FLOAT, A_FLOAT, A_FLOAT, 0);
    // class_addmethod(trans_class, (t_method)trans_reset, gensym("reset"),
    // A_NULL,
    //                 0);
    class_addmethod(trans_class, (t_method)trans_process, gensym("PtObj"),
                    A_SYMBOL, 0);
    class_addmethod(trans_class, (t_method)trans_processoffline,
                    gensym("PtObjFrames"), A_SYMBOL, 0);
}
