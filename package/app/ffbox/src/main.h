#ifdef __cplusplus
extern "C" {
#endif
#ifndef __MAIN_H__
#define __MAIN_H__ 1

#define ADD_APP_ITEM(name,args) {#name,name##_main,args},

typedef struct app_item_t{
	char *app_name;
	int (*app_main)(int argc, char *argv[]);
	void *args;
}app_item_t;

#endif
#ifdef __cplusplus
	}
#endif