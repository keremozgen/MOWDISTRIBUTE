# MOWDISTRIBUTE
A project for updating source code of multiple targets that can be cross-platform.
You can use mowdistribute.h with including mowsocket, mowthread and mowfile.
You don't need to use CMakeFile.txt or fips build tool.
In mowdistribute.c you can see how it is implemented.
This library uses UDP broadcast for discovery, but on android you need to create a multicast lock.
There is a bug that we create this lock and acquire it but does not have any affects. But there is a workaround solution added inside header.

Here is a quick intro:

struct mdBroadcast {
	char* message;	//THIS IS UNIQUE FOR YOUR APPLICATION. MAYBE CAN BE YOUR APPLICATION NAME AND ITS VERSION.
	uint16_t message_len;
	volatile int* cond;	//YOU CAN STOP BROADCASTING WITH CHANGING THE VALUE OF POINTED VARIABLE.
	uint16_t broadcast_interval_ms;	//INTERVAL BETWEEN MESSAGES.
	int action;	//MD_GET OR MD_DISTRIBUTE
	char* update_location;	//LOCATION OF THE PROJECT
	char* android_lib_location;	//IF ANDROID APPLICATION WANTS AN UPDATED LIB THIS IS (libname).so
	uint64_t waiting_limit_seconds;	//FOR A LIMITED TIME. 0 FOR UNLIMITED
	void* android_app_struct_ptr;	//ON ANDROID THIS IS struct android_app*
};
You need to use this struct.

	int md_get_update(struct mdBroadcast* bs);
This function is same on both distributor and receiver application. You can call it as a thread. Returns when stopping conditions (timeout,
*cond changes etc.).

	int md_send_folder_recursively(struct mowfolder* folder, struct mowsocket** sockets, uint32_t socket_count);
You can send the project to receivers if you want them to store the project.
There is a hash calculation (http://cyan4973.github.io/xxHash/) done for looking if content lengths are the same. So if length is not the same and
file does not exists this step is skipped. If you don't want any hash calculations you can #define MOW_SKIP_HASH 1.

	int md_update_lib(void *android_app_str);
This function is for android application. It updates it's native library so you don't have to install the new apk from android studio.
