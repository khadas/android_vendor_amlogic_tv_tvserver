#define LOG_TAG "tvserver"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>

#include <android/log.h>
#include <cutils/properties.h>

#include "../tvsetting/CTvSetting.h"
#include <tvutils.h>
#include <CFile.h>
#include "audio_effect.h"
#include "CTvAudio.h"

#include "CTvLog.h"

CTvAudio::CTvAudio()
{
}

CTvAudio::~CTvAudio()
{
}

