#ifndef INI_FILE_H
#define INI_FILE_H

// The properties that are specified in the ini file.
extern int pinButtonUp;
extern int pinButtonDown;
extern int pinButtonMode;
extern int pinDHT22;
extern int pinRelay;
extern int i2cAddress;
extern char* mysqlHost;
extern char* mysqlUser;
extern char* mysqlPasswd;
extern char* mysqlDatabase;
extern int mysqlPort;
extern char* mysqlSocket;

void readIniFile();

#endif /* INI_FILE_H */
