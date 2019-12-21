#ifdef __cplusplus
extern "C" {
#endif

#ifndef VAD_OP_H_
#define VAD_OP_H_

#define BIG_ENDIAN 0
typedef enum vad_status_t{
	VAD_STATUS_MIN=-1,
	VAD_CHECK_IDEL,
	VAD_CHECK_STARTED,
	VAD_CHECK_SPEAKING,
	VAD_CHECK_FINISHED,
	VAD_CHECK_DONE,
	VAD_STATUS_MAX
}vad_status_t;

typedef struct vad_args_t{
	short *sample;
	unsigned int bytes;
	volatile unsigned char savedStatus;
	unsigned char confirmCount;
	unsigned int vadThreshold;
	unsigned char startedConfirmCount;
	unsigned char finishedConfirmCount;
	unsigned int unitBytes;
}vad_args_t;

vad_status_t get_vad_status(vad_args_t *args);

#endif

#ifdef __cplusplus
}
#endif
