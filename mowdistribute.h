#include "MOWSOCKET/mowsocket.h"
#include "MOWFILE/mowfile.h"
#include "MOWTHREAD/mowthread.h"
#define XXH_INLINE_ALL
#define XXH_IMPORT
#define XXH_STATIC_LINKING_ONLY
#define XXH_PRIVATE_API
#include "xxHash/xxhash.h"

#ifdef _WIN32
#include <process.h>
#endif
/*

XXH_INLINE_ALL : Make all functions inline, with bodies directly included within xxhash.h. Inlining functions is beneficial for speed on small keys. It's extremely effective when key length is expressed as a compile time constant, with performance improvements observed in the +200% range . See this article for details. Note: there is no need for an xxhash.o object file in this case.
XXH_REROLL : reduce size of generated code. Impact on performance vary, depending on platform and algorithm.
XXH_ACCEPT_NULL_INPUT_POINTER : if set to 1, when input is a NULL pointer, xxhash result is the same as a zero-length input (instead of a dereference segfault). Adds one branch at the beginning of the hash.
XXH_FORCE_MEMORY_ACCESS : default method 0 uses a portable memcpy() notation. Method 1 uses a gcc-specific packed attribute, which can provide better performance for some targets. Method 2 forces unaligned reads, which is not standard compliant, but might sometimes be the only way to extract better read performance.
XXH_CPU_LITTLE_ENDIAN : by default, endianess is determined at compile time. It's possible to skip auto-detection and force format to little-endian, by setting this macro to 1. Setting it to 0 forces big-endian.
XXH_PRIVATE_API : same impact as XXH_INLINE_ALL. Name underlines that XXH_* symbols will not be published.
XXH_NAMESPACE : prefix all symbols with the value of XXH_NAMESPACE. Useful to evade symbol naming collisions, in case of multiple inclusions of xxHash source code. Client applications can still use regular function name, symbols are automatically translated through xxhash.h.
XXH_STATIC_LINKING_ONLY : gives access to state declaration for static allocation. Incompatible with dynamic linking, due to risks of ABI changes.
XXH_NO_LONG_LONG : removes support for XXH64, for targets without 64-bit support.
XXH_IMPORT : MSVC specific : should only be defined for dynamic linking, it prevents linkage errors.

*/

//IF ANDROID INCLUDE ANDROID ONLY LIBRARIES
#if defined(__ANDROID__) && !defined(ANDROIDPRINT)
#define ANDROIDPRINT
#include <jni.h>
#include <android/log.h>
#include "android/android_native_app_glue.h"
#endif
#if defined(__ANDROID__)
#include <stdio.h>
#include <dlfcn.h>
char* android_native_lib_loc = NULL;
int android_tmp_val = 0;
#define MOW_SKIP_HASH 1
#define printf(...) __android_log_print(ANDROID_LOG_DEBUG, "MOW", __VA_ARGS__);
#else
#define MOW_SKIP_HASH 0
#endif

//GENERAL DEFINES HERE
#define MOWDISTRIBUTEOK 1
#define MOWDISTRIBUTERR 0

#define MD_GET 2		//FIND GOOD NAMES
#define MD_DISTRIBUTE 3	//FIND GOOD NAMES

#define MOWBROADCASTPORT 8998
#define MOWDISTRIBUTEPORT 9999

//GENERAL DEBUG DEFINES HERE

//IF ANDROID INCLUDE ANDROID ONLY LIBRARIES

//ANDROID ONLY DEFINES HERE

//IF WINDOWS INCLUDE WINDOWS ONLY LIBRARIES

//DEFINE MOWDISTRIBUTE VARIABLES HERE

struct mdAvailable {
	uint32_t size;
	uint32_t* address_ho;
	uint16_t* port_ho;
};

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

//DEFINE MOWDISTRIBUTE FUNCTIONS HERE

int md_get_update(struct mdBroadcast* bs);

unsigned int compare_ip_with_adapters(uint32_t address_ho, struct mowadapter* adapters);

unsigned int compare_ip_with_uint32_t_p(uint32_t address_ho, uint32_t* address_p);

int md_send_folder_recursively(struct mowfolder* folder, struct mowsocket** sockets, uint32_t socket_count);

int md_update_lib(void *android_app_str);	//FOR ANDROID

//IMPLEMENT MOWDISTRIBUTE FUNCTIONS HERE

int md_update_lib(void *android_app_str) {
#ifdef __ANDROID__
	if (NULL == android_app_str) return MOWDISTRIBUTERR;
	struct android_app* and_app = (struct android_app*) android_app_str;
	char* lib = "/data/data/org.fips.mowdistribute/files/libmowdistribute.so";
	void* myso = dlopen(lib, RTLD_NOW | RTLD_NODELETE | RTLD_LOCAL);
	if (NULL == myso) return MOWDISTRIBUTERR;
	int* android_tmp_val_ptr = (int*)dlsym(myso, "android_tmp_val");
	char* dl_err_str = NULL;
	(*android_tmp_val_ptr) = android_tmp_val + 1;
	dl_err_str = dlerror();
	if (dl_err_str)
		printf("dlsym %s %s %d\n", dl_err_str, __FILE__, __LINE__);
	void (*fun_ptr)(struct android_app*) = dlsym(myso, "android_main");
	dl_err_str = dlerror();
	if (dl_err_str)
		printf("dlsym %s %s %d\n", dl_err_str, __FILE__, __LINE__);
	dlclose(myso);
	dl_err_str = dlerror();
	if (dl_err_str)
		printf("dlclose %s %s %d\n", dl_err_str, __FILE__, __LINE__);
	printf("Running the new lib\n");
	fun_ptr(and_app);
	return MOWDISTRIBUTEOK;
#endif
	return MOWDISTRIBUTERR;
}

unsigned int compare_ip_with_uint32_t_p(uint32_t address_ho, uint32_t* address_p) {
	if (NULL == address_p) {
		return 0;
	}
	while (*address_p) {
		if (*address_p == address_ho) {
			return 1;
		}
		address_p++;
	}
	return 0;
}

unsigned int compare_ip_with_adapters(uint32_t address_ho, struct mowadapter* adapters) {	// 0 ON NO MATCH 1 ON MATCH
	if (NULL == adapters) {
		return 0;
	}
	while (adapters) {
		if (adapters->h_address == address_ho || adapters->h_broadcast == address_ho) {
			return 1;
		}
		adapters = adapters->next;
	}
	return 0;
}

int md_send_folder_recursively(struct mowfolder* folder, struct mowsocket** sockets, uint32_t socket_count) {
	if (NULL == folder || NULL == sockets || 0 == socket_count) {
		printf("md_send_folder_recursively parameters are incorrect\n");
		return MOWDISTRIBUTERR;
	}
	//1 FOR FILE 2 FOR FOLDER
	//SEND FOLDER SELF INFO (8bits for folder/file descriptor 64bits for folder_name_length)
	char* folder_info = (char*)calloc(9, sizeof(char));
	if (NULL == folder_info) {
		printf("%s %d ", __FILE__, __LINE__);
		goto MD_SEND_FOLDER_RECURSIVELY_ALLOC_ERROR;
	}
	*folder_info = 2;
	uint64_t no_folder_name_length = hton64_t(folder->name_length);
	memcpy(folder_info + 1, &no_folder_name_length, sizeof(no_folder_name_length));		//CHAR	+	64BIT NAME LENGTH
	uint64_t send_r = 0;																//2		+	(NAME LENGTH)
	for (uint32_t i = 0; i < socket_count; i++)
	{
		send_r = msends(sockets[i], folder_info, 9);					//SEND INFO MESSAGE
		if (9 != send_r) {
			printf("Sending folder info returned %llu of %d.%s %d\n", send_r, sizeof(folder_info), __FILE__, __LINE__);
		}
		send_r = msends(sockets[i], folder->folder_name, folder->name_length);			//SEND FOLDER NAME
		if (folder->name_length != send_r) {
			printf("Sending folder name returned %llu of %d.%s %d\n", send_r, folder->name_length, __FILE__, __LINE__);
		}
	}
	if (folder_info) free(folder_info);

	//SEND FILES
	for (uint64_t i = 0; i < folder->file_count; i++)
	{
		//GET THE FILE HASH
		XXH128_hash_t hash = { 0 };
		XXH128_hash_t peer_hash = { 0 };
		for (uint32_t j = 0; j < socket_count; j++) {
			char file_info = 1; uint64_t tmp64_t;
			if (1 != msends(sockets[j], &file_info, sizeof(char))) {
				printf("Sending file info failed.%s %d\n", __FILE__, __LINE__);
			}
			//SEND FILE NAME LENGTH
			tmp64_t = hton64_t(folder->files[i]->name_length);
			if (sizeof(folder->files[i]->name_length) != msends(sockets[j], &tmp64_t, sizeof(tmp64_t))) {
				printf("Sending file name length failed.%s %d\n", __FILE__, __LINE__);
			}
			if (folder->files[i]->name_length != msends(sockets[j], folder->files[i]->file_name, folder->files[i]->name_length)) {
				printf("Sending file name failed.%s %d\n", __FILE__, __LINE__);
			}
			tmp64_t = hton64_t(folder->files[i]->content_length);
			if (sizeof(folder->files[i]->content_length) != msends(sockets[j], &tmp64_t, sizeof(tmp64_t))) {
				printf("Sending file content length failed.%s %d\n", __FILE__, __LINE__);
			}
			//GET THE INFO OF HASH
			if (16 != mrecv_fill(sockets[j]->socketd, &peer_hash, 16)) {
				printf("Can't get peer hash\n");
			}
			else {
				peer_hash.high64 = ntoh64_t(peer_hash.high64);
				peer_hash.low64 = ntoh64_t(peer_hash.low64);
				if (peer_hash.low64 == 0 && peer_hash.high64 == 0) {	//NO NEED FOR HASH CALCULATION
					goto MD_SEND_RECURSIVELY_SEND_FILE_INFO_4;
				}
				else {
					if (hash.high64 == 0 && hash.low64 == 0) {
						hash = XXH128(folder->files[i]->content, folder->files[i]->content_length, 13);
					}
				}
				if (1 == XXH128_isEqual(peer_hash, hash)) {	//IF HASHES ARE EQUAL THAN SEND SKIP INFO
					file_info = 3;
					if (sizeof(file_info) != msends(sockets[j], &file_info, sizeof(file_info))) {
						printf("Sending file info for skip failed.%s %d\n", __FILE__, __LINE__);
					}
					goto MD_SEND_FOLDER_RECURSIVELY_NEXT_SOCKET;
				}
				else {
				MD_SEND_RECURSIVELY_SEND_FILE_INFO_4:
					file_info = 4;
					if (sizeof(file_info) != msends(sockets[j], &file_info, sizeof(file_info))) {
						printf("Sending file info after hash comparison failed.%s %d\n", __FILE__, __LINE__);
					}
				}
			}
			if (folder->files[i]->content_length != msends(sockets[j], folder->files[i]->content, folder->files[i]->content_length)) {	//todo(kerem): HANDLE SENDING 0 CONTENT LENGTH FOR EMPTY FILES
				printf("Sending file content failed.%s %d\n", __FILE__, __LINE__);
			}
			else {
				printf("%s address %s file\n", ho_addr_to_str(sockets[j]->h_address), folder->files[i]->file_name);
			}
		MD_SEND_FOLDER_RECURSIVELY_NEXT_SOCKET:;
		}
	}
	for (uint64_t k = 0; k < folder->folder_count; k++)
	{
		if (MOWDISTRIBUTERR == md_send_folder_recursively(folder->folders[k], sockets, socket_count)) {
			printf("md_send_folder_recursively failed %s %d\n", __FILE__, __LINE__);
			return MOWDISTRIBUTERR;
		}
	}
	return MOWDISTRIBUTEOK;
MD_SEND_FOLDER_RECURSIVELY_ALLOC_ERROR:
	printf("md_send_folder_recursively can't allocate memory\n");
	return MOWDISTRIBUTERR;
}



int md_get_update(struct mdBroadcast* bs) {
	int stored_cond = 11;
	int retVal = MOWDISTRIBUTERR;
	if (NULL == bs || NULL == bs->message || 0 == bs->message_len || bs->message_len != strlen(bs->message)) {
		printf("md_get_update parameters are not correct\n");
		goto MD_GET_UPDATE_RETURN;
	}
	if (MD_DISTRIBUTE != bs->action && MD_GET != bs->action) {
		printf("md_get_update action can be MD_DISTRIBUTE or MD_GET\n");
		goto MD_GET_UPDATE_RETURN;
	}
	if (NULL == bs->update_location) {
		printf("md_get_update update_location is NULL\n");
		goto MD_GET_UPDATE_RETURN;
	}
	if (NULL != bs->cond) stored_cond = *bs->cond;
	struct mowadapter* cleanAdapters = m_get_adapters();
	if (NULL == cleanAdapters) {
		printf("md_get_update can't get adapters\n");
		goto MD_GET_UPDATE_RETURN;
	}
	m_clean_adapters(&cleanAdapters);
	if (NULL == cleanAdapters) {
		printf("md_get_update there aren't any working adapters\n");
		goto MD_GET_UPDATE_RETURN;
	}
	int adapter_count = 0;
	{
		struct mowadapter* t = cleanAdapters;
		while (t) {
			printf("%s %d address\n", ho_addr_to_str(t->h_address), adapter_count);
			adapter_count++;
			t = t->next;
		}
	}printf("%d TOTAL ADAPTERS\n", adapter_count);
	struct mowsocket** bsockets = (struct mowsocket**) calloc(adapter_count, sizeof(struct mowsocket*));
	if (NULL == bsockets) {
		printf("md_get_update can't allocate memory\n");
		m_free_adapters(&cleanAdapters);
		goto MD_GET_UPDATE_RETURN;
	}
	{
		struct mowadapter* tadapter = cleanAdapters;
		for (int i = 0; i < adapter_count; i++)
		{
			bsockets[i] = msocket(MOW_IP4, MOW_UDP, MOW_LISTEN, tadapter->h_broadcast, MOWBROADCASTPORT);
			if (NULL == bsockets[i]) {
				printf("md_get_update can't create socket %s %d\n", __FILE__, __LINE__);
				goto MD_GET_UPDATE_RETURN;
			}
			if (MOWSOCKETOK != msetsockpredopt(bsockets[i], MOW_SO_BROADCAST, 1)) {
				printf("md_get_update can't set socket option broadcast\n");
				goto MD_GET_UPDATE_RETURN;
			}
			tadapter = tadapter->next;
		}
	}
	uint64_t send_r = 0;
	uint64_t time_b = 0, time_a = 0;
	char* message_buf = NULL;
	struct mowadapter* tmpAdapters = m_get_adapters();
	if (NULL == tmpAdapters) {
		printf("md_get_update can't get any adapters\n");
		goto MD_GET_UPDATE_RETURN;
	}
	uint32_t* ip_list = NULL;
	uint32_t ip_list_size = 0;
	int64_t left_time_millis = bs->waiting_limit_seconds * 1000;
	while ((stored_cond == ((NULL == bs->cond) ? stored_cond : *bs->cond)) && ((bs->waiting_limit_seconds > 0) ? (left_time_millis > 0) : 1)) {
		message_buf = (char*)calloc(bs->message_len + 1, sizeof(char));
		if (NULL == message_buf) {
			printf("md_get_update can't allocate memory\n");
			goto MD_GET_UPDATE_RETURN;
		}
		if (MD_GET == bs->action) {
			struct mowadapter* tadapter = cleanAdapters;
			for (int i = 0; i < adapter_count; i++)
			{
				send_r = msendto(bsockets[i]->socketd, bs->message, bs->message_len, tadapter->h_broadcast, MOWBROADCASTPORT);
				if (send_r != bs->message_len) {
					printf("md_get_update %llu bytes sent of %lu %s adapter\n", send_r, bs->message_len, ho_addr_to_str(cleanAdapters[i].h_address));
				}
				tadapter = tadapter->next;
			}
		}
		else {
			char* distributor_msg = (char*)calloc(bs->message_len + 1, sizeof(char));
			if (NULL == distributor_msg) {
				printf("md_get_update can't allocate memory\n");
				goto MD_GET_UPDATE_RETURN;
			}memcpy(distributor_msg, bs->message, bs->message_len);
			distributor_msg[bs->message_len] = 1;
			struct mowadapter* tadapter = cleanAdapters;
			for (int i = 0; i < adapter_count; i++)
			{
				send_r = msendto(bsockets[i]->socketd, distributor_msg, bs->message_len + 1, tadapter->h_broadcast, MOWBROADCASTPORT);
				if (send_r != bs->message_len + 1) {
					printf("md_get_update %llu bytes sent of %lu %s adapter\n", send_r, bs->message_len + 1, ho_addr_to_str(cleanAdapters[i].h_address));
				}
				//BECAUSE OUR MULTICAST LOCK ON ANDROID DOES NOT WORK WE CAN SEND MESSAGE ALSO TO THE IP ADDRESS FOR ANDROID DEVICE TO GET OUR MESSAGE
				for (uint32_t ipt = 0; ipt < ip_list_size; ipt++)
				{
					if ((ip_list[ipt] | ~(tadapter->h_netmask)) == tadapter->h_broadcast) {
						send_r = msendto(bsockets[i]->socketd, distributor_msg, bs->message_len + 1, ip_list[ipt], MOWBROADCASTPORT);
						printf("Sent to %s\n", ho_addr_to_str(ip_list[ipt]));
						if (send_r != bs->message_len + 1) {
							printf("md_get_update %llu bytes sent of %lu %s adapter\n", send_r, bs->message_len + 1, ho_addr_to_str(cleanAdapters[i].h_address));
						}
					}
				}
				tadapter = tadapter->next;
			}
			free(distributor_msg);
		}
		uint64_t delta = 0;
		uint16_t elapsed_time = 0;
		while (elapsed_time < bs->broadcast_interval_ms) {
			for (int i = 0; i < adapter_count; i++)
			{
				time_b = mprecise_time_of_day_ns();
				if (MOWSOCKETOK == msetsockpredopt(bsockets[i], MOW_SO_RECVTIMEO, bs->broadcast_interval_ms / adapter_count)) {
					uint32_t peer_addr_ho = 0;
					uint16_t peer_port_ho = 0;
					uint64_t rnb = mrecvfrom(bsockets[i]->socketd, message_buf, bs->message_len + 1, &peer_addr_ho, &peer_port_ho);
					//printf("Message from %s %d\n", ho_addr_to_str(peer_addr_ho), peer_port_ho);
					if (rnb)
						if (0 == compare_ip_with_adapters(peer_addr_ho, tmpAdapters) && 0 != peer_addr_ho) {
							if (0 == rnb) {
								printf("md_get_update recv returned 0 %s %d\n", __FILE__, __LINE__);
							}
							if (0 == memcmp(bs->message, message_buf, bs->message_len) && (MD_GET == bs->action) ? ((rnb == (bs->message_len + 1) && 1 == message_buf[bs->message_len])) : (rnb == (bs->message_len))) {
								if (0 == compare_ip_with_uint32_t_p(peer_addr_ho, ip_list)) {
									//UPDATE THE LIST
									ip_list_size++;
									uint32_t* tp = (uint32_t*)realloc(ip_list, sizeof(uint32_t) * (ip_list_size + 1));
									if (NULL == tp) {
										printf("md_get_update can't allocate memory %s %d\n", __FILE__, __LINE__);
										free(ip_list);
										break;
									}
									ip_list = tp;
									ip_list[ip_list_size - 1] = peer_addr_ho;
									ip_list[ip_list_size] = 0;
									printf("Found %s address\n", ho_addr_to_str(peer_addr_ho));

								}
								else if (MD_GET == bs->action) {	//FOR FAILSAFE MEASURES SENDING 1 MESSAGE MORE FOR ENSURING THE DISTRIBUTOR GETS THIS MESSAGE
									printf("GOTO MD_GET_UPDATE_ACTION\n");
									free(message_buf);
									goto MD_GET_UPDATE_ACTION;
								}
							}
						}
					if (MOWSOCKETERR == msetsockpredopt(bsockets[i], MOW_SO_RECVTIMEO, 0)) {	//todo(kerem): Do we really need to set it to 0? Maybe not try it out
						printf("md_get_update MOW_SO_RECVTIMEO can't set to 0\n");
						goto MD_GET_UPDATE_RETURN;
					}
				}
				time_a = mprecise_time_of_day_ns();
				delta = time_a - time_b;
				delta /= 1000000;
				elapsed_time += (uint16_t)delta;
				if (bs->waiting_limit_seconds) left_time_millis -= delta;
			}
		}
		free(message_buf);
		if (bs->waiting_limit_seconds > 0 && left_time_millis <= 0) break;
	}
MD_GET_UPDATE_ACTION:
	if (MD_GET == bs->action) {
		for (int i = 0; i < adapter_count; i++)
		{
			mclose(&bsockets[i]);
		}
		free(bsockets);
		bsockets = NULL;
		if (ip_list_size != 1) {
			printf("md_get_update There are %u number of distributors\nNot updating\n", ip_list_size);
			goto MD_GET_UPDATE_RETURN;
		}
		struct mowadapter* t_adapter = cleanAdapters;
		uint32_t ho_addr_to_use = 0;
		while (t_adapter) {
			uint32_t t = ip_list[0] | ~(t_adapter->h_netmask);
			if ((ip_list[0] | ~(t_adapter->h_netmask)) == t_adapter->h_broadcast)	ho_addr_to_use = t_adapter->h_address;
			t_adapter = t_adapter->next;
		}
		struct mowsocket* distributor_connection = msocket(MOW_IP4, MOW_TCP, MOW_LISTEN, ho_addr_to_use, MOWDISTRIBUTEPORT);
		if (NULL == distributor_connection) {
			printf("md_get_update can't create socket %s %d %s address\n", __FILE__, __LINE__, ho_addr_to_str(ho_addr_to_use));
			goto MD_GET_UPDATE_RETURN;
		}
		if (MOWSOCKETERR == msetsockpredopt(distributor_connection, MOW_SO_DONTLINGER, 1)) {
			printf("md_get_update can't set sockopt MOW_SO_DONTLINGER %s %d\n", __FILE__, __LINE__);
		}
		//ACCEPT THE CONNECTION
		uint32_t tcp_conn_addr;
		uint16_t tcp_conn_port;
		int r = 0;
	MD_GET_UPDATE_ACCEPT:
		r = maccepts(distributor_connection, &tcp_conn_addr, &tcp_conn_port);
		if (MOWSOCKETERR == r) {
			printf("md_get_update accept returned error %s %d\n", __FILE__, __LINE__);
			mclose(&distributor_connection);
			goto MD_GET_UPDATE_RETURN;
		}
		if (tcp_conn_addr != ip_list[0]) {
			printf("Non distributor ip address %s connected.\n", ho_addr_to_str(tcp_conn_addr));
			mclosesd(r);
			goto MD_GET_UPDATE_ACCEPT;
		}
		//GET THE FOLDERS AND FILES
		printf("Connected with the distibutor. Accepted\n");

		char info = 0;
		uint64_t recv_len = 1;
		char* distributor_main_folder_path = NULL;
		uint64_t distributor_main_folder_path_length = 0;
		char* t_abs_update_location = NULL;
		//GET THE CURRENT WORKING DIRECTORY
		char* md_before_wd = m_get_current_dir();
		if (NULL == md_before_wd) {
			printf("md_get_update can't get the current working directory\n");
		}
		//CHANGE THE WORKING DIRECTORY TO THE UPDATE DIRECTORY
		if (bs->update_location[0] != '.') {
			if (MOWFILEERR == m_dir_exist(bs->update_location)) {
				printf("%s update location does not exists. Creating it\n", bs->update_location);
				if (MOWFILEERR == m_create_dir(bs->update_location)) {
					printf("Can't create %s directory\n", bs->update_location);
				}
			}
			if (MOWFILEERR == m_set_current_dir(bs->update_location)) {
				printf("Can't change directory to %s\n", bs->update_location);
			}
		}
		else {
			size_t md_before_wd_length = strlen(md_before_wd);
			size_t update_location_length = strlen(bs->update_location);
			if (0 == md_before_wd_length || 0 == update_location_length) {
				//todo(kerem): Handle error
			}
			t_abs_update_location = (char*)calloc(update_location_length + md_before_wd_length + 2, sizeof(char));	//+2 (1 FOR PATH DELIMITER 1 FOR NULL TERMINATION)
			if (NULL == t_abs_update_location) {
				printf("md_get_update can't allocate memory\n");	//todo(kerem): Handle error
			}
			memcpy(t_abs_update_location, md_before_wd, md_before_wd_length);
			t_abs_update_location[md_before_wd_length] = M_OS_DELIMITER_CHAR;
			memcpy(t_abs_update_location + md_before_wd_length + 1, bs->update_location, update_location_length);
			m_path_conv_compat(t_abs_update_location);
			printf("%s changing path to\n", t_abs_update_location);
			if (MOWFILEERR == m_set_current_dir(t_abs_update_location)) {
				printf("Can't change directory to %s\n", t_abs_update_location);
			}
			if (MOWFILEERR == m_dir_exist(t_abs_update_location)) {
				printf("%s update location does not exists. Creating it\n", t_abs_update_location);
				if (MOWFILEERR == m_create_dir(t_abs_update_location)) {
					printf("Can't create %s directory\n", t_abs_update_location);
				}
			}
		}
		char* relative_folder_path = NULL;
		char* abs_folder_path = NULL;
		while (1) {
			//FIRST GET THE INFO TO DETERMINE FOLDER OR FILE
			recv_len = mrecv_fill(r, &info, sizeof(char));
			if (0 == info) {
				printf("Distributor closing\n");
#ifdef __ANDROID__
				char android_lib_info = 5;
				if (sizeof(android_lib_info) != msend(r, &android_lib_info, sizeof(android_lib_info))) {
					printf("Can't send android_lib_info\n");	//HANDLE ERROR
				}
				struct mowfile* lib = calloc(sizeof(struct mowfile), 1);
				if (NULL == lib) {

				}
				uint64_t lib_name_len, lib_content_len;
				if (sizeof(lib_name_len) != mrecv_fill(r, &lib_name_len, sizeof(lib_name_len))) {

				}
				lib_name_len = ntoh64_t(lib_name_len);
				lib->name_length = lib_name_len;
				lib->file_name = (char*)calloc(lib_name_len + 1, sizeof(char));
				if (NULL == lib->file_name) {	//todo(kerem): HANDLE ERROR CONDITIONS
					printf("Can't allocate memory %s %d\n", __FILE__, __LINE__);
				}
				if (lib_name_len != mrecv_fill(r, lib->file_name, lib_name_len)) {
					printf("Can't receive %s %d\n", __FILE__, __LINE__);
				}
				if (sizeof(lib_content_len) != mrecv_fill(r, &lib_content_len, sizeof(lib_content_len))) {
					printf("Can't receive %s %d\n", __FILE__, __LINE__);
				}
				lib_content_len = ntoh64_t(lib_content_len);
				lib->content_length = lib_content_len;
				lib->content = (char*)calloc(lib_content_len + 1, sizeof(char));
				if (NULL == lib->content) {
					printf("Can't allocate memory %s %d\n", __FILE__, __LINE__);
				}
				if (lib_content_len != mrecv_fill(r, lib->content, lib_content_len)) {
					printf("Can't receive %s %d\n", __FILE__, __LINE__);
				}
				//GET THIS FROM /proc/self/cmdline
				//FIRST REMOVE THE EXISTING ONE IN THE /files directory
				printf("%d returned deleting the lib\n", system("rm /data/data/org.fips.mowdistribute/files/libmowdistribute.so"));
				if (MOWFILEERR == m_write_file("/data/data/org.fips.mowdistribute/files", lib)) {
					printf("Can't write native lib to %s location\n", "/data/data/org.fips.mowdistribute/files"/*android_native_lib_loc*/);
				}
				else {
					if (MOWSOCKETERR == mshutdown(r, MCLOSE_BOTH)) {	//CLOSE READ(receive) OPERATION
						printf("Closing connection with %s address error occured\n", ho_addr_to_str(ip_list[0]));
					}
					mclose(&distributor_connection);

					if (MOWDISTRIBUTERR == md_update_lib(bs->android_app_struct_ptr)) {
						printf("Error md_update_lib %s %d\n", __FILE__, __LINE__);
					}
					else {
						printf("%d lib returned\n", android_tmp_val);
					}
				}
#endif
				if (MOWSOCKETERR == mshutdown(r, MCLOSE_BOTH)) {	//CLOSE READ(receive) OPERATION
					printf("Closing connection with %s address error occured\n", ho_addr_to_str(ip_list[0]));
				}
				printf("Connection closed with the distributor\n");
				break;
			}
			if (sizeof(info) != recv_len) {
				printf("Getting info returned %llu\n", recv_len);
				//HANDLE
	}
			uint64_t tmp_val = 0;
			recv_len = mrecv_fill(r, &tmp_val, sizeof(tmp_val));
			if (sizeof(tmp_val) != recv_len) {
				printf("Getting value returned %llu\n", tmp_val);
				//HANDLE
			}
			tmp_val = ntoh64_t(tmp_val);
			char* t_name = (char*)calloc(tmp_val + 1, sizeof(char));	//todo(kerem): HANDLE THIS TEMPORARY FIX //HANDLED?
			if (NULL == t_name) {
				printf("Can't allocate memory for name\n");
				//HANDLE
			}
			recv_len = mrecv_fill(r, t_name, tmp_val);
			if (tmp_val != recv_len) {
				printf("Getting name returned %llu of %llu\n", recv_len, tmp_val);
				//HANDLE
			}
			if (2 == info) {
				if (NULL == distributor_main_folder_path) {
					distributor_main_folder_path_length = strlen(t_name);
					distributor_main_folder_path = (char*)calloc(distributor_main_folder_path_length + 1, sizeof(char));
					if (NULL == distributor_main_folder_path) {
						printf("%s %d can't allcate memory\n");	//HANDLE ERROR
					}
					memcpy(distributor_main_folder_path, t_name, distributor_main_folder_path_length);
					if (tmp_val != distributor_main_folder_path_length) {	//todo(kerem): Handle error
						printf("Error on distributor main_folder_path length. Should be %llu but %llu\n", tmp_val, distributor_main_folder_path_length);
					}
				}
				relative_folder_path = t_name + distributor_main_folder_path_length;
				if (M_OS_DELIMITER_CHAR == t_name[distributor_main_folder_path_length - 1] || M_OS_FALSE_DELIMITER_CHAR == t_name[distributor_main_folder_path_length - 1]) {
					relative_folder_path++;	//+1 FOR FILE DELIMITER
				}
				if (abs_folder_path) free(abs_folder_path);
				//HANDLE THIS TEMPORARY FIXED ALLOCATION
				abs_folder_path = (char*)calloc(strlen(relative_folder_path) + 3 + ((NULL != t_abs_update_location) ? strlen(t_abs_update_location) : strlen(bs->update_location)), sizeof(char)); //todo(kerem): HANDLE THIS TEMPORARY FIX //HANDLED?
				if (NULL == abs_folder_path) {	//todo(kerem): HANDLE ERROR

				}
				memcpy(abs_folder_path, (t_abs_update_location) ? t_abs_update_location : bs->update_location, ((t_abs_update_location) ? strlen(t_abs_update_location) : strlen(bs->update_location)));
				abs_folder_path[((t_abs_update_location) ? strlen(t_abs_update_location) : strlen(bs->update_location))] = M_OS_DELIMITER_CHAR;
				memcpy(abs_folder_path + 1 + ((t_abs_update_location) ? strlen(t_abs_update_location) : strlen(bs->update_location)), relative_folder_path, strlen(relative_folder_path));
				m_path_conv_compat(abs_folder_path);
				if (abs_folder_path[strlen(abs_folder_path) - 1] != M_OS_DELIMITER_CHAR)
					abs_folder_path[strlen(abs_folder_path)] = M_OS_DELIMITER_CHAR;
				if (MOWFILEERR == m_dir_exist(abs_folder_path)) {
					if (MOWFILEERR == m_create_dir(abs_folder_path)) {
						printf("Can't create %s folder\n", abs_folder_path);
					}
				}
			}
			else if (1 == info) {
				uint64_t content_len = 0;
				recv_len = mrecv_fill(r, &content_len, sizeof(content_len));
				if (sizeof(content_len) != recv_len) {
					printf("Getting content length returned %llu\n", recv_len);
					//HANDLE
				}
				content_len = ntoh64_t(content_len);
				char* current_file_abs_name = (char*)calloc(strlen(abs_folder_path) + strlen(t_name) + 2, sizeof(char));
				if (NULL == current_file_abs_name) {
					//todo(kerem): Handle
					printf("%s %d can't allocate memory\n", __FILE__, __LINE__);
				}
				memcpy(current_file_abs_name, abs_folder_path, strlen(abs_folder_path));
				memcpy(current_file_abs_name + strlen(abs_folder_path), t_name, strlen(t_name));
				struct mowfile* current_file = m_read_file(current_file_abs_name);
				free(current_file_abs_name);
				char file_info_hash[16] = { 0 };	//16BYTES FOR HASH (128)/8
				//IF FILE DOES NOT EXIST OR CONTENT LENGTHS DONT MATCH WE ARE SURE THE FILES ARE DIFFERENT
				if (NULL == current_file || current_file->content_length != content_len) {
					if (16 != msend(r, file_info_hash, 16)) {
						printf("Can't send file_info_hash\n");	//HANDLE ERROR
					}
					if (NULL != current_file && current_file->content_length != content_len) {
						current_file->content_length = content_len;
						char* t_content = (char*)realloc(current_file->content, (content_len + 1) * sizeof(char));
						if (NULL == t_content) {
							printf("Can't realloc %s %d\n", __FILE__, __LINE__);
						}
						else {
							current_file->content = t_content;
							current_file->content[content_len] = 0;
							goto MD_GET_UPDATE_AFTER_HASH;
						}
					}
					goto MD_GET_UPDATE_FILE_DOES_NOT_EXIST;
				}
				if (NULL != current_file) {//CAN CHECK THE HASH VALUES BUT WE ARE SURE THAT SIZES ARE SAME
					//CALCULATE THE HASH OF THE FILE CONTENT AND SEND HASH VALUE
					XXH128_hash_t hash = { 0 };
#if !defined(MOW_SKIP_HASH) || (MOW_SKIP_HASH == 0)
					hash = XXH128(current_file->content, current_file->content_length, 13);
#endif
					hash.high64 = hton64_t(hash.high64);
					hash.low64 = hton64_t(hash.low64);
					memcpy(file_info_hash, &hash, 16);
					if (16 != msend(r, file_info_hash, 16)) {
						printf("Can't send file_info_hash\n");	//HANDLE ERROR
					}
				MD_GET_UPDATE_AFTER_HASH:
					recv_len = mrecv_fill(r, &info, sizeof(char));
					if (sizeof(info) != recv_len) {
						printf("Getting info returned %llu\n", recv_len);
						//HANDLE
					}
					if (3 == info) {	//HASH AND CONTENT LENGTH ARE SAME SKIP
						m_free_file(current_file);	//HANDLE RETURN VALUE
						goto MD_GET_UPDATE_NEXT;
					}
					else if (4 == info) {	//VALUES ARE NOT THE SAME GET THE NEW CONTENT
						recv_len = mrecv_fill(r, current_file->content, content_len);
						if (content_len != recv_len) {
							printf("Getting content returned %llu of %llu\n", recv_len, content_len);
							//HANDLE
						}
						if (MOWFILEERR == m_write_file(abs_folder_path, current_file)) {
							printf("Can't write the file %s\n", current_file->file_name);
						}
						else {
							printf("%s file written\n", current_file->file_name);
						}
						m_free_file(current_file);	//HANDLE RETURN VALUE
						goto MD_GET_UPDATE_NEXT;
					}
					else {
						printf("Unknown option sent by distributor\n");	//todo(kerem): HANDLE ERROR
					}
				}
			MD_GET_UPDATE_FILE_DOES_NOT_EXIST:;
				char* content = (char*)calloc(content_len + 1, sizeof(char));
				if (NULL == content) {
					printf("Can't allocate memory for content\n");
					//HANDLE
				}
				recv_len = mrecv_fill(r, &info, sizeof(char));
				if (sizeof(info) != recv_len && 4 != info) {
					printf("Getting info ERROR\n");
					//HANDLE
				}
				recv_len = mrecv_fill(r, content, content_len);
				if (content_len != recv_len) {
					printf("Getting content returned %llu of %llu\n", recv_len, content_len);
					//HANDLE
				}
				struct mowfile wfile;
				memset(&wfile, 0, sizeof(wfile));
				wfile.file_name = t_name;
				wfile.name_length = tmp_val;
				wfile.content_length = content_len;
				wfile.content = content;

				uint64_t nbcontent_len = hton64_t(content_len);
				printf("%s file name\n", t_name);

				if (MOWFILEERR == m_write_file(abs_folder_path, &wfile)) {
					printf("Can't create %s path %s file\n", abs_folder_path, t_name);
				}
				free(content);
			}
			else {
				//HANDLE
			}

		MD_GET_UPDATE_NEXT:
			free(t_name);
}
		if (distributor_main_folder_path) free(distributor_main_folder_path);
		distributor_main_folder_path_length = 0;
		if (abs_folder_path) free(abs_folder_path);
		if (t_abs_update_location) free(t_abs_update_location);
		mclose(&distributor_connection);
		//CHANGE BACK THE WORKING DIRECTORY
		if (md_before_wd) {
			printf("Changing path back to %s\n", md_before_wd);
			if (MOWFILEERR == m_set_current_dir(md_before_wd)) {
				printf("Can't change directory back to %s\n", md_before_wd);
			}
			free(md_before_wd);
		}
		else printf("Can't change directory back\n");

	}
	else {
		for (int i = 0; i < adapter_count; i++)
		{
			mclosesd(bsockets[i]->socketd);
			free(bsockets[i]);
		}
		free(bsockets);
		bsockets = NULL;
		if (0 == ip_list_size) {
			printf("There aren't any peers waiting for an update\n");
			goto MD_GET_UPDATE_RETURN;
		}
		//READ THE WANTED FOLDER
		struct mowfolder* parrent_folder = m_read_folder(bs->update_location);
		struct mowfile* android_lib = NULL;
		if (NULL == parrent_folder) {
			printf("md_get_update can't read update location\n");
			goto MD_GET_UPDATE_RETURN;
		}
		//CONNECT WITH EVERY PEER
		struct mowsocket** peer_connections = calloc(ip_list_size, sizeof(struct mowsocket*));
		if (NULL == peer_connections) {
			printf("md_get update can't allocate memory\n");
			m_free_folder(parrent_folder);
			goto MD_GET_UPDATE_RETURN;
		}
		struct mowadapter* t_adapter = cleanAdapters;
		for (uint32_t i = 0; i < ip_list_size; i++)
		{
			uint32_t ho_addr_to_use = 0;
			while (t_adapter) {
				uint32_t t = ip_list[i] | ~(t_adapter->h_netmask);
				if ((ip_list[0] | ~(t_adapter->h_netmask)) == t_adapter->h_broadcast)	ho_addr_to_use = t_adapter->h_address;
				t_adapter = t_adapter->next;
			}
			peer_connections[i] = msocket(MOW_IP4, MOW_TCP, MOW_SEND, ho_addr_to_use, MOWDISTRIBUTEPORT);
			if (NULL == peer_connections[i]) {
				printf("md_get_update can't create socket for %s address\n", ho_addr_to_str(ho_addr_to_use));
				//todo(kerem): HANDLE THIS ERROR
			}
			if (MOWSOCKETERR == msetsockpredopt(peer_connections[i], MOW_SO_DONTLINGER, 1)) {
				printf("md_get_update can't set sockopt MOW_SO_DONTLINGER for %d socket %s %d\n", i, __FILE__, __LINE__);
			}
			if (MOWSOCKETERR == mconnects(peer_connections[i], ip_list[i], MOWDISTRIBUTEPORT)) {
				printf("Can't connect to %s address\n", ho_addr_to_str(ip_list[i]));
			}
		}

		//SEND THE FOLDERS AND FILES
		if (MOWDISTRIBUTEOK == md_send_folder_recursively(parrent_folder, peer_connections, ip_list_size)) {
			printf("md_get_update distributed succesfully\n");
			for (uint32_t i = 0; i < ip_list_size; i++)
			{
				printf("Sending closing info to %s\n", ho_addr_to_str(ip_list[i]));
				char finished_info = 0;
				if (1 != msends(peer_connections[i], &finished_info, sizeof(finished_info))) {
					printf("Can't send finished info to %s address\n", ho_addr_to_str(ip_list[i]));
				}
			}
			for (uint32_t i = 0; i < ip_list_size; i++)
			{
				char finished_info = 0;
				if (sizeof(finished_info) == mrecv(peer_connections[i]->socketd, &finished_info, sizeof(finished_info))) {
					if (0 == finished_info) {
						printf("%s acked the close\n", ho_addr_to_str(ip_list[i]));

					}
					else if (5 == finished_info && NULL != bs->android_lib_location) {
						//CAN JUST COMPILE FOR ARM FOR NOW LATER IMPLEMENT THIS DIFFERENTLY
						if (NULL == android_lib) {
							m_set_current_dir(bs->update_location);
							if (0 != system("fips set config android-make-debug && fips build")) {	//todo(kerem): CHANGE
								printf("Can't build android app\n");
							}
							android_lib = m_read_file(bs->android_lib_location);
							if (NULL == android_lib) {
								printf("Can't read android_lib\n");
							}
						}
						if (NULL != android_lib) {
							//SEND LIB
							uint64_t len_no = hton64_t(android_lib->name_length);
							if (sizeof(len_no) != msends(peer_connections[i], &len_no, sizeof(len_no))) {
								printf("Can't send file name length %s address\n", ho_addr_to_str(ip_list[i]));
							}
							if (android_lib->name_length != msends(peer_connections[i], android_lib->file_name, android_lib->name_length)) {
								printf("Can't send file name %s address\n", ho_addr_to_str(ip_list[i]));
							}
							len_no = hton64_t(android_lib->content_length);
							if (sizeof(len_no) != msends(peer_connections[i], &len_no, sizeof(len_no))) {
								printf("Can't send content length %s address\n", ho_addr_to_str(ip_list[i]));
							}
							if (android_lib->content_length != msends(peer_connections[i], android_lib->content, android_lib->content_length)) {
								printf("Can't send content %s address\n", ho_addr_to_str(ip_list[i]));
							}
						}
					}
					else {
						printf("Peer %s is not responding normally to close\n", ho_addr_to_str(ip_list[i]));
					}
				}
				if (MOWSOCKETERR == mclose_option(&peer_connections[i], MCLOSE_RECEIVE)) {	//CLOSE READ(receive) OPERATION
					printf("Closing connection with %s address error occured\n", ho_addr_to_str(ip_list[i]));
				}
			}free(peer_connections);
		}
		else {
			printf("md_get_update can't distribute succesfully\n");
		}


		//FREE THE FOLDER STRUCT
		m_free_folder(parrent_folder);
	}



	retVal = MOWDISTRIBUTEOK;
MD_GET_UPDATE_RETURN:
	if (bsockets) {
		for (int i = 0; i < adapter_count; i++)
		{
			mclose(bsockets + i);
		}free(bsockets);
	}
	if (tmpAdapters) m_free_adapters(&tmpAdapters);
	if (cleanAdapters) m_free_adapters(&cleanAdapters);
	if (ip_list) free(ip_list);
	printf("md_get_update returning\n");
	return MOWDISTRIBUTEOK;
}
