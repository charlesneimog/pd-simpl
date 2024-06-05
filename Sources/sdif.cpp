#include "partialtrack.hpp"

static t_class *SdifObj;

// ==============================================
typedef struct _SdifObj {
    t_object xObj;
    std::string path;
    t_outlet *outlet;
    t_canvas *canvas;

} t_SdifObj;

// ==============================================
static void WriteSdif(t_SdifObj *x, t_symbol *p) {

    AnalysisData *Anal = getAnalisysPtr(p);

    if (Anal == nullptr) {
        pd_error(NULL, "[pt.sdif] Pointer not found");
        return;
    }

    std::string path = x->path + "/filename2.sdif";

    SdifFileT *File = SdifFOpen(path.c_str(), eWriteFile);
    if (!File) {
        pd_error(x, "Error opening file %s", path.c_str());
        return;
    }

    SdifFWriteGeneralHeader(File);
    SdifFWriteAllASCIIChunks(File);
    SdifSignature Signature = SdifStringToSignature("1TRC");
    SdifUInt4 streamID = 1;

    // While Loop
    int nrows = 1;
    int ncols = 4;

    float partial[4];
    double time;

    for (int j = 0; j < Anal->Frames.size(); j++) {
        simpl::Frame *Frame = Anal->Frames[j];
        for (int i = 0; i < Frame->num_partials(); i++) {
            simpl::Peak *Peak = Frame->partial(i);
            if (Peak != nullptr) {
                time = 1 / sys_getsr() * j * Anal->HopSize;
                partial[0] = i;
                partial[1] = Peak->frequency;
                partial[2] = Peak->amplitude;
                partial[3] = Peak->phase;
                SdifFWriteFrameAndOneMatrix(File, Signature, streamID, time,
                                            Signature, eFloat4, nrows, ncols,
                                            partial);
            }
        }
    }

    // Close
    SdifFClose(File);

    return;
}

// ==============================================
static void *NewSdifObj(t_symbol *s, int argc, t_atom *argv) {
    t_SdifObj *x = (t_SdifObj *)pd_new(SdifObj);

    x->canvas = canvas_getcurrent();
    x->path = canvas_getdir(x->canvas)->s_name;

    SdifGenInit("");

    return x;
}

// ==============================================
void SdifObjSetup(void) {
    SdifObj = class_new(gensym("pt-sdif"), (t_newmethod)NewSdifObj, NULL,
                        sizeof(t_SdifObj), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(SdifObj, (t_method)WriteSdif, gensym("PtObjFrames"),
                    A_SYMBOL, 0);
}
