#include "CTvDmx.h"
CTvDmx::CTvDmx()
{
    mDmxDevId = 0;
}

CTvDmx::~CTvDmx()
{
}

int CTvDmx::Open(AM_DMX_OpenPara_t &para)
{
    return AM_DMX_Open ( mDmxDevId, &para );
}

int CTvDmx::Close()
{
    return     AM_DMX_Close ( mDmxDevId );
}

int CTvDmx::SetSource(AM_DMX_Source_t source)
{
    return AM_DMX_SetSource ( mDmxDevId, source );
}
