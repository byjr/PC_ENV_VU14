#ifndef __RT_PLAYER_H_
#define __RT_PLAYER_H_
#include <lzUtils/alsa_ctrl/alsa_ctrl.h>
#include <thread>
#include <atomic>
class RePlayerPar{
public:
	alsa_args_t* pRecPar;
	alsa_args_t* pPlyPar;
	size_t mChunkTimeMs;
};
class RePlayer{
	RePlayerPar* mPar;
	alsa_ctrl_t* mRec;
	alsa_ctrl_t* mPly;
	std::thread mTrd;
	std::atomic<bool> mPauseFlag;
	size_t mChunkBytes;
public:
	RePlayer(RePlayerPar* par);
	~RePlayer();
	void pause();
	void resume();
};	
#endif