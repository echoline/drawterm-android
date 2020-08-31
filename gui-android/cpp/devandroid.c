#include	"u.h"
#include	"lib.h"
#include	"dat.h"
#include	"fns.h"
#include	"error.h"

#include <stdbool.h>

#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraManager.h>
#include <media/NdkImageReader.h>
#include <android/log.h>

enum
{
    Qdir		= 0,
    Qcam,
};
#define QID(p, c, y) 	(((p)<<16) | ((c)<<4) | (y))

typedef struct {
    uchar *data;
    int len;
} CAux;

static int Ncameras = 0;
static ACameraDevice **devices = NULL;
static AImageReader **readers = NULL;
static ANativeWindow **windows = NULL;
static ACameraCaptureSession **sessions = NULL;
static ACaptureRequest **requests = NULL;
static AImage **images = NULL;
static CAux **datas = NULL;
static ACameraManager *manager;
static ACaptureSessionOutputContainer **container = NULL;
static ACaptureSessionOutput **output = NULL;
static ACameraOutputTarget **target = NULL;

static void androidinit(void);

static void
reinitialize()
{
    int i;
    for (i = 0; i < Ncameras; i++) {
        ACaptureSessionOutputContainer_free(container[i]);
        ACaptureSessionOutput_free(output[i]);
        ACameraCaptureSession_close(sessions[i]);
        ACameraOutputTarget_free(target[i]);
        ACaptureRequest_free(requests[i]);
        AImageReader_delete(readers[i]);
        AImage_delete(images[i]);
        if ((int)datas[i] > Ncameras) {
            free(datas[i]->data);
            free(datas[i]);
        }
        ACameraDevice_close(devices[i]);
    }
    ACameraManager_delete(manager);

    free(devices);
    free(readers);
    free(windows);
    free(sessions);
    free(requests);
    free(images);
    free(datas);
    free(container);
    free(output);
    free(target);

    androidinit();
}

static void
onDisconnected(void *context, ACameraDevice *device1)
{
}

static void
onError(void *context, ACameraDevice *device1, int error)
{
    reinitialize();
}

static void
onImageAvailable(void *context, AImageReader *reader)
{
    int i = (int)context;
    uchar *data;
    int len;
    CAux *aux;

//    qlock(&androidlk);
    __android_log_print(ANDROID_LOG_VERBOSE, "DRAWTERM", "onImageAvailable");
    if (images[i] != NULL)
        AImage_delete(images[i]);
    AImageReader_acquireLatestImage(reader, &images[i]);
    aux = malloc(sizeof(CAux));
//    qlock(&androidlk);
    AImage_getPlaneData(images[i], 0, &data, &len);
    aux->data = malloc(len);
    memcpy(aux->data, data, len);
    aux->len = len;
    if (datas[i] != NULL) {
        free(datas[i]->data);
        free(datas[i]);
    }
    datas[i] = aux;
//    qunlock(&androidlk);
}


static void
androidinit(void)
{
    ACameraDevice_StateCallbacks SCBs = {
            .context = NULL,
            .onDisconnected = onDisconnected,
            .onError = onError,
    };
    ACameraIdList *cameras = NULL;
    ACameraCaptureSession_stateCallbacks CSSCBs = {
            .context = NULL,
    };

    manager = ACameraManager_create();
    ACameraManager_getCameraIdList(manager, &cameras);
    Ncameras = cameras->numCameras;
    devices = malloc(sizeof(ACameraDevice*) * Ncameras);
    readers = malloc(sizeof(AImageReader*) * Ncameras);
    windows = malloc(sizeof(ANativeWindow*) * Ncameras);
    sessions = malloc(sizeof(ACameraCaptureSession*) * Ncameras);
    requests = malloc(sizeof(ACaptureRequest*) * Ncameras);
    images = malloc(sizeof(AImage*) * Ncameras);
    datas = malloc(sizeof(CAux*) * Ncameras);
    container = malloc(sizeof(ACaptureSessionOutputContainer*) * Ncameras);
    output = malloc(sizeof(ACaptureSessionOutput*) * Ncameras);
    target = malloc(sizeof(ACameraOutputTarget*) * Ncameras);
    for (int i = 0; i < Ncameras; i++) {
        images[i] = NULL;
        AImageReader_ImageListener ALs = {
                .context = (void*)i,
                .onImageAvailable = onImageAvailable,
        };
        ACameraManager_openCamera(manager, cameras->cameraIds[i], &SCBs, &devices[i]);
        AImageReader_new(640, 480, AIMAGE_FORMAT_JPEG, 4, &readers[i]);
        AImageReader_setImageListener(readers[i], &ALs);
        AImageReader_getWindow(readers[i], &windows[i]);
        ACaptureSessionOutput_create(windows[i], &output[i]);
        ACaptureSessionOutputContainer_create(&container[i]);
        ACaptureSessionOutputContainer_add(container[i], output[i]);
        ACameraDevice_createCaptureSession(devices[i], container[i], &CSSCBs, &sessions[i]);
        ACameraOutputTarget_create(windows[i], &target[i]);
        ACameraDevice_createCaptureRequest(devices[i], TEMPLATE_ZERO_SHUTTER_LAG, &requests[i]);
        ACaptureRequest_addTarget(requests[i], target[i]);
    }
    ACameraManager_deleteCameraIdList(cameras);
}

static Chan*
androidattach(char *param)
{
    Chan *c;

    c = devattach('N', param);
    c->qid.path = QID(0, 0, Qdir);
    c->qid.type = QTDIR;
    c->qid.vers = 0;

    return c;
}

static int
androidgen(Chan *c, char *n, Dirtab *d, int nd, int s, Dir *dp)
{
    Qid q;

    if (s == DEVDOTDOT) {
        mkqid(&q, Qdir, 0, QTDIR);
        devdir(c, q, "#N", 0, eve, 0555, dp);
        return 1;
    }
    if (s < Ncameras) {
        sprintf(up->genbuf, "cam%d.jpg", s);
        mkqid(&q, (s << 4) | Qcam, 0, QTFILE);
        devdir(c, q, up->genbuf, 0, eve, 0444, dp);
        return 1;
    }
    return -1;
}

static Walkqid*
androidwalk(Chan *c, Chan *nc, char **name, int nname)
{
    return devwalk(c, nc, name, nname, 0, 0, androidgen);
}

static int
androidstat(Chan *c, uchar *db, int n)
{
    return devstat(c, db, n, 0, 0, androidgen);
}

static void
onCaptureCompleted(void *context, ACameraCaptureSession *session,
                   ACaptureRequest *request, const ACameraMetadata *result)
{
}

static void
onCaptureFailed(void *context, ACameraCaptureSession *session,
                ACaptureRequest *request, ACameraCaptureFailure *failure)
{
    reinitialize();
}

static Chan*
androidopen(Chan *c, int omode)
{
    p9_uvlong s;
    int i;

    c = devopen(c, omode, 0, 0, androidgen);

    ACameraCaptureSession_captureCallbacks CBs = {
            .context = (void*)c,
            .onCaptureCompleted = onCaptureCompleted,
            .onCaptureFailed = onCaptureFailed,
    };

    if (c->qid.path & Qcam) {
        s = c->qid.path >> 4;
        c->aux = (void*)s;
        ACameraCaptureSession_capture(sessions[s], &CBs, 1, &requests[s], &i);
//        while(images[i] == NULL)
  //          usleep(10 * 1000);
    }
    c->mode = openmode(omode);
    c->flag |= COPEN;
    c->offset = 0;
    c->iounit = 8192;

    return c;
}

static void
androidclose(Chan *c)
{
    if (datas[(int)c->aux] == NULL)
        return;
    CAux *aux = datas[(int)c->aux];
    free(aux->data);
    free(aux);
    datas[(int)c->aux] = NULL;
}

static long
androidread(Chan *c, void *v, long n, vlong off)
{
    char *a = v;
    CAux *aux;
    long l;
    int i = c->qid.path >> 4;

    switch((ulong)c->qid.path & 0xF) {
        default:
            error(Eperm);
            return -1;

        case Qcam:
            while (datas[i] == NULL)
                usleep(10 * 1000);
            aux = datas[i];
            l = aux->len - off;
            if (l == 0) {
                free(aux->data);
                free(aux);
                datas[i] = NULL;
                return l;
            }
            if (l > n)
                l = n;
            memcpy(a, &aux->data[off], l);
            return l;
        case Qdir:
            return devdirread(c, a, n, 0, 0, androidgen);
    }
}

static long
androidwrite(Chan *c, void *vp, long n, vlong off)
{
    error(Eperm);
    return -1;
}

Dev androiddevtab = {
        'N',
        "android",

        devreset,
        androidinit,
        devshutdown,
        androidattach,
        androidwalk,
        androidstat,
        androidopen,
        devcreate,
        androidclose,
        androidread,
        devbread,
        androidwrite,
        devbwrite,
        devremove,
        devwstat,
};
