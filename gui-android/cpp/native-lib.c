#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/log.h>
#include <u.h>
#include <libc.h>
#include <draw.h>
#include <string.h>

void absmousetrack(int, int, int, ulong);
ulong ticks(void);
int dt_main(int, char**);
int screenWidth;
int screenHeight;
Point mousept = {0, 0};
int buttons = 0;
float ws = 1;
float hs = 1;
extern char *snarfbuf;
int mPaused = 0;
ANativeWindow *window = NULL;
jobject mainActivityObj;
JavaVM *jvm;
void flushmemscreen(Rectangle r);

JNIEXPORT void JNICALL
Java_org_echoline_drawterm_MainActivity_setObject(
        JNIEnv *env,
        jobject obj) {
    mainActivityObj = (*env)->NewGlobalRef(env, obj);
    jint rs = (*env)->GetJavaVM(env, &jvm);
    assert(rs == JNI_OK);
}

JNIEXPORT void JNICALL
Java_org_echoline_drawterm_MainActivity_setPass(
        JNIEnv *env,
        jobject obj,
        jstring str) {
    setenv("PASS", (char*)(*env)->GetStringUTFChars(env, str, 0), 1);
}

JNIEXPORT void JNICALL
Java_org_echoline_drawterm_MainActivity_setWidth(
        JNIEnv *env,
        jobject obj,
        jint width) {
    screenWidth = width;
}

JNIEXPORT void JNICALL
Java_org_echoline_drawterm_MainActivity_setHeight(
        JNIEnv *env,
        jobject obj,
        jint height) {
    screenHeight = height;
}

JNIEXPORT void JNICALL
Java_org_echoline_drawterm_MainActivity_setWidthScale(
        JNIEnv *env,
        jobject obj,
        jfloat s) {
    ws = s;
}

JNIEXPORT void JNICALL
Java_org_echoline_drawterm_MainActivity_setHeightScale(
        JNIEnv *env,
        jobject obj,
        jfloat s) {
    hs = s;
}

JNIEXPORT jint JNICALL
Java_org_echoline_drawterm_MainActivity_dtmain(
        JNIEnv *env,
        jobject obj,
        jobjectArray argv) {
    int i, ret;
    char **args = (char **) malloc(((*env)->GetArrayLength(env, argv)+1) * sizeof(char *));

    for (i = 0; i < (*env)->GetArrayLength(env, argv); i++) {
        jobject str = (jobject) (*env)->GetObjectArrayElement(env, argv, i);
        args[i] = strdup((char*)(*env)->GetStringUTFChars(env, (jstring)str, 0));
    }
    args[(*env)->GetArrayLength(env, argv)] = NULL;

    ret = dt_main(i, args);

    for (i = 0; args[i] != NULL; i++) {
        free(args[i]);
    }
    free(args);

    return ret;
}

JNIEXPORT void JNICALL
Java_org_echoline_drawterm_MainActivity_setMouse(
        JNIEnv *env,
        jobject obj,
        jintArray args) {
    jboolean isCopy;
    jint *data;
    if ((*env)->GetArrayLength(env, args) < 3)
        return;
    data = (*env)->GetIntArrayElements(env, args, &isCopy);
    mousept.x = (int)(data[0] / ws);
    mousept.y = (int)(data[1] / hs);
    buttons = data[2];
    (*env)->ReleaseIntArrayElements(env, args, data, 0);
    absmousetrack(mousept.x, mousept.y, buttons, ticks());
}

JNIEXPORT void JNICALL
Java_org_echoline_drawterm_MainActivity_setDTSurface(
	JNIEnv* jenv,
	jobject obj,
	jobject surface) {
    if (surface != NULL) {
        window = ANativeWindow_fromSurface(jenv, surface);
	ANativeWindow_setBuffersGeometry(window, screenWidth, screenHeight,
		AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM);
	flushmemscreen(Rect(0, 0, screenWidth, screenHeight));
    } else {
        ANativeWindow_release(window);
	window = NULL;
    }
}
