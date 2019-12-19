#ifndef __RT_PLAYER_H_
#define __RT_PLAYER_H_
#include <lzUtils/alsa_ctrl/alsa_ctrl.h>
#include <cppUtils/MTQueue/MTQueue.h>
#include <thread>
class RtPlayerPar{
public:
	alsa_args_t* pRecPar;
	alsa_args_t* pPlyPar;
	MTQueuePar* pMTQPar;
};
#define PPERIOD_TMIE_MS 10
#define PERIOD_BYTES ((48 * 2 * 2) * PPERIOD_TMIE_MS)
class RtPlayer{
	RtPlayerPar* mPar;
	alsa_ctrl_t* mRec;
	alsa_ctrl_t* mPly;
	std::thread mPlyTrd;
	std::thread mRecTrd;
	MTQueue* mMTQ;
public:
	RtPlayer(RtPlayerPar* par);
	~RtPlayer();
};	
#endif