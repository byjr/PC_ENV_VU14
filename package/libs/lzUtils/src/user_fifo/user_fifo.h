#ifndef UARTD_FIFO_H_
#define UARTD_FIFO_H_ 1
#include "../un_fifo_ops/un_fifo_ops.h"
typedef struct item_arg_t{
	int argc;
	char ** argv;
	void * args;
}item_arg_t;

typedef struct cmd_item_t{
	const char *cmd;
	int (*handle)(item_arg_t *);
	void *args;
}cmd_item_t;

typedef enum fifo_cmd_type_t{
	FIFO_CMD_TYPE_MIN,
	FIFO_CMD_UNSPECIFIED_FIFO,	//不指定处理函数的有名
	FIFO_CMD_UNSPECIFIED_PIPE,	//不指定处理函数的无名
	FIFO_CMD_SPECIFIED_FIFO,	//指定处理函数的有名
	FIFO_CMD_SPECIFIED_PIPE,	//指定处理函数的无名
	FIFO_CMD_TYPE_MAX
}fifo_cmd_type_t;

typedef struct fifo_cmd_t{
	fifo_cmd_type_t type;
	char* path;
	cmd_item_t* tbl;
	int fd;
	int (*handle)(char *,int);
	unixFifoOps_t *ops_ptr;
}fifo_cmd_t;
int user_fifo_write_fmt_len(unixFifoOps_t *ptr,size_t len,const char *fmt,...);
unixFifoOps_t *user_fifo_write_init(const char *path);
int user_fifo_write_str(unixFifoOps_t *ptr,char *contex);
int user_fifo_write_fmt(unixFifoOps_t *ptr,const char *fmt,...);
int user_fifo_read_proc(fifo_cmd_t *pArgs);
int user_fifo_read_init(fifo_cmd_t *pArgs);

#define userFifoWriteFmt(path,x...) ({\
	unixFifoOps_t *userFifoPtr=user_fifo_write_init(path);\
	if(userFifoPtr){\
		int ret=user_fifo_write_fmt(userFifoPtr,x);\
		if(ret<0){\
			show_errno(0,"user_fifo_write_fmt");\
		}\
		un_fifo_ops_destory(userFifoPtr);\
	}\
})
#endif