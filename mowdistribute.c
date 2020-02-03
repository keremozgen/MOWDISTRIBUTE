#include "mowdistribute.h"

#ifdef __ANDROID__
void android_main(struct android_app* app) {
#if 0
	{
		struct mowsocket* ts = msocket(MOW_IP4, MOW_UDP, MOW_LISTEN, str_to_ho_addr("192.168.1.31"), 8998);
		if (MOWSOCKETERR == msetsockpredopt(ts, MOW_SO_BROADCAST, 1))
			printf("Can't set sock broadcast\n");
		char d[7];
		uint32_t pad;
		uint16_t pap;
		while (1) {
			msendto(ts->socketd, "DENEME\0", 7, str_to_ho_addr("192.168.1.38"), 8998);
			memset(d, 0, 7);
			mrecvfrom(ts->socketd, d, 7, &pad, &pap);
			if (pad != str_to_ho_addr("192.168.1.31"))
				printf("%s\n", d);
		}
	}
#endif
	/*{
		struct mowsocket* ts = msocket(MOW_IP4, MOW_UDP, MOW_LISTEN, str_to_ho_addr("192.168.1.31"), 8998);
		if (MOWSOCKETERR == msetsockpredopt(ts, MOW_SO_BROADCAST, 1))
			printf("Can't set sock broadcast\n");
		char d[7];
		uint32_t pad;
		uint16_t pap;
		while (1) {
			msendto(ts->socketd, "DENEME\0", 7, str_to_ho_addr("192.168.1.38"), 8998);
			memset(d, 0, 7);
			mrecvfrom(ts->socketd, d, 7, &pad, &pap);
			if (pad != str_to_ho_addr("192.168.1.31"))
				printf("%s\n", d);
		}
	}*/
	printf("Value of android_tmp_val22: %d\n", android_tmp_val);
	if (0 == android_tmp_val) {
		printf("Inside\n");
		if (MOWDISTRIBUTERR == md_update_lib(app)) {
			printf("Using the apk lib\n");
		}
		else {
			printf("update returned %s %d\n", __FILE__, __LINE__);
			exit(0);
		}
	}
	else {
		printf("Not\n");
	}

	JavaVM* vm = app->activity->vm;
	JNIEnv* env = app->activity->env;
	(*vm)->AttachCurrentThread(vm, &env, NULL);
	ANativeActivity* activity = app->activity;
	jmethodID jtmID;
	jthrowable exc;

	jclass jNativeClass = (*env)->GetObjectClass(env, activity->clazz);
	jtmID = (*env)->GetMethodID(env, jNativeClass, "getApplication", "()Landroid/app/Application;");
	jobject jNativeApplication = (jobject)(*env)->CallObjectMethod(env, activity->clazz, jtmID);
	jtmID = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, jNativeApplication), "getApplicationContext", "()Landroid/content/Context;");
	jobject jNativeContext = (jobject)(*env)->CallObjectMethod(env, jNativeApplication, jtmID);

	/*jtmID = (*env)->GetMethodID(env, jNativeClass, "getApplicationInfo", "()Landroid/content/pm/ApplicationInfo;");
	jobject jAppInfo = (*env)->CallObjectMethod(env, jNativeContext, jtmID);
	jfieldID nativeLibraryDirFID = (*env)->GetFieldID(env, (*env)->GetObjectClass(env, jAppInfo), "nativeLibraryDir", "Ljava/lang/String;");
	jstring nativeLibjStr = (*env)->GetObjectField(env, jAppInfo, nativeLibraryDirFID);
	char *nativeLibLoc = (*env)->GetStringUTFChars(env, nativeLibjStr, 0);
	printf("%s nativeLibLoc\n", nativeLibLoc);
	android_native_lib_loc = nativeLibLoc;*/

	jfieldID jNativeWIFI_SERVICE_fid = (*env)->GetStaticFieldID(env, (*env)->GetObjectClass(env, jNativeContext), "WIFI_SERVICE", "Ljava/lang/String;");
	jstring jNativeSFID_jstr = (jstring)(*env)->GetStaticObjectField(env, (*env)->FindClass(env, "android/content/Context"), jNativeWIFI_SERVICE_fid);
	jstring wifiLockjStr = (*env)->NewStringUTF(env, "PROJECT-MOWa::wifilock");

	jtmID = (*env)->GetMethodID(env, (*env)->FindClass(env, "android/content/Context"), "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
	jobject jSystemService = (*env)->CallObjectMethod(env, jNativeContext, jtmID, jNativeSFID_jstr);

	jclass jWMClass = (*env)->FindClass(env, "android/net/wifi/WifiManager");
	jclass jWMMLClass = (*env)->FindClass(env, "android/net/wifi/WifiManager$MulticastLock");
	jtmID = (*env)->GetMethodID(env, jWMClass, "createMulticastLock", "(Ljava/lang/String;)Landroid/net/wifi/WifiManager$MulticastLock;");
	jobject jMCObj = (*env)->CallObjectMethod(env, jSystemService, jtmID, wifiLockjStr);

	jtmID = (*env)->GetMethodID(env, jWMMLClass, "setReferenceCounted", "(Z)V");
	(*env)->CallVoidMethod(env, jMCObj, jtmID, 0);

	jtmID = (*env)->GetMethodID(env, jWMMLClass, "acquire", "()V");
	(*env)->CallVoidMethod(env, jMCObj, jtmID);
	jtmID = (*env)->GetMethodID(env, jWMMLClass, "isHeld", "()Z");
	jboolean isheld = (*env)->CallBooleanMethod(env, jMCObj, jtmID);
	if (isheld) {
		printf("MulticastLock is Held\n");
	}
	else {
		printf("MulticastLock is not Held\n");
	}
	exc = (*env)->ExceptionOccurred(env);
	if (exc) {
		(*env)->ExceptionDescribe(env);
		(*env)->ExceptionClear(env);
	}


	jclass jWMWFClass = (*env)->FindClass(env, "android/net/wifi/WifiManager$WifiLock");
	jtmID = (*env)->GetMethodID(env, jWMClass, "createWifiLock", "(ILjava/lang/String;)Landroid/net/wifi/WifiManager$WifiLock;");
	jfieldID wifiModFID = (*env)->GetStaticFieldID(env, jWMClass, "WIFI_MODE_FULL_HIGH_PERF", "I");
	jint wifiModJINT = (jint)(*env)->GetStaticIntField(env, jWMClass, wifiModFID);
	jobject jWFObj = (*env)->CallObjectMethod(env, jSystemService, jtmID, wifiModJINT, wifiLockjStr);
	jtmID = (*env)->GetMethodID(env, jWMWFClass, "setReferenceCounted", "(Z)V");
	(*env)->CallVoidMethod(env, jWFObj, jtmID, 1);

	jtmID = (*env)->GetMethodID(env, jWMWFClass, "acquire", "()V");
	(*env)->CallVoidMethod(env, jWFObj, jtmID);
	jtmID = (*env)->GetMethodID(env, jWMWFClass, "isHeld", "()Z");
	isheld = (*env)->CallBooleanMethod(env, jWFObj, jtmID);
	if (isheld) {
		printf("WifiLock is Held\n");
	}
	else {
		printf("WifiLock is not Held\n");
	}
	exc = (*env)->ExceptionOccurred(env);
	if (exc) {
		(*env)->ExceptionDescribe(env);
		(*env)->ExceptionClear(env);
	}

	//	PowerManager powerManager = (PowerManager)getSystemService(POWER_SERVICE);
	//	WakeLock wakeLock = powerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK,
	//	"MyApp::MyWakelockTag");
	//	 wakeLock.acquire();
	jclass jPMClass = (*env)->FindClass(env, "android/os/PowerManager");
	jclass jWLClass = (*env)->FindClass(env, "android/os/PowerManager$WakeLock");
	jfieldID jPMps = (*env)->GetStaticFieldID(env, (*env)->FindClass(env, "android/content/Context"), "POWER_SERVICE", "Ljava/lang/String;");
	jstring jPMPSStr = (jstring)(*env)->GetStaticObjectField(env, (*env)->FindClass(env, "android/content/Context"), jPMps);
	jfieldID jPMPWL = (*env)->GetStaticFieldID(env, jPMClass, "PARTIAL_WAKE_LOCK", "I");
	jint jPMPWLi = (jint)(*env)->GetStaticIntField(env, jPMClass, jPMPWL);
	jtmID = (*env)->GetMethodID(env, (*env)->FindClass(env, "android/content/Context"), "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
	jobject jPMObj = (*env)->CallObjectMethod(env, jNativeContext, jtmID, jPMPSStr);

	jtmID = (*env)->GetMethodID(env, jPMClass, "newWakeLock", "(ILjava/lang/String;)Landroid/os/PowerManager$WakeLock;");
	jobject jWLObj = (*env)->CallObjectMethod(env, jPMObj, jtmID, jPMPWLi, wifiLockjStr);

	jtmID = (*env)->GetMethodID(env, jWLClass, "setReferenceCounted", "(Z)V");
	(*env)->CallVoidMethod(env, jWLObj, jtmID, 1);

	jtmID = (*env)->GetMethodID(env, jWLClass, "acquire", "()V");
	(*env)->CallVoidMethod(env, jWLObj, jtmID);
	jtmID = (*env)->GetMethodID(env, jWLClass, "isHeld", "()Z");
	isheld = (*env)->CallBooleanMethod(env, jWLObj, jtmID);
	if (isheld) {
		printf("WakeLock is Held\n");
	}
	else {
		printf("WakeLock is not Held\n");
	}
	exc = (*env)->ExceptionOccurred(env);
	if (exc) {
		(*env)->ExceptionDescribe(env);
		(*env)->ExceptionClear(env);
	}


	printf("Printing info\n");
	printf("internalDataPath: %s\nexternalDataPath: %s\n", activity->internalDataPath, activity->externalDataPath);
	jclass jEnvironment = (*env)->FindClass(env, "android/os/Environment");
	jfieldID jEnvDirFID = (*env)->GetStaticFieldID(env, jEnvironment, "DIRECTORY_DOWNLOADS", "Ljava/lang/String;");
	jstring jDirDown = (*env)->GetStaticObjectField(env, jEnvironment, jEnvDirFID);
	jmethodID env_gESPD = (*env)->GetStaticMethodID(env, jEnvironment, "getExternalStoragePublicDirectory", "(Ljava/lang/String;)Ljava/io/File;");
	jobject jF = (*env)->CallStaticObjectMethod(env, jEnvironment, env_gESPD, jDirDown);
	env_gESPD = (*env)->GetMethodID(env, (*env)->FindClass(env, "java/io/File"), "getAbsolutePath", "()Ljava/lang/String;");
	jstring ESPD = (*env)->CallObjectMethod(env, jF, env_gESPD);
	const char* cESPD = (*env)->GetStringUTFChars(env, ESPD, 0);
	char* android_update = (char*)calloc(strlen(cESPD) + strlen("/mowdistribute") + 1, sizeof(char));
	memcpy(android_update, cESPD, strlen(cESPD));
	android_update[strlen(cESPD)] = M_OS_DELIMITER_CHAR;
	memcpy(android_update + strlen(cESPD), "/mowdistribute", strlen("/mowdistribute"));
	printf("Android update location: %s\n", android_update);
	printf("cESPD: %s\n", cESPD);

	//todo(KEREM): HANDLE ALLOCS
/*
	jclass activityThread = (*env)->FindClass(env,"android/app/ActivityThread");
	jmethodID currentActivityThread = (*env)->GetStaticMethodID(env,activityThread, "currentActivityThread", "()Landroid/app/ActivityThread;");
	jobject at = (*env)->CallStaticObjectMethod(env,activityThread, currentActivityThread);

	jmethodID getApplication = (*env)->GetMethodID(env,activityThread, "getApplication", "()Landroid/app/Application;");
	jobject context = (*env)->CallObjectMethod(env,at, getApplication);


	jclass jContext = (*env)->FindClass(env, "android/content/Context");
	jclass jSContext;
	jobject jObj = (*env)->FindClass(env, "java/lang/Object");
	jfieldID fid = (*env)->GetStaticFieldID(env, jContext, "WIFI_SERVICE", "Ljava/lang/String;");
	jstring str = (jstring)(*env)->GetStaticObjectField(env, jContext, fid);

	jmethodID jContext_getApplicationContextMethod = (*env)->GetMethodID(env, jContext, "getApplicationContext", "()Landroid/content/Context;");
	jSContext = (jclass)(*env)->CallObjectMethod(env, activity->clazz, jContext_getApplicationContextMethod);

	//jclass ctx = (*env)->FindClass(env,"android/content/Context");


	jstring nativeString = (*env)->NewStringUTF(env, "DENEME\0");
	const char* cstr = (*env)->GetStringUTFChars(env, nativeString, 0);
	printf("%s is the toString1\n", cstr);
	cstr = (*env)->GetStringUTFChars(env, nativeString, 0);
	printf("%s is the toString2\n", cstr);



	jmethodID mid = (*env)->GetMethodID(env, jContext, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
	jobject tm = (*env)->CallObjectMethod(env, jSContext, mid, str);
	//tm = (*env)->NewGlobalRef(env,tm);


	jstring nativeString2 = (*env)->NewStringUTF(env, "DENEME2\0");
	jclass wifiManagerClass = (*env)->FindClass(env, "android/net/wifi/WifiManager");
	jmethodID createWifiLockMethod = (*env)->GetMethodID(env, wifiManagerClass, "createWifiLock", "(ILjava/lang/String;)Landroid/net/wifi/WifiManager$WifiLock;");
	jfieldID wifiModFID = (*env)->GetStaticFieldID(env, wifiManagerClass, "WIFI_MODE_FULL_HIGH_PERF", "I");
	jint wifiModJINT = (jint)(*env)->GetStaticIntField(env, wifiManagerClass, wifiModFID);
	jobject mwifilock = (*env)->CallObjectMethod(env, tm, createWifiLockMethod, wifiModJINT, nativeString2);
	jmethodID refCounted = (*env)->GetMethodID(env, (*env)->FindClass(env, "android/net/wifi/WifiManager$WifiLock"), "setReferenceCounted", "(Z)V");
	(*env)->CallVoidMethod(env, mwifilock, refCounted, 1);
	//mwifilock = (*env)->NewGlobalRef(env, mwifilock);
	createWifiLockMethod = (*env)->GetMethodID(env, (*env)->FindClass(env, "android/net/wifi/WifiManager$WifiLock"), "acquire", "()V");
	(*env)->CallVoidMethod(env, mwifilock, createWifiLockMethod);
	createWifiLockMethod = (*env)->GetMethodID(env, (*env)->FindClass(env, "android/net/wifi/WifiManager$WifiLock"), "isHeld", "()Z");
	jboolean isheldWifiLock = (*env)->CallBooleanMethod(env, mwifilock, createWifiLockMethod);
	if (isheldWifiLock) {
		printf("WifiLock is held\n");
	}

	exc = (*env)->ExceptionOccurred(env);
	if (exc) {
		(*env)->ExceptionDescribe(env);
		(*env)->ExceptionClear(env);
	}

	jclass multicastlockJClass = (*env)->FindClass(env, "android/net/wifi/WifiManager");
	//jstring MOWDISTRIBUTEMULTICASTLOCK = (*env)->NewStringUTF(env,"MOWDISTRIBUTEMULTICASTLOCK");
	jmethodID mclmid = (*env)->GetMethodID(env, multicastlockJClass, "createMulticastLock", "(Ljava/lang/String;)Landroid/net/wifi/WifiManager$MulticastLock;");
	jobject mclock = (*env)->CallObjectMethod(env, tm, mclmid, nativeString);
	refCounted = (*env)->GetMethodID(env, (*env)->FindClass(env, "android/net/wifi/WifiManager$MulticastLock"), "setReferenceCounted", "(Z)V");
	(*env)->CallVoidMethod(env, mclock, refCounted, 1);
	//mclock = (*env)->NewGlobalRef(env, mclock);
	jmethodID acquireLockMethod = (*env)->GetMethodID(env, (*env)->FindClass(env, "android/net/wifi/WifiManager$MulticastLock"), "acquire", "()V");
	(*env)->CallVoidMethod(env, mclock, acquireLockMethod);
	mclmid = (*env)->GetMethodID(env, (*env)->FindClass(env, "android/net/wifi/WifiManager$MulticastLock"), "isHeld", "()Z");
	isheld = (*env)->CallBooleanMethod(env, mclock, mclmid);
	if (isheld) {
		printf("MulticastLock is Held\n");
	}
	exc = (*env)->ExceptionOccurred(env);
	if (exc) {
		(*env)->ExceptionDescribe(env);
		(*env)->ExceptionClear(env);
	}
	*/
	/*

	*/


#else
int main(int argc, char* argv[]) {
#endif
#if defined(_WIN32) && defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif

#if 0
	{
		struct mowsocket* ts = msocket(MOW_IP4, MOW_UDP, MOW_LISTEN, str_to_ho_addr("192.168.1.38"), 8998);
		if (MOWSOCKETERR == msetsockpredopt(ts, MOW_SO_BROADCAST, 1))
			printf("Can't set sock broadcast\n");
		char d[7];
		uint32_t pad;
		uint16_t pap;
		while (1) {
			msendto(ts->socketd, "DENEME\0", 7, str_to_ho_addr("192.168.1.31"), 8998);
			msendto(ts->socketd, "DENEME\0", 7, str_to_ho_addr("192.168.1.255"), 8998);
			memset(d, 0, 7);
			mrecvfrom(ts->socketd, d, 7, &pad, &pap);
			if (pad != str_to_ho_addr("192.168.1.38"))
				printf("%s\n", d);
		}
	}
#endif


	char* mdcwd = m_get_current_dir();
	printf("Current working directory: %s\n", mdcwd);
	if (mdcwd) free(mdcwd);

	struct mowadapter* alladapters = m_get_adapters();
	m_clean_adapters(&alladapters);
	struct mowadapter* t = alladapters;
	while (t) {
		printf("%s / %s\n", ho_addr_to_str(t->h_address), ho_addr_to_str(t->h_broadcast));
		t = t->next;
	}m_free_adapters(&alladapters);

	volatile int k = 11;
	struct mdBroadcast bs;
	bs.message = "DENEME";
	bs.message_len = strlen(bs.message);
	bs.cond = &k;


#if defined(_WIN32)
	bs.action = MD_DISTRIBUTE;
	bs.waiting_limit_seconds = 4;
	bs.broadcast_interval_ms = 1000;
#ifdef _WIN32
	bs.update_location = "D:\\PROJECTS\\MOWDISTRIBUTE";	//////
	bs.android_lib_location = "D:\\PROJECTS\\fips-build\\MOWDISTRIBUTE\\android-make-debug\\libmowdistribute.so";
#else
#if 0
	bs.update_location = "/mnt/d/PROJECTS/MOWDISTRIBUTE";
#else
	bs.update_location = "/home/hp/MOWDISTRIBUTE";
#endif
#endif
#else
	bs.action = MD_GET;
	bs.broadcast_interval_ms = 1000;
	bs.waiting_limit_seconds = 0;
#ifdef __ANDROID__
	bs.android_app_struct_ptr = app;
#if 1
	bs.update_location = android_update;//activity->internalDataPath;
	//todo(KEREM): HANDLE ALLOCS
#else
	char* android_update_location = (char*)calloc(strlen(activity->externalDataPath) + strlen("/mowdistribute") + 1, sizeof(char));
	memcpy(android_update_location, activity->externalDataPath, strlen(activity->externalDataPath));
	android_update_location[strlen(activity->externalDataPath)] = M_OS_DELIMITER_CHAR;
	memcpy(android_update_location + strlen(activity->externalDataPath), "/mowdistribute", strlen("/mowdistribute"));
	printf("Android update location: %s\n", android_update_location);
	bs.update_location = android_update_location;
#endif
#else
	bs.update_location = "/home/hp/MOWDISTRIBUTE";
#endif
#endif
	printf("Starting md_get_update\n");
	mowthread bthread = mthread(md_get_update, &bs);
	mthread_join(bthread, NULL);
	//md_get_update(&bs);
#ifdef __ANDROID__
	jtmID = (*env)->GetMethodID(env, (*env)->FindClass(env, "android/net/wifi/WifiManager$WifiLock"), "release", "()V");
	(*env)->CallVoidMethod(env, jWFObj, jtmID);
	jtmID = (*env)->GetMethodID(env, (*env)->FindClass(env, "android/net/wifi/WifiManager$MulticastLock"), "release", "()V");
	(*env)->CallVoidMethod(env, jMCObj, jtmID);
	//(*env)->CallVoidMethod(env, mclock, acquireLockMethod);
	(*env)->DeleteLocalRef(env, jMCObj);
	//(*env)->DeleteLocalRef(env, mclock);
	/*
	createWifiLockMethod = (*env)->GetMethodID(env, (*env)->FindClass(env, "android/net/wifi/WifiManager$WifiLock"), "release", "()V");
	(*env)->CallVoidMethod(env, mwifilock, createWifiLockMethod);
	acquireLockMethod = (*env)->GetMethodID(env, (*env)->FindClass(env, "android/net/wifi/WifiManager$MulticastLock"), "release", "()V");
	(*env)->CallVoidMethod(env, mclock, acquireLockMethod);
	(*env)->CallVoidMethod(env, mclock, acquireLockMethod);
	(*env)->DeleteLocalRef(env, mwifilock);
	(*env)->DeleteLocalRef(env, mclock);
	*/
#endif
	if (MD_GET == bs.action) {
#if _WIN32
		if (0 != system("pushd ..\\..\\..\\MOWDISTRIBUTE && fips set config win64-vstudio-debug && fips build && popd")) {
			printf("Error occured when building\n");
		}
		else {
			printf("Built\n");
		}
#else
#ifndef __ANDROID__
		if (0 != system("./../../../MOWDISTRIBUTE/fips set config linux-make-debug && ./../../../MOWDISTRIBUTE/fips build")) {
			printf("Error occured when building\n");
		}
#endif
#endif

		printf("Running the binary\n");


#if 1

#ifdef _WIN32
		fcloseall();
		if (-1 == _execl(argv[0], argv[0], NULL)) {
			printf("ERROR: %s\n", strerror(errno));

		}
#else
#ifndef __ANDROID__
		fcloseall();
		if (-1 == execl(argv[0], argv[0], NULL)) {
			printf("ERROR: %s\n", strerror(errno));

		}
#endif

#endif

#endif
	}

#if defined(_WIN32) && defined(_DEBUG)

#endif

#ifndef __ANDROID__
	return 0;
#endif
}