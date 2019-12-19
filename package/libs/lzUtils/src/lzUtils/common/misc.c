#include <stdarg.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <ctype.h>
#include <syscall.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include "fp_op.h"
#include "fd_op.h"
#include "misc.h"
#include "../slog/slog.h"

size_t get_pids_by_name(const char *pidName,pid_t **pp_pid,size_t max){
	char filename[256];
	char name[128];
	struct dirent *next;
	FILE *file;
	DIR *dir;
	int fail = 1;
	pid_t *p_pid=NULL;
	dir = opendir("/proc");
	if (!dir) {
		return -1;
		perror("Cannot open /proc\n");
	}
	int count=0;
	for (count=0;(next = readdir(dir)) != NULL && count<max;) {
		/* Must skip ".." since that is outside /proc */
		if (strcmp(next->d_name, "..") == 0)
			continue;

		/* If it isn't a number, we don't want it */
		if (!isdigit(*next->d_name))
			continue;

		memset(filename, 0, sizeof(filename));
		sprintf(filename, "/proc/%s/status", next->d_name);
		if (!(file = fopen(filename, "r")))
			continue;

		memset(filename, 0, sizeof(filename));
		if (fgets(filename, sizeof(filename) - 1, file) != NULL) {
			/* Buffer should contain a string like "Name:   binary_name" */
			sscanf(filename, "%*s %s", name);
			if (!strcmp(name, pidName)) {
				if(pp_pid){
					if ((p_pid = realloc(p_pid, sizeof(pid_t) * (count + 1))) == NULL){
						s_err("realloc fail!");
						fclose(file);
						goto exit;
					}
					p_pid[count] = strtol(next->d_name, NULL, 0);					
				}
				count++;
			}
		}
		fail=0;
		if(pp_pid) *pp_pid=p_pid;
		fclose(file);
	}
exit:	
	closedir(dir);
	if(fail && count<=0)return -1;	
	return count;
}
int pkill(const char *name, int sig){
	int ret=0;
	pid_t *p_pid=NULL;
	if(!name){
		s_err("name:%s is invalid!",name);
		return -1;
	}
    size_t count=get_pids_by_name(name,&p_pid,1);
	if(!(count>0&&p_pid)){
		s_err("get_pids_by_name %s failure!",name);
		return -2;
	}
	ret=kill(p_pid[0],sig);
	FREE(p_pid);
	if(ret<0)return -3;
    return 0;
}
static pthread_mutex_t popen_mtx = PTHREAD_MUTEX_INITIALIZER;
int my_popen(const char *fmt,...){
    char buf[MAX_USER_COMMAND_LEN]="";
    char cmd[MAX_USER_COMMAND_LEN]="";
    FILE *pfile;
    int status = -2;
    pthread_mutex_lock(&popen_mtx);
	va_list args;    
	va_start(args,fmt);
	vsprintf(cmd,fmt,args);
	va_end(args);
//  	s_inf("%s",cmd);	
    if ((pfile = popen(cmd, "r"))) {
        fcntl(fileno(pfile), F_SETFD, FD_CLOEXEC);
        while(!feof(pfile)) {
            fgets(buf, sizeof buf, pfile);
        }
//		s_inf("%s",buf);
        status = pclose(pfile);
    }
    pthread_mutex_unlock(&popen_mtx);
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return -1;
}
int my_popen_get(char *rbuf, int rbuflen, const char *cmd, ...){
	char buf[MAX_USER_COMMAND_LEN];
	va_list args;
	FILE *pfile;
	int status = -2;
	char *p = rbuf;

	rbuflen = (!rbuf) ? 0 : rbuflen;

	va_start(args, (char *)cmd);
	vsnprintf(buf, sizeof(buf), cmd, args);
	va_end(args);

	pthread_mutex_lock(&popen_mtx);
	if ((pfile = popen(buf, "r"))) {
		fcntl(fileno(pfile), F_SETFD, FD_CLOEXEC);
		while (!feof(pfile)) {
			if ((rbuflen > 0) && fgets(buf, CCHIP_MIN(rbuflen, sizeof(buf)), pfile)) {
				int len = snprintf(p, rbuflen, "%s", buf);
				rbuflen -= len;
				p += len;
			}
			else {
				break;
			}
		}
		if ((rbuf) && (p != rbuf) && (*(p - 1) == '\n')) {
			*(p - 1) = 0;
		}
		status = pclose(pfile);
	}
	pthread_mutex_unlock(&popen_mtx);
	if (WIFEXITED(status)) {
		return WEXITSTATUS(status);
	}
	return -1;
}
void argv_free(char *argv[]){
	int i=0;
	for(i=0;argv[i];i++){
		free(argv[i]);
	}
	free(argv);
	argv=NULL;
}
char *argv_to_argl(char *argv[]){
	int i=0,rt_len=0,rt_idx=0;
	char *argl=NULL;
	if(!(argv && argv[0]))return NULL;
	for(i=0;;i++){
		if(!argv[i])break;
		rt_len=strlen(argv[i]);
		rt_idx+=rt_len;				
		argl=(char*)realloc(argl,rt_idx+2);
		if(!argl)return NULL;
		strcat(argl,argv[i]);
		argl[rt_idx++]=' ';
	}
	argl[rt_idx-1]='\0';
	return argl;
}
#define match_char(ch) (ch==' '||ch=='\0'||ch=='\n')
char** argl_to_argv(char argl[],int *pArgc){
	int i=0,count=0;
	if(!(argl && argl[0]))return NULL;
	char **argv=(char**)calloc(1,sizeof(char*));
	if(!argv)return NULL;
	argv[0]=argl;
	for(i=0;;i++){
		if(match_char(argl[i])){
			while(argl[i] ==' ' && argl[i+1] == ' '){
				i++;
			}
			count++;
			argv=(char**)realloc(argv,(count+1)*sizeof(char *));
			if(!argv)return NULL;
			argv[count]=argl+i+1;
			char *arg=calloc(1,argv[count]-argv[count-1]);
			if(!arg){
				argv_free(argv);
				return NULL;
			}
			strncpy(arg,argv[count-1],argv[count]-argv[count-1]-1);
			argv[count-1]=arg;
		}
		if(argl[i]=='\0'||argl[i]=='\n')break;
	}
	argv[count]=NULL;
	if(pArgc){
		*pArgc=count;
	}
	return argv;
}

int vfexec(char *argl,char isBlock){
	int ret=-1;
	char **argv=argl_to_argv(argl,NULL);
	if(!(argv && argv[0]))goto exit;
	pid_t pid=vfork();
	if(pid<0)goto exit;
	if(pid==0){
		int ret=execvp(argv[0],argv);
		if(ret<0){
			show_errno(0,"execv");
			goto exit;
		}
	}else{
		int ws=0;
		int ret=isBlock?pid=waitpid(pid,&ws,0):waitpid(pid,&ws,WNOHANG);
		if(ret<0 && errno!=ECHILD){
			show_errno(0,"waitpid");
			goto exit;
		}
		ret=0;
	}
exit:
	if(argv)argv_free(argv);
	return ret<0?-1:0;
}

int fexec(char *argl,char isBlock){
	int ret=-1;
	char **argv=argl_to_argv(argl,NULL);
	if(!(argv && argv[0]))goto exit;
	pid_t pid=fork();
	if(pid<0)goto exit;
	if(pid==0){
		int ret=execvp(argv[0],argv);
		if(ret<0){
			show_errno(0,"execv");
			goto exit;
		}
	}else{
		int ws=0;
		int ret=isBlock?pid=waitpid(pid,&ws,0):waitpid(pid,&ws,WNOHANG);
		if(ret<0 && errno!=ECHILD){
			show_errno(0,"waitpid");
			goto exit;
		}
		ret=0;
	}
exit:
	if(argv)argv_free(argv);
	return ret<0?-1:0;
}

//编码目标字符集
static const char * base64Char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"; 
//长度关系 (y：输出长度，x:输入长度):y=(x%3)?(x/3+1)*4:x/3*4 考虑传输效率将每次编解码的二进制字节数暂定为：(1024/4-1)*3=765
size_t base64Encode(char *bin,size_t binSize,char *base64,size_t baseSize){
    int i, j;
    char current;
    for ( i = 0, j = 0 ; i < binSize ; i += 3 ){
		if(j+4 >= baseSize){
			s_err("out buf is't enough,alredy en %u bytes,output %u bytes !",i,j);
			base64[j] = '\0';
			return -1;
		}
        current = (bin[i] >> 2) ;
        current &= (char)0x3F;
        base64[j++] = base64Char[(int)current];

        current = ( (char)(bin[i] << 4 ) ) & ( (char)0x30 ) ;
        if ( i + 1 >= binSize ){
            base64[j++] = base64Char[(int)current];
            base64[j++] = '=';
            base64[j++] = '=';
            break;
        }
        current |= ( (char)(bin[i+1] >> 4) ) & ( (char) 0x0F );
        base64[j++] = base64Char[(int)current];

        current = ( (char)(bin[i+1] << 2) ) & ( (char)0x3C ) ;
        if ( i + 2 >= binSize ){
            base64[j++] = base64Char[(int)current];
            base64[j++] = '=';
            break;
        }
        current |= ( (char)(bin[i+2] >> 6) ) & ( (char) 0x03 );
        base64[j++] = base64Char[(int)current];

        current = ( (char)bin[i+2] ) & ( (char)0x3F ) ;
        base64[j++] = base64Char[(int)current];
    }
    base64[j] = '\0';
    return j;
}
size_t base64Decode(char *base64,size_t baseSize,char * bin,size_t binSize){
    int i, j;
    char k;
    char temp[4];
	if((baseSize/4-1)*3>binSize){
		s_err("out buf is't enough,alredy encode %u bytes,output %u bytes !",i,j);
		return -1;
	}
    for ( i = 0, j = 0;i < baseSize ; i += 4 ){
        memset( temp, 0xFF, sizeof(temp) );
        for ( k = 0 ; k < 64 ; k ++ ){
            if ( base64Char[k] == base64[i] )
                temp[0]= k;
        }
        for ( k = 0 ; k < 64 ; k ++ ){
            if ( base64Char[k] == base64[i+1] )
                temp[1]= k;
        }
        for ( k = 0 ; k < 64 ; k ++ ){
            if ( base64Char[k] == base64[i+2] )
                temp[2]= k;
        }
        for ( k = 0 ; k < 64 ; k ++ ){
            if ( base64Char[k] == base64[i+3] )
                temp[3]= k;
        }

        bin[j++] = ((char)(((char)(temp[0] << 2))&0xFC)) |
                ((char)((char)(temp[1]>>4)&0x03));
        if ( base64[i+2] == '=' )
            break;

        bin[j++] = ((char)(((char)(temp[1] << 4))&0xF0)) |
                ((char)((char)(temp[2]>>2)&0x0F));
        if ( base64[i+3] == '=' )
            break;

        bin[j++] = ((char)(((char)(temp[2] << 6))&0xF0)) |
                ((char)(temp[3]&0x3F));
    }
    return j;
}
int unique_process_lock(char *name){
	char buf[1024]="";
	snprintf(buf,sizeof(buf),"/var/run/%s.pid",get_last_name(name));
	int fd = open(buf, O_WRONLY | O_CREAT, 0666); 
	if (fd < 0) { 
		s_err("Fail to open %s",buf);
		exit(1); 
	} 
	struct flock lock ={0}; 
	if (fcntl(fd, F_GETLK, &lock) < 0) {
		s_err("Fail to fcntl F_GETLK");
		exit(1); 
	} 
	lock.l_type = F_WRLCK; 
	lock.l_whence = SEEK_SET;
	if (fcntl(fd, F_SETLK, &lock) < 0) { 
		s_err("Fail to fcntl F_SETLK"); 
		exit(1); 
	}
	bzero(buf,sizeof(buf));
	snprintf(buf,sizeof(buf),"%d",getpid());
	write(fd,buf,strlen(buf));
	return 0;
}


