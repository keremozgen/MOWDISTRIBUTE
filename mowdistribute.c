#include "mowdistribute.h"

int main() {
#if defined(_WIN32) && defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
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
	bs.broadcast_interval_ms = 1000;

#if defined(_WIN32)
	bs.action = MD_DISTRIBUTE;
	bs.waiting_limit_seconds = 2;
#ifdef _WIN32
	bs.update_location = "D:\\PROJECTS\\MOWDISTRIBUTE";
#else
	bs.update_location = "/home/hp/MOWDISTRIBUTE";
#endif
#else
	bs.action = MD_GET;
	bs.waiting_limit_seconds = 0;
	bs.update_location = "../../../DENEME11";
#endif


	mowthread bthread = mthread(md_get_update, &bs);


	mthread_join(bthread, NULL);
#if defined(_WIN32) && defined(_DEBUG)

#endif
	return 0;
}