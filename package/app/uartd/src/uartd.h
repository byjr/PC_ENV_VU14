#ifndef AML_UARTD_H_
#define AML_UARTD_H_
#include <stdio.h>
#include <string>
#include <vector>
#include <thread>
#include <stdlib.h>
#include "condFifo.h"
class ItemPar{
public:	
	int argc;
	char ** argv;
	void *args;
};
class CmdItem{
public:
	char *name;
	int (*handle)(ItemPar& par);
	void *args;
};
class UartdPar{
public:
	char* devNode;
	size_t baudRate;
	size_t openMode;
	bool ttMasterFlag;
	char* ttHardPath;
};
class Uartd{
	UartdPar* mPar;
	int mUartdFd;
	int mHardWriteFd;
	int mHardReadFd;
	CondFifo* wFifo;
	CondFifo* rFifo;
	std::thread rUartThread;
	std::thread wUartThread;
	std::thread parseThread;
	std::thread ttMasterThread;
	int isHardReadable();
	int isHardWriteable();
	int setOpt(int nSpeed, int nBits, char nEvent, int nStop);
	std::vector<CmdItem> mCmdVct;
	volatile bool ClientExitFlag;
public:	
	Uartd(UartdPar* par);
	~Uartd();
	int HardReadProcess();
	int HardWriteProcess();
	int CmdparseProcess();
	int TtMasterProcess();
	int sendCmd(const char *cmd);
	void setExitFlag(){
		ClientExitFlag = true;
	}
	bool getExitFlag(){
		return ClientExitFlag;
	}	
};
#endif
