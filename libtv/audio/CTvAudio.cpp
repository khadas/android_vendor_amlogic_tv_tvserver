#define LOG_TAG "CTvAudio"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>

#include <android/log.h>
#include <cutils/properties.h>

#include "../tvsetting/CTvSetting.h"
#include "../tvutils/tvutils.h"
#include "../tvutils/CFile.h"
#include "audio_effect.h"
#include "CTvAudio.h"

#include "CTvLog.h"

CTvAudio::CTvAudio()
{
}

CTvAudio::~CTvAudio()
{
}

