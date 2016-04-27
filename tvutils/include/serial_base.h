#ifndef __SERIAL_BASE_H__
#define __SERIAL_BASE_H__

extern int com_a_open_dev();
extern int com_b_open_dev();
extern int com_a_close_dev();
extern int com_b_close_dev();
extern int com_a_get_dev();
extern int com_b_get_dev();
extern int com_a_set_opt(int speed, int db, int sb, int pb, int timeout, int raw_mode);
extern int com_b_set_opt(int speed, int db, int sb, int pb, int timeout, int raw_mode);
extern int com_a_write_data(const unsigned char *pData, unsigned int uLen);
extern int com_b_write_data(const unsigned char *pData, unsigned int uLen);
extern int com_a_read_data(char *pData, unsigned int uLen);
extern int com_b_read_data(char *pData, unsigned int uLen);

#endif//__SERIAL_BASE_H__
