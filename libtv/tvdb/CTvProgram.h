//
//
//  amlogic 2013
//
//  @ Project : tv
//  @ File Name : CTvProgram.h
//  @ Date : 2013-11
//  @ Author :
//
//

#if !defined(_CTVPROGRAM_H)
#define _CTVPROGRAM_H

#include "CTvDatabase.h"
#include "CTvChannel.h"
#include <utils/String8.h>
#include <utils/RefBase.h>
#include <stdlib.h>
#include "CTvLog.h"
using namespace android;
// Program对应ATV中的一个频道，DTV中的一个service
class CTvEvent;
class CTvProgram : public LightRefBase<CTvProgram> {
public:
    /*this type value is link to  enum AM_SCAN_ServiceType in am_scan.h
     * enum AM_SCAN_ServiceType
    {
        AM_SCAN_SRV_UNKNOWN     = 0,    < 未知类型
        AM_SCAN_SRV_DTV         = 1,    < 数字电视类型
        AM_SCAN_SRV_DRADIO      = 2,    < 数字广播类型
        AM_SCAN_SRV_ATV         = 3,    < 模拟电视类型
    };
    */
    /**未定义类型*/
    static const int TYPE_UNKNOWN = 0;
    /**电视节目*/
    static const int TYPE_TV    =  4;
    /**广播节目*/
    static const int TYPE_RADIO =  2;
    /**模拟节目*/
    static const int TYPE_ATV   =  3;
    /**数据节目*/
    static const int TYPE_DATA  =  5;
    /**数字节目*/
    static const int TYPE_DTV   = 1 ;
    /** PVR/Timeshifting playback program*/
    static const int TYPE_PLAYBACK =  6;

    static const int PROGRAM_SKIP_NO = 0;
    static const int PROGRAM_SKIP_YES = 1;
    static const int PROGRAM_SKIP_UNKOWN = 2;

    /**
     *Service中的基础元素信息
     */
public:
    class Element {
    private :
        int mpid;

    public :
        Element(int pid)
        {
            this->mpid = pid;
        }
        /**
         *取得基础元素的PID
         *@return 返回PID
         */
        int getPID()
        {
            return mpid;
        }
    };



    /**
     *多语言基础元素信息
     */
public:
    class MultiLangElement : public Element {
    private :
        String8 mlang;

    public :
        MultiLangElement(int pid, String8 lang): Element(pid)
        {
            this->mlang = lang;
        }

        /**
         *取得元素对应的语言
         *@return 返回3字符语言字符串
         */
        String8 getLang()
        {
            return mlang;
        }
    };



    /**
    *视频信息
    */
public :
    class Video : public Element {
    public:
        /**MPEG1/2*/
        static const int FORMAT_MPEG12 = 0;
        /**MPEG4*/
        static const int FORMAT_MPEG4  = 1;
        /**H.264*/
        static const int FORMAT_H264   = 2;
        /**MJPEG*/
        static const int FORMAT_MJPEG  = 3;
        /**Real video*/
        static const int FORMAT_REAL   = 4;
        /**JPEG*/
        static const int FORMAT_JPEG   = 5;
        /**Microsoft VC1*/
        static const int FORMAT_VC1    = 6;
        /**AVS*/
        static const int FORMAT_AVS    = 7;
        /**YUV*/
        static const int FORMAT_YUV    = 8;
        /**H.264 MVC*/
        static const int FORMAT_H264MVC = 9;
        /**QJPEG*/
        static const int FORMAT_QJPEG  = 10;

        Video(int pid, int fmt): Element(pid)
        {
            this->mformat = fmt;
        }

        /**
         *取得视频编码格式
         *@return 返回视频编码格式
         */
        int getFormat()
        {
            return mformat;
        }
    private :
        int mformat;
    };

    /**
     *音频信息
     */
public :
    class Audio : public MultiLangElement {
    public  :
        /**MPEG*/
        static const int FORMAT_MPEG      = 0;
        /**PCM 16位小端*/
        static const int FORMAT_PCM_S16LE = 1;
        /**AAC*/
        static const int FORMAT_AAC       = 2;
        /**AC3*/
        static const int FORMAT_AC3       = 3;
        /**ALAW*/
        static const int FORMAT_ALAW      = 4;
        /**MULAW*/
        static const int FORMAT_MULAW     = 5;
        /**DTS*/
        static const int FORMAT_DTS       = 6;
        /**PCM 16位大端*/
        static const int FORMAT_PCM_S16BE = 7;
        /**FLAC*/
        static const int FORMAT_FLAC      = 8;
        /**COOK*/
        static const int FORMAT_COOK      = 9;
        /**PCM 8位*/
        static const int FORMAT_PCM_U8    = 10;
        /**ADPCM*/
        static const int FORMAT_ADPCM     = 11;
        /**AMR*/
        static const int FORMAT_AMR       = 12;
        /**RAAC*/
        static const int FORMAT_RAAC      = 13;
        /**WMA*/
        static const int FORMAT_WMA       = 14;
        /**WMA Pro*/
        static const int FORMAT_WMAPRO    = 15;
        /**蓝光PCM*/
        static const int FORMAT_PCM_BLURAY = 16;
        /**ALAC*/
        static const int FORMAT_ALAC      = 17;
        /**Vorbis*/
        static const int FORMAT_VORBIS    = 18;
        /**AAC latm格式*/
        static const int FORMAT_AAC_LATM  = 19;
        /**APE*/
        static const int FORMAT_APE       = 20;


        Audio(int pid, String8 lang, int fmt): MultiLangElement(pid, lang)
        {
            this->mformat = fmt;
        }

        /**
         *取得音频编码格式
         *@return 返回音频编码格式
         */
        int getFormat()
        {
            return mformat;
        }
    private :
        int mformat;
    };

    /**
     *字幕信息
     */
public :
    class Subtitle : public MultiLangElement {
    public :
        /**DVB subtitle*/
        static const int TYPE_DVB_SUBTITLE = 1;
        /**数字电视Teletext*/
        static const int TYPE_DTV_TELETEXT = 2;
        /**模拟电视Teletext*/
        static const int TYPE_ATV_TELETEXT = 3;
        /**数字电视Closed caption*/
        static const int TYPE_DTV_CC = 4;
        /**模拟电视Closed caption*/
        static const int TYPE_ATV_CC = 5;



        Subtitle(int pid, String8 lang, int type, int num1, int num2): MultiLangElement(pid, lang)
        {

            this->type = type;
            if (type == TYPE_DVB_SUBTITLE) {
                compositionPage = num1;
                ancillaryPage   = num2;
            } else if (type == TYPE_DTV_TELETEXT) {
                magazineNo = num1;
                pageNo = num2;
            }
        }

        /**
         *取得字幕类型
         *@return 返回字幕类型
         */
        int getType()
        {
            return type;
        }

        /**
         *取得DVB subtitle的composition page ID
         *@return 返回composition page ID
         */
        int getCompositionPageID()
        {
            return compositionPage;
        }

        /**
         *取得DVB subtitle的ancillary page ID
         *@return 返回ancillary page ID
         */
        int getAncillaryPageID()
        {
            return ancillaryPage;
        }

        /**
         *取得teletext的magazine number
         *@return 返回magazine number
         */
        int getMagazineNumber()
        {
            return magazineNo;
        }

        /**
         *取得teletext的page number
         *@return 返回page number
         */
        int getPageNumber()
        {
            return pageNo;
        }

    private :
        int compositionPage;
        int ancillaryPage;
        int magazineNo;
        int pageNo;
        int type;
    };

    /**
     *Teletext信息
     */
public :
    class Teletext : public MultiLangElement {
    public:
        Teletext(int pid, String8 lang, int mag, int page): MultiLangElement(pid, lang)
        {
            magazineNo = mag;
            pageNo = page;
        }

        /**
         *取得teletext的magazine number
         *@return 返回magazine number
         */
        int getMagazineNumber()
        {
            return magazineNo;
        }

        /**
         *取得teletext的page number
         *@return 返回page number
         */
        int getPageNumber()
        {
            return pageNo;
        }

    private :
        int magazineNo;
        int pageNo;
    };

    //节目号信息
public:
    /**如果没有发现子频道，忽略用户的输入*/
    static const int MINOR_CHECK_NONE         = 0;
    /**如果没有发现子频道，向上寻找（子频道数字增加）,找到子频道号最大的*/
    static const int MINOR_CHECK_UP           = 1;
    /**如果没有发现子频道，向下寻找（子频道数字减小）,找到子频道号最小的*/
    static const int MINOR_CHECK_DOWN         = 2;
    /*如果没有发现子频道，向上寻找，然后找到向上最近的.*/
    static const int MINOR_CHECK_NEAREST_UP   = 3;
    /*如果没有发现子频道，向下寻找，然后找到向下最近的.*/
    static const int MINOR_CHECK_NEAREST_DOWN = 4;

    /**
     *取得节目号
     *@return 返回节目号
     */
    int getNumber()
    {
        return major;
    }

    /**
     *取得主节目号(ATSC)
     *@return 返回节目的主节目号
     */
    int getMajor()
    {
        return major;
    }

    /**
     *取得次节目号(ATSC)
     *@return 返回节目的次节目号
     */
    int getMinor()
    {
        return minor;
    }

    /**
     *是否为ATSC模式
     *@return 如果是ATSC模式返回true
     */
    bool isATSCMode()
    {
        return atscMode;
    }

    /**
     *取得子频道号自动查找策略(ATSC)
     *@return 返回子频道号自动查找策略
     */
    int getMinorCheck()
    {
        return minorCheck;
    }

private:
    int major;
    int minor;
    int minorCheck;
    bool atscMode;


public:
    CTvProgram(CTvDatabase::Cursor &c);
    CTvProgram(int channelID, int type, int num, int skipFlag);
    /**
     *向数据库添加一个Program,atscMode
    */
    CTvProgram(int channelID, int type, int major, int minor, int skipFlag);
    ~CTvProgram();
    // 创建并向数据库添加一个Program
    CTvProgram(int channelID, int type);

    CTvProgram();


    int getCurrentAudio(String8 defaultLang);
    Video *getVideo()
    {
        return mpVideo;
    }
    Audio *getAudio(int id)
    {
        if (mvAudios.size() <= 0) return NULL;
        return mvAudios[id];
    }

    int getAudioTrackSize()
    {
        return mvAudios.size();
    }
    static int selectByID(int id, CTvProgram &p);
    static CTvProgram selectByNumber(int num, int type);
    int selectByNumber(int type, int major, int minor, CTvProgram &prog, int minor_check = MINOR_CHECK_NONE);
    static int selectByNumber(int type, int num, CTvProgram &prog);
    static int selectByChannel(int channelID, int type, Vector<sp<CTvProgram> > &out);
    // 列出全部TVProgram
    static int selectAll(bool no_skip, Vector<sp<CTvProgram> > &out);
    static int selectByType(int type, int skip, Vector<sp<CTvProgram> > &out);
    static int selectByChanID(int type, int skip, Vector<sp<CTvProgram> > &out);
    static Vector<CTvProgram> selectByChannel(int channelID);
    // 根据节目名称中的关键字查找指定TVProgram
    static Vector<CTvProgram>  selectByName(int name);
    void tvProgramDelByChannelID(int channelID);
    int getID()
    {
        return id;
    };
    int getSrc()
    {
        return src;
    };
    int getProgType()
    {
        return type;
    };
    int getChanOrderNum()
    {
        return chanOrderNum;
    };
    int getChanVolume()
    {
        return volume;
    };
    int getSourceId()
    {
        return sourceID;
    };
    int getServiceId()
    {
        return dvbServiceID;
    };
    int getProgSkipFlag();
    int getSubtitleIndex(int progId);
    int setSubtitleIndex(int progId, int index);
    void setCurrAudioTrackIndex(int programId, int audioIndex);
    int getCurrAudioTrackIndex();

    String8 getName();
    void getCurrentSubtitle();
    void getCurrentTeletext();
    int getChannel(CTvChannel &c);
    int upDateChannel(CTvChannel &c, int std, int freq, int fineFreq);
    int updateVolComp(int progID, int volValue);
    void updateProgramName(int progId, String8 strName);
    void setSkipFlag(int progId, bool bSkipFlag);
    void setFavoriteFlag(int progId, bool bFavor);
    int getFavoriteFlag()
    {
        return favorite;
    };
    void deleteProgram(int progId);
    static int CleanAllProgramBySrvType(int srvType);
    void setLockFlag(int progId, bool bLockFlag);
    bool getLockFlag();
    void swapChanOrder(int ProgId1, int chanOrderNum1, int ProgId2, int chanOrderNum2);
    int getAudioChannel();
    static int updateAudioChannel(int progId, int ch);
    static int deleteChannelsProgram(CTvChannel &c);
    Vector<Subtitle *> getSubtitles()
    {
        return mvSubtitles;
    }
private:
    int CreateFromCursor(CTvDatabase::Cursor &c);
    int selectProgramInChannelByNumber(int channelID, int num, CTvDatabase::Cursor &c);
    int selectProgramInChannelByNumber(int channelID, int major, int minor, CTvDatabase::Cursor &c);
    CTvChannel channel;
    int id;
    int dvbServiceID;
    int type;
    String8 name;
    int channelID;
    int skip;
    int favorite;
    int volume;
    int sourceID;
    int pmtPID;
    int src;
    int audioTrack;
    int chanOrderNum;
    int currAudTrackIndex;
    bool lock;
    bool scrambled;
    // video信息,类型不定
    Video *mpVideo;
    // audio信息,类型不定
    Vector<Audio *> mvAudios;
    // subtitle信息类型不定
    Vector<Subtitle *> mvSubtitles;
    // teletext信息,类型不定
    Vector<Teletext *> mvTeletexts;

};

#endif  //_CTVPROGRAM_H
