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
#include <android/sensor.h>

enum
{
	Qdir		= 0,
	Qcam		= 1,
	Qaccel		= 2,
	Qcompass	= 4,
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
static ACameraIdList *cameras = NULL;
static ASensorManager *sensorManager = NULL;

static void androidinit(void);

static void
reinitialize()
{
	ACameraManager_deleteCameraIdList(cameras);
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

	__android_log_print(ANDROID_LOG_VERBOSE, "drawterm", "onImageAvailable");
	if (images[i] != NULL)
		AImage_delete(images[i]);
	AImageReader_acquireLatestImage(reader, &images[i]);
	aux = malloc(sizeof(CAux));
	AImage_getPlaneData(images[i], 0, &data, &len);
	aux->data = malloc(len);
	memcpy(aux->data, data, len);
	aux->len = len;
	if (datas[i] != NULL) {
		free(datas[i]->data);
		free(datas[i]);
	}
	datas[i] = aux;
}


static void
androidinit(void)
{
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

	sensorManager = ASensorManager_getInstanceForPackage("org.echoline.drawterm");
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
	if (s == Ncameras) {
		sprintf(up->genbuf, "accel");
		mkqid(&q, Qaccel, 0, QTFILE);
		devdir(c, q, up->genbuf, 0, eve, 0444, dp);
		return 1;
	}
	if (s == (Ncameras+1)) {
		sprintf(up->genbuf, "compass");
		mkqid(&q, Qcompass, 0, QTFILE);
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
	ACameraDevice_StateCallbacks SCBs = {
		.context = NULL,
		.onDisconnected = onDisconnected,
		.onError = onError,
	};
	ACameraCaptureSession_stateCallbacks CSSCBs = {
		.context = NULL,
	};
	AImageReader_ImageListener ALs = {
		.onImageAvailable = onImageAvailable,
	};

	if (c->qid.path & Qcam) {
		s = c->qid.path >> 4;
		c->aux = (void*)s;
		images[s] = NULL;
		ACameraManager_openCamera(manager, cameras->cameraIds[s], &SCBs, &devices[s]);
		AImageReader_new(640, 480, AIMAGE_FORMAT_JPEG, 4, &readers[s]);
		ALs.context = (void*)s;
		AImageReader_setImageListener(readers[s], &ALs);
		AImageReader_getWindow(readers[s], &windows[s]);
		ACaptureSessionOutput_create(windows[s], &output[s]);
		ACaptureSessionOutputContainer_create(&container[s]);
		ACaptureSessionOutputContainer_add(container[s], output[s]);
		ACameraDevice_createCaptureSession(devices[s], container[s], &CSSCBs, &sessions[s]);
		ACameraOutputTarget_create(windows[s], &target[s]);
		ACameraDevice_createCaptureRequest(devices[s], TEMPLATE_STILL_CAPTURE, &requests[s]);
		ACaptureRequest_addTarget(requests[s], target[s]);
		ACameraCaptureSession_capture(sessions[s], &CBs, 1, &requests[s], &i);
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
	int i;
	if (c->qid.path & Qcam) {
		i = (int)c->aux;
		if (datas[i] == NULL)
			return;
		CAux *aux = datas[i];
		free(aux->data);
		free(aux);
		datas[i] = NULL;
		ACaptureSessionOutputContainer_free(container[i]);
		ACaptureSessionOutput_free(output[i]);
		ACameraCaptureSession_close(sessions[i]);
		ACameraOutputTarget_free(target[i]);
		ACaptureRequest_free(requests[i]);
		AImageReader_delete(readers[i]);
		AImage_delete(images[i]);
		if (datas[i] != NULL) {
			free(datas[i]->data);
			free(datas[i]);
		}
		ACameraDevice_close(devices[i]);
	}
}

static long
androidread(Chan *c, void *v, long n, vlong off)
{
	char *a = v;
	CAux *aux;
	long l;
	int i = c->qid.path >> 4;
	const ASensor *sensor;
	ASensorEventQueue *queue = NULL;
	ASensorEvent data;

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
		case Qaccel:
			queue = ASensorManager_createEventQueue(sensorManager, ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS), 1, NULL, NULL);
			if (queue == NULL)
				return 0;
			sensor = ASensorManager_getDefaultSensor(sensorManager, ASENSOR_TYPE_ACCELEROMETER_UNCALIBRATED);
			if (sensor == NULL)
				return 0;
			if (ASensorEventQueue_enableSensor(queue, sensor))
				return 0;
			l = 0;
			if (ALooper_pollAll(1000, NULL, NULL, NULL) == 1) {
				if (ASensorEventQueue_getEvents(queue, &data, 1)) {
					l = snprint(a, n, "%11f %11f %11f\n", data.acceleration.x, data.acceleration.y, data.acceleration.z);
				}
			}
			ASensorEventQueue_disableSensor(queue, sensor);
			ASensorManager_destroyEventQueue(sensorManager, queue);
			return l;
		case Qcompass:
			queue = ASensorManager_createEventQueue(sensorManager, ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS), 1, NULL, NULL);
			if (queue == NULL)
				return 0;
			sensor = ASensorManager_getDefaultSensor(sensorManager, ASENSOR_TYPE_MAGNETIC_FIELD);
			if (sensor == NULL)
				return 0;
			if (ASensorEventQueue_enableSensor(queue, sensor))
				return 0;
			l = 0;
			if (ALooper_pollAll(1000, NULL, NULL, NULL) == 1) {
				if (ASensorEventQueue_getEvents(queue, &data, 1)) {
					l = snprint(a, n, "%11f %11f %11f\n", data.magnetic.x, data.magnetic.y, data.magnetic.z);
				}
			}
			ASensorEventQueue_disableSensor(queue, sensor);
			ASensorManager_destroyEventQueue(sensorManager, queue);
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
