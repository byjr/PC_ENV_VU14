#ifndef __RT_PLAYER_H_
#define __RT_PLAYER_H_
#include <lzUtils/alsa_ctrl/alsa_ctrl.h>
#include <thread>
class RePlayerPar{
public:
	alsa_args_t* pRecPar;
	alsa_args_t* pPlyPar;
};
#define PPERIOD_TMIE_MS 10
#define PERIOD_BYTES ((48 * 2 * 2) * PPERIOD_TMIE_MS)
class RePlayer{
	RePlayerPar* mPar;
	alsa_ctrl_t* mRec;
	alsa_ctrl_t* mPly;
	std::thread mTrd;
	volatile bool isPauseFlag;
public:
	RePlayer(RePlayerPar* par);
	~RePlayer();
	void pause();
	void resume();
};	
#endif