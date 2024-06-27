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
static void WriteSdif(t_SdifObj *x, t_symbol *s, int argc, t_atom *argv) {
    std::string path = x->path + "/filename2.sdif";
    post("Writing SDIF file to %s", path.c_str());

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

    time = 0.0;
    partial[0] = 0.0;
    partial[1] = 440;
    partial[2] = 0.3;
    partial[3] = 0.0;
    SdifFWriteFrameAndOneMatrix(File, Signature, streamID, time, Signature,
                                eFloat4, nrows, ncols, partial);
    time = 0.0;
    partial[0] = 0.0;
    partial[1] = 440;
    partial[2] = 0.3;
    partial[3] = 0.0;
    SdifFWriteFrameAndOneMatrix(File, Signature, streamID, time, Signature,
                                eFloat4, nrows, ncols, partial);

    time = 0.023;
    partial[0] = 0.0;
    partial[1] = 440;
    partial[2] = 0.3;
    partial[3] = 0.0;
    SdifFWriteFrameAndOneMatrix(File, Signature, streamID, time, Signature,
                                eFloat4, nrows, ncols, partial);

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

    WriteSdif(x, NULL, 0, NULL);

    return x;
}

// ==============================================
void SdifObjSetup(void) {
    SdifObj = class_new(gensym("pt-sdif"), (t_newmethod)NewSdifObj, NULL,
                        sizeof(t_SdifObj), CLASS_DEFAULT, A_GIMME, 0);
}
