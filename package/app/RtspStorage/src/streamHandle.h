#include <vector>
class anyFrame{
	char *mBuf;
	size_t mSize;
public:	
	anyFrame(char *buf,size_t size){
		mBuf = new char[size];
		if(!mBuf){
			return ;
		}
		mSize = size;
		memcpy(mBuf,buf,mSize);		
	}
	~anyFrame(){
		delete[]mBuf;
	}
	char *getBuf(){
		return mBuf;
	}
	size_t getSize(){
		return mSize;
	}	
};
enum class anyStatus{
	SUCCEED,
	FAILED,
	DONE,
};
class anyStream{
	std::vector<anyFrame*> mVct;
	size_t mMax;
	size_t ri;
	size_t wi;
public:
	anyStream(size_t max){
		mMax = max;	
		for(int i = 0; i < mMax;i ++){
			mVct.push_back(NULL);
		}		 
	}
	anyFrame* getOne(){
		if(wi == ri){
			return NULL;
		}
		return mVct[ri];
	}
	void getSync(){
		delete mVct[ri];
		mVct[ri] = NULL;
		ri = (ri < mMax) ? (ri +1) : 0;	
	}	
	bool readOne(char **pBuf,size_t *pSize){
		auto frame = getOne();
		if(!frame){
			return false;
		}
		memcpy(*pBuf,frame->getBuf(),frame->getSize());
		*pSize = frame->getSize();
		getSync();
		return true;
	}
	bool cycWrite(char *buf,size_t size){	
		if(wi >= ri){
			delete mVct[ri];
			mVct[ri] = NULL;
			ri = (ri < mMax) ? (ri +1) : 0;
		}
		auto frame = new anyFrame(buf,size);
		if(!frame){
			s_err(__func__);
			return false;
		}	
		wi = (wi < mMax) ? (wi +1) : 0;
		mVct[wi]=frame;
		return true;
	}
	anyStatus writeOne(char *buf,size_t size){
		auto frame = new anyFrame(buf,size);
		if(!frame){
			s_err(__func__);
			return anyStatus::FAILED;
		}		
		if(wi >= ri){
			return anyStatus::DONE;
		}
		wi = (wi < mMax) ? (wi +1) : 0;
		mVct[wi]=frame;
		return anyStatus::SUCCEED;
	}
	void clear(){
		for(int i = 0;i < mMax; i++){
			if(mVct[i]){
				delete mVct[i];
				mVct[i]=NULL;
			}
		}
		wi = ri = 0;
	}
	~anyStream(){
		for(int i = 0; i < mMax;i ++){
			mVct.pop_back();
		}	
	} 
};
class BAStream{
	anyStream *befor;
	anyStream *after;
	bool is_befor_read_done;
public:	
	BAStream(size_t max){
		befor = new anyStream(max);
		after = new anyStream(max);
		is_befor_read_done = false;
	}
	~BAStream(){
		delete befor;
		delete after;
	}
	bool writeBefor(char *buf,size_t size){
		return befor->cycWrite(buf,size);
	}
	anyStatus writeAfter(char *buf,size_t size){
		return befor->writeOne(buf,size);
	}
	bool readOne(char **pBuf,size_t *pSize){
		if(!is_befor_read_done){
			if(!befor->readOne(pBuf,pSize)){
				is_befor_read_done = true;
				return after->readOne(pBuf,pSize);
			}
			return true;
		}else{
			return after->readOne(pBuf,pSize);
		}
	}
};