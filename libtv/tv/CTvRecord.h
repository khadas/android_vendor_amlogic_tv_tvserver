

class CTvRecord {
private :
    char filename[256];
    int progid;
    int vpid;
    int apid;
    static void dvr_init(void);
    void setvpid(int vid)
    {
        this->vpid = vid;
    }
    int getvpid()
    {
        return vpid;
    }
    void setapid(int aid)
    {
        this->apid = aid;
    }
    int getapid()
    {
        return apid;
    }
    static int dvr_data_write(int fd, uint8_t *buf, int size);
    static void *dvr_data_thread(void *arg);
    char *GetRecordFileName();
    void SetCurRecProgramId(int id);
    void start_data_thread(int dev_no);
    void get_cur_program_pid(int progId);
    int start_dvr(void);
    void stop_data_thread(int dev_no);


public:
    CTvRecord();
    ~CTvRecord();
    void SetRecordFileName(char *name);
    void StartRecord(int id);
    void StopRecord();
    void SetRecCurTsOrCurProgram(int sel);  // 1: all program in the Ts; 0:current program



};
