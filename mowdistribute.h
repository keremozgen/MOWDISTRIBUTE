#include "MOWSOCKET/mowsocket.h"
#include "MOWFILE/mowfile.h"
#include "MOWTHREAD/mowthread.h"
#if (AES == 1)
#include "meow_hash/meow_hash_x64_aesni.h"
#endif

//GENERAL DEFINES HERE
#define MOWDISTRIBUTEOK 1
#define MOWDISTRIBUTERR 0

#define MD_GET 2		//FIND GOOD NAMES
#define MD_DISTRIBUTE 3	//FIND GOOD NAMES

#define MOWBROADCASTPORT 8888
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
	char* message;
	uint16_t message_len;
	volatile int* cond;
	uint16_t broadcast_interval_ms;
	int action;
	char* update_location;
	uint64_t waiting_limit_seconds;
};

//DEFINE MOWDISTRIBUTE FUNCTIONS HERE

int md_get_update(struct mdBroadcast* bs);

unsigned int compare_ip_with_adapters(uint32_t address_ho, struct mowadapter* adapters);

unsigned int compare_ip_with_uint32_t_p(uint32_t address_ho, uint32_t* address_p);

int md_send_folder_recursively(struct mowfolder* folder, struct mowsocket** sockets, uint32_t socket_count);

int md_get_folder(int64_t socketd, char* loocation);

//IMPLEMENT MOWDISTRIBUTE FUNCTIONS HERE

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

int md_get_folder(int64_t socketd, char* location) {
	char info = 0;
	uint64_t recv_nb = 1;
	if (1 != mrecv(socketd, &info, sizeof(char))) {
		printf("md_get_folder can't get folder info.%s %d\n", __FILE__, __LINE__);
		return MOWDISTRIBUTERR;
	}
	if (1 != info) {
		printf("md_get_folder distributor is not sending in order\n");
		return MOWDISTRIBUTERR;
	}
	char* md_cwd = m_get_current_dir();
	if (NULL == md_cwd) {
		printf("md_get_folder can't get current folder\n");
		return MOWDISTRIBUTERR;
	}
	if (MOWFILEERR == m_set_current_dir(location)) {
		printf("md_get_folder can't set current directory\n");
		return MOWDISTRIBUTERR;
	}

	while (0 < recv_nb) {
		if (1 == recv_nb) {			//FILE OR FOLDER
			if (1 == info) {		//FILE
				uint64_t file_name_length = 0, file_content_length = 0;
				if (sizeof(file_name_length) == mrecv(socketd, &file_name_length, sizeof(file_name_length)))
				{
					file_name_length = ntoh64_t(file_name_length);
					char* file_name = (char*)calloc(file_name_length + 1, sizeof(char));
					if (NULL == file_name) {

					}
					if (file_name_length != mrecv(socketd, file_name, file_name_length)) {

					}
				}
				if (sizeof(file_content_length) == mrecv(socketd, &file_content_length, sizeof(file_content_length))) {
					file_content_length = ntoh64_t(file_content_length);
					char* file_content = (char*)calloc(file_content_length + 1, sizeof(char));
					if (NULL == file_content) {

					}
					uint64_t content_length_received = mrecv_fill(socketd, file_content, file_content_length);
					if (file_content_length != content_length_received) {
						printf("File content length was expected %llu but %llu came instead\n", file_content_length, content_length_received);
					}
					printf("File content:\n%s\n", file_content);
				}

			}
			else if (2 == info) {	//FOLDER
				uint64_t folder_name_length = 0;
				if (sizeof(folder_name_length) == mrecv(socketd, &folder_name_length, sizeof(folder_name_length))) {
					folder_name_length = ntoh64_t(folder_name_length);
					char* folder_name = (char*)calloc(folder_name_length + 1, sizeof(char));
					if (NULL == folder_name) {

					}
					if (folder_name_length != mrecv(socketd, folder_name, folder_name_length)) {

					}
				}
			}
			else {					//UNEXPECTED
				printf("Unexpected parameter from distributor\n");
			}
		}

	}
	m_set_current_dir(md_cwd);
	free(md_cwd);
	printf("md_get_folder returning\n");
}

#if (AES == 1)
static void
PrintHash(meow_u128 Hash)
{
	printf("    %08X-%08X-%08X-%08X\n",
		MeowU32From(Hash, 3),
		MeowU32From(Hash, 2),
		MeowU32From(Hash, 1),
		MeowU32From(Hash, 0));
}
#endif

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

#if (AES == 1)
		meow_u128 fileHash = MeowHash(MeowDefaultSeed, folder->files[i]->content_length, folder->files[i]->content);
		PrintHash(fileHash);
#endif

		for (uint32_t j = 0; j < socket_count; j++)
		{
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
			if (folder->files[i]->content_length != msends(sockets[j], folder->files[i]->content, folder->files[i]->content_length)) {
				printf("Sending file content failed.%s %d\n", __FILE__, __LINE__);
			}
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
	while (stored_cond == ((NULL == bs->cond) ? stored_cond : *bs->cond)) {
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
				tadapter = tadapter->next;
			}
			free(distributor_msg);
		}
		uint64_t delta = 0;
		double interval = 0.0f;
		uint16_t elapsed_time = 0;
		while (elapsed_time < bs->broadcast_interval_ms) {
			for (int i = 0; i < adapter_count; i++)
			{
				time_b = mprecise_time_of_day_ns();
				if (MOWSOCKETOK == msetsockpredopt(bsockets[i], MOW_SO_RECVTIMEO, bs->broadcast_interval_ms / adapter_count)) {
					uint32_t peer_addr_ho = 0;
					uint16_t peer_port_ho = 0;
					uint64_t rnb = mrecvfrom(bsockets[i]->socketd, message_buf, bs->message_len + 1, &peer_addr_ho, &peer_port_ho);
					if (rnb)
						if (0 == compare_ip_with_adapters(peer_addr_ho, tmpAdapters) && 0 != peer_addr_ho) {
							if (0 == rnb) {
								printf("md_get_update recv returned 0 %s %d\n", __FILE__, __LINE__);
							}
							if (0 == memcmp(bs->message, message_buf, bs->message_len) && (MD_GET == bs->action) ? ((rnb == (bs->message_len + 1) && 1 == message_buf[bs->message_len])) : (rnb == (bs->message_len))) {
								if (0 == compare_ip_with_uint32_t_p(peer_addr_ho, ip_list)) {
									//UPDATE THE LIST
									ip_list_size++;
									uint32_t* tp = (uint32_t*)realloc(ip_list, sizeof(uint32_t) * ip_list_size + 1);
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
				interval = (double)((double)((double)bs->broadcast_interval_ms - (double)delta) / (double)adapter_count);
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
			recv_len = mrecv(r, &info, sizeof(char));
			if (0 == info) {
				printf("Distributor closing\n");
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
			if (0 == info) {
				printf("All files are taken from distributor\n");
				break;
			}
			uint64_t tmp_val = 0;
			recv_len = mrecv(r, &tmp_val, sizeof(tmp_val));
			if (sizeof(tmp_val) != recv_len) {
				printf("Getting value returned %llu\n", tmp_val);
				//HANDLE
			}
			tmp_val = ntoh64_t(tmp_val);
			char* t_name = (char*)calloc(tmp_val + 1, sizeof(char));
			if (NULL == t_name) {
				printf("Can't allocate memory for name\n");
				//HANDLE
			}
			recv_len = mrecv(r, t_name, tmp_val);
			if (tmp_val != recv_len) {
				printf("Getting name returned %llu of %llu\n", recv_len, tmp_val);
				//HANDLE
			}
			if (2 == info) {
				if (NULL == distributor_main_folder_path) {
					distributor_main_folder_path = t_name;
					distributor_main_folder_path_length = strlen(distributor_main_folder_path);
					if (tmp_val != distributor_main_folder_path_length) {	//todo(kerem): Handle error
						printf("Error on distributor main_folder_path length. Should be %llu but %llu\n", tmp_val, distributor_main_folder_path_length);
					}
				}
				relative_folder_path = t_name + distributor_main_folder_path_length + 1;	//+1 FOR FILE DELIMITER
				if (abs_folder_path) free(abs_folder_path);
				abs_folder_path = (char*)calloc(strlen(relative_folder_path) + 1 + ((t_abs_update_location) ? strlen(t_abs_update_location) : strlen(bs->update_location)), sizeof(char));
				if (NULL == abs_folder_path) {	//todo(kerem): HANDLE ERROR

				}
				memcpy(abs_folder_path, (t_abs_update_location) ? t_abs_update_location : bs->update_location, ((t_abs_update_location) ? strlen(t_abs_update_location) : strlen(bs->update_location)));
				abs_folder_path[((t_abs_update_location) ? strlen(t_abs_update_location) : strlen(bs->update_location))] = M_OS_DELIMITER_CHAR;
				memcpy(abs_folder_path + 1 + ((t_abs_update_location) ? strlen(t_abs_update_location) : strlen(bs->update_location)), relative_folder_path, strlen(relative_folder_path));
				m_path_conv_compat(abs_folder_path);
				if (MOWFILEERR == m_dir_exist(abs_folder_path)) {
					if (MOWFILEERR == m_create_dir(abs_folder_path)) {
						printf("Can't create %s folder\n", abs_folder_path);
					}
					else {
						printf("%s folder created\n", abs_folder_path);
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
				char* content = (char*)calloc(content_len + 1, sizeof(char));
				if (NULL == content) {
					printf("Can't allocate memory for content\n");
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
#if (AES == 1)
				meow_u128 fileHash = MeowHash(MeowDefaultSeed, content_len, content);
				PrintHash(fileHash);
#endif
				if (MOWFILEERR == m_write_file(abs_folder_path, &wfile)) {
					printf("Can't create %s path %s file\n", abs_folder_path, t_name);
				}
				free(content);
			}
			else {
				//HANDLE
			}


			free(t_name);
		}
		if (abs_folder_path) free(abs_folder_path);
		if (t_abs_update_location) free(t_abs_update_location);
		mclose(&distributor_connection);
		//CHANGE BACK THE WORKING DIRECTORY
		if (md_before_wd) {
			printf("Changing path back to %s\n", md_before_wd);
			if (MOWFILEERR == m_set_current_dir(md_before_wd)) {
				printf("Can't change directory back to %s\n", md_before_wd);
			}
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
				if (1 != mrecv(peer_connections[i]->socketd, &finished_info, sizeof(finished_info))) {
					if (0 != finished_info) {
						printf("Peer %s is not responding normally to close\n", ho_addr_to_str(ip_list[i]));
					}
					else {
						printf("%s acked the close\n", ho_addr_to_str(ip_list[i]));
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
