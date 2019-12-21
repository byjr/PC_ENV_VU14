#ifndef __RT_PLAYER_H_
#define __RT_PLAYER_H_
#include <lzUtils/alsa_ctrl/alsa_ctrl.h>
#include <cppUtils/MTQueue/MTQueue.h>
#include <thread>
#include <atomic>
class RtPlayerPar{
public:
	alsa_args_t* pRecPar;
	alsa_args_t* pPlyPar;
	MTQueuePar* pMTQPar;
	size_t mChunkTimeMs;
};
class RtPlayer{
	RtPlayerPar* mPar;
	alsa_ctrl_t* mRec;
	alsa_ctrl_t* mPly;
	std::thread mPlyTrd;
	std::thread mRecTrd;
	MTQueue* mMTQ;
	std::atomic<bool> mPauseFlag;
	std::atomic<bool> mFullFlag;
	size_t mChunkBytes;
public:
	RtPlayer(RtPlayerPar* par);
	~RtPlayer();
	void pause();
	void resume();
};	
#endif