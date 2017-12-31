#ifndef SIMPLE_FILE_SYSTEM_MY_FILE_SYSTEM_H
#define SIMPLE_FILE_SYSTEM_MY_FILE_SYSTEM_H

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

using namespace std;

#define BLOCKSIZE 1024                          //磁盘块大小
#define SIZE 1024000                            //虚拟磁盘空间大小
#define END 65535                               //FAT中的文件结束标志
#define FREE 0                                  //FAT中盘块空闲标志
#define MAX_OPEN_FILE 10                          //最多同时打开文件个数
#define MAX_TEXT 10000

/**
 * FAT File System Structure
 * FAT 文件系统结构
 *
 * |Start| End |	Name	|	Note			|
 * |-----|-----|------------|-------------------|
 * |  0  |  0  | Boot Block | 引导区			|
 * |-----|-----|------------|-------------------|
 * |  1  |  2  | 	FAT1	| 文件分配表 		|
 * |-----|-----|------------|-------------------|
 * |  3  |  4  | 	FAT2	| 文件分配表-备份 	|
 * |-----|-----|------------|-------------------|
 * |  5  |1000 |  DATA BLOCK| 数据区			|
 * |-----|-----|------------|-------------------|
 * |  5  |  5  | ROOT_DIR	| 根目录文件		|
 *
 */

typedef struct FCB
{

    char filename[8]; //文件名
    char exname[3];//文件扩展名
    unsigned char attribute;//文件属性字段：为简单起见，我们只为文件设置
    //了两种属性：值为0时表示目录文件，值为1时表示数据文件
    unsigned short time;//文件创建时间
    unsigned short date;//文件创建日期
    unsigned short first;//文件起始盘块号
    unsigned long length;//文件长度（字节数）
    char free;//表示目录项是否为空，若值为0，表示空，值为1，表示已分配
} fcb;

/**
* FAT : File Allocation Table
*/
typedef struct FAT
{
    unsigned short id;
} fat;

/**
 * USER_OPEN : File Table Opened By User
 */
typedef struct USER_OPEN
{
    char filename[8]; //文件名
    char exname[3];//文件扩展名
    unsigned char attribute;//文件属性：值为0时表示目录文件，值为1时表示数据文件
    unsigned short time;//文件创建时间
    unsigned short date;//文件创建日期
    unsigned short first;//文件起始盘块号
    unsigned long length;//文件长度（对数据文件是字节数，对目录文件可以是目录项个数）
    char free;//表示目录项是否为空，若值为0，表示空，值为1，表示已分配
//前面内容是文件的FCB中的内容。
// 下面设置的dirno和diroff记录了相应打开文件的目录项在父目录文件中的位置，//这样如果该文件的fcb被修改了，则要写回父目录文件时比较方便
    int dirno; //相应打开文件的目录项在父目录文件中的盘块号
    int diroff;// 相应打开文件的目录项在父目录文件的dirno盘块中的目录项序号
    char dir[80]; //相应打开文件所在的目录名，这样方便快速检查出指定文件是否已经打开
    int father;  /* 父目录在打开文件表项的位置 */
    unsigned short count; //读写指针在文件中的位置
    char fcbstate; //是否修改了文件的FCB的内容，如果修改了置为1，否则为0
    char topenfile; //表示该用户打开表项是否为空，若值为0，表示为空，否则表示已被某打开文件占据
} useropen;

/**
* Boot Block | 引导块
*
* @note
* 	引导块占用第0号物理块，不属于文件系统管辖，
* 	如果系统中有多个文件系统，只有根文件系统才
* 	有引导程序放在引导块中，其余文件系统都不
* 	使用引导块
*
*/
typedef struct BLOCK0 //引导块内容
{
    char magic[8];  /* 文件系统魔数 */ //?
    char information[200];
    unsigned short root; //根目录文件的起始盘块号
    unsigned char *startblock; //虚拟磁盘上数据区开始位置

} block0;

unsigned char *myvhard;  /* 指向虚拟磁盘的起始地址   */
USER_OPEN openfilelist[MAX_OPEN_FILE];  /* 用户打开文件表数组  */
int curdir;  /* 用户打开文件表中的当前目录所在打开文件表项的位置 */
char currentdir[80];  /* 记录当前目录的目录名（包括目录的路径） */
unsigned char* startp;  /* 记录虚拟磁盘上数据区开始位置  */
char myfilename[] = "myfilesys";

void my_format();                                          // 格式化文件系统
void my_mkdir(char * dirName);                       // 创建子目录
void my_rmdir(char * dirName);                       // 删除子目录
void my_ls();                                              // 显示目录中的内容
void my_cd(char * dirName);                          // 更改当前目录
int my_create(char * fileName);                      // 创建文件
int my_open(char * fileName);                        // 打开文件
int my_close(int fd);                      // 关闭文件
int my_write(int fd);                       // 写文件
int do_write(int fd, char *text, int len, char wstyle);    // 实际写文件
int my_read (int fd, unsigned int len);                       // 读文件
int do_read (int fd, unsigned int len, char *text);                 // 实际读文件
void my_rm(char * fileName);                         // 删除文件
void my_startsys();                                        // 进入文件系统
void my_exitsys();                                         // 退出文件系统
int findblock();
int findopenfile();

#endif
