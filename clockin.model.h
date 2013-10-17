#include <mysql.h>

//依赖   yum install -y mysql-devel
//编译的时候  要加上   -lmysqlclient  -I/usr/local/mysql/include  或者配置gcc环境变量   export C_INCLUDE_PATH=$C_INCLUDE_PATH:/usr/local/mysql/include

extern MYSQL mysql;
//打卡model
int insert_clockin(char *phoneNumber, char *phoneMac, char *datetime);
