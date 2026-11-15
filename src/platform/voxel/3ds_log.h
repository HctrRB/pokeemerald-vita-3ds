#ifndef _3DS_LOG_H_VOXEL_
#define _3DS_LOG_H_VOXEL_

#include "../vita/vita_platform.h"

#define CTR_LOG_VIDEO 0
#define CtrLog_Write(tag, fmt, ...) VITA_LOG("[VOXEL] " fmt "\n", ##__VA_ARGS__)

#endif // _3DS_LOG_H_VOXEL_
