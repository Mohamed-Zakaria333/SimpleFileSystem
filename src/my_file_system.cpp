#include "my_file_system.h"

/**
 * Command : format
 * Command Format : format
 * Function Interface : void my_format()
 * Function : Format the file system, layout the virtual disk and establish root directory file
 * Input : None
 * Output : None
 */
/*
函数需完成的工作：
① 将虚拟磁盘第一个块作为引导块，开始的8个字节是文件系统的魔数，记为“10101010”；在之后写入文件系统的描述信息，如FAT表大小及位置、根目录大小及位置、盘块大小、盘块数量、数据区开始位置等信息；
② 在引导块后建立两张完全一样的FAT表，用于记录文件所占据的磁盘块及管理虚拟磁盘块的分配，每个FAT占据两个磁盘块；对于每个FAT中，前面5个块设置为已分配，后面995个块设置为空闲；
③ 在第二张FAT后创建根目录文件root，将数据区的第1块（即虚拟磁盘的第6块）分配给根目录文件，在该磁盘上创建两个特殊的目录项：“.”和“..”，其内容除了文件名不同之外，其他字段完全相同。
 */
void my_format()
{
    fat *fat1, *fat2;
    block0 *blk0;
    time_t now;
    struct tm *nowtime;
    fcb *root;
    int i;
    blk0 = (block0 *)myvhard;
    fat1 = (fat *)(myvhard + BLOCK_SIZE);
    fat2 = (fat *)(myvhard + 3 * BLOCK_SIZE);
    root = (fcb *)(myvhard + 5 * BLOCK_SIZE);
    strcpy(blk0->magic, "10101010");
    strcpy(blk0->information, "My FileSystem Ver 1.0 \n BLOCK_SIZE=1KB Whole size=1000KB Blocknum=1000 RootBlocknum=2 \n");  /*???????????????2??,??????????5,6 */
    blk0->root = 5;
    blk0->startblock = (unsigned char *) root;
    for(i = 0; i < 5; i++)
    {
        fat1->id = END;
        fat2->id = END;
        fat1++;
        fat2++;
    }
    fat1->id = 6;
    fat2->id = 6;
    fat1++;
    fat2++;
    fat1->id = END;
    fat2->id = END;
    fat1++;
    fat2++;
    for(i = 7; i < SIZE / BLOCK_SIZE; i++)
    {
        fat1->id = FREE;
        fat2->id = FREE;
        fat1++;
        fat2++;
    }
    now = time(nullptr);
    nowtime = localtime(&now);
    strcpy(root->filename, ".");
    strcpy(root->exname, "");
    root->attribute = 0x28;
    root->time = nowtime->tm_hour * 2048 + nowtime->tm_min * 32 + nowtime->tm_sec / 2;
    root->date = (nowtime->tm_year - 80) * 512 + (nowtime->tm_mon + 1) * 32 + nowtime->tm_mday;
    root->first = 5;
    root->length = 2 * sizeof(fcb);
    root->free = 1;
    root++;
    now = time(nullptr);
    nowtime = localtime(&now);
    strcpy(root->filename, "..");
    strcpy(root->exname, "");
    root->attribute = 0x28;
    root->time = nowtime->tm_hour * 2048 + nowtime->tm_min * 32 + nowtime->tm_sec / 2;
    root->date = (nowtime->tm_year - 80) * 512 + (nowtime->tm_mon + 1) * 32 + nowtime->tm_mday;
    root->first = 5;
    root->length = 2 * sizeof(fcb);
    root->free = 1;
}

/**
 * Command : mkdir
 * Command Format : mkdir dn
 * Function Interface : void my_mkdir(const char * dirname)
 * Function : Create sub directory in the current directory
 * Input : dn : directory name
 * Output : None
 */
/*
函数需完成的工作：
① 调用do_read()读入当前目录文件内容到内存，检查当前目录下新建目录文件是否重名，若重名则返回，并显示错误信息；
② 为新建子目录文件分配一个空闲打开文件表项，如果没有空闲表项则返回-1，并显示错误信息；
③ 检查FAT是否有空闲的盘块，如有则为新建目录文件分配一个盘块，否则释放①中分配的打开文件表项，返回，并显示错误信息；
④ 在当前目录中为新建目录文件寻找一个空闲的目录项或为其追加一个新的目录项;需修改当前目录文件的长度信息，并将当前目录文件的用户打开文件表项中的fcbstate置为1；
⑤ 准备好新建目录文件的FCB的内容，文件的属性为目录文件，以覆盖写方式调用do_write()将其填写到对应的空目录项中；
⑥ 在新建目录文件所分配到的磁盘块中建立两个特殊的目录项“.”和“..”目录项，方法是：首先在用户空间中准备好内容，然后以截断写或者覆盖写方式调用do_write()将其写到③中分配到的磁盘块中；
⑦ 返回。
 */
void my_mkdir(char * dirname)
{
    fcb *fcbptr;
    fat *fat1, *fat2;
    time_t now;
    struct tm *nowtime;
    char text[MAX_TEXT];
    unsigned short blkno;
    int rbn, fd, i;
    fat1 = (fat *)(myvhard + BLOCK_SIZE);
    fat2 = (fat *)(myvhard + 3 * BLOCK_SIZE);
    openfilelist[curdir].count = 0;
    rbn = do_read(curdir, openfilelist[curdir].length, text);
    fcbptr = (fcb *)text;
    for(i = 0; i < rbn / sizeof(fcb); i++)
    {
        if(strcmp(fcbptr->filename, dirname) == 0 && strcmp(fcbptr->exname, "") == 0)
        {
            printf("Error: The directory name is already exist!\n");
            return;
        }
        fcbptr++;
    }
    fcbptr = (fcb *)text;
    for(i = 0; i < rbn / sizeof(fcb); i++)    /*??????????????fcb???п??fcb???????????е????????fcb  */
    {
        if(fcbptr->free == 0)
            break;
        fcbptr++;
    }
    blkno = find_block();
    if(blkno == -1)
        return;
    (fat1 + blkno)->id = END;
    (fat2 + blkno)->id = END;
    now = time(nullptr);
    nowtime = localtime(&now);
    strcpy(fcbptr->filename, dirname);
    strcpy(fcbptr->exname, "");
    fcbptr->attribute = 0x30;
    fcbptr->time = nowtime->tm_hour * 2048 + nowtime->tm_min * 32 + nowtime->tm_sec / 2;
    fcbptr->date = (nowtime->tm_year - 80) * 512 + (nowtime->tm_mon + 1) * 32 + nowtime->tm_mday;
    fcbptr->first = blkno;              /*???????block?????fcb????? */
    fcbptr->length = 2 * sizeof(fcb);    /*??????????????????????2??fcb????????? . ?? . . ????????? */
    fcbptr->free = 1;        /*?????????????????????????*/
    openfilelist[curdir].count = i * sizeof(fcb);
    do_write(curdir, (char *)fcbptr, sizeof(fcb), 2); /*????????????????????fcb??д???????????fcb?? */

    fd = my_open(dirname);      /*????????????????my_opne()????????*/
    if(fd == -1)                  /*???????? ???????????????????????????? ???? . ?? . . ?????????*/
        return;
    fcbptr = (fcb *) malloc(sizeof(fcb));
    now = time(nullptr);
    nowtime = localtime(&now);
    strcpy(fcbptr->filename, ".");
    strcpy(fcbptr->exname, "");
    fcbptr->attribute = 0x28;
    fcbptr->time = nowtime->tm_hour * 2048 + nowtime->tm_min * 32 + nowtime->tm_sec / 2;
    fcbptr->date = (nowtime->tm_year - 80) * 512 + (nowtime->tm_mon + 1) * 32 + nowtime->tm_mday;
    fcbptr->first = blkno;    /*????????????????????????blkno?????????????????????? "."?????????????????????? ??.????????????????????????????????????????????????????? */
    fcbptr->length = 2 * sizeof(fcb);
    fcbptr->free = 1;
    do_write(fd, (char *)fcbptr, sizeof(fcb), 2);
    now = time(nullptr);
    nowtime = localtime(&now);
    strcpy(fcbptr->filename, "..");
    strcpy(fcbptr->exname, "");
    fcbptr->attribute = 0x28;
    fcbptr->time = nowtime->tm_hour * 2048 + nowtime->tm_min * 32 + nowtime->tm_sec / 2;
    fcbptr->date = (nowtime->tm_year - 80) * 512 + (nowtime->tm_mon + 1) * 32 + nowtime->tm_mday;
    fcbptr->first = blkno;
    fcbptr->length = 2 * sizeof(fcb);
    fcbptr->free = 1;
    do_write(fd, (char *)fcbptr, sizeof(fcb), 2);
    free(fcbptr);
    my_close(fd);
    fcbptr = (fcb *)text;     /*???????????д?? text???????д??????? */
    fcbptr->length = openfilelist[curdir].length;
    openfilelist[curdir].count = 0;
    do_write(curdir, (char *)fcbptr, sizeof(fcb), 2);
    openfilelist[curdir].fcbstate = 1;
}

/**
 * Command : rmdir
 * Command Format : rmdir dn
 * Function Interface : void my_rmdir(const char * name)
 * Function : delete sub directory in the current directory
 * Input : dn : directory name
 * Output : None
 */
/*
函数需完成的工作：
① 调用do_read()读入当前目录文件内容到内存，检查当前目录下欲删除目录文件是否存在，若不存在则返回，并显示错误信息；
② 检查欲删除目录文件是否为空（除了“.”和“..”外没有其他子目录和文件），可根据其目录项中记录的文件长度来判断，若不为空则返回，并显示错误信息；
③ 检查该目录文件是否已经打开，若已打开则调用my_close()关闭掉；
④ 回收该目录文件所占据的磁盘块，修改FAT；
⑤ 从当前目录文件中清空该目录文件的目录项，且free字段置为0：以覆盖写方式调用do_write()来实现；
⑥ 修改当前目录文件的用户打开表项中的长度信息，并将表项中的fcbstate置为1；
⑦ 返回。
 */
void my_rmdir(char * dirname)
{
    fcb *fcbptr, *fcbptr2;
    fat *fat1, *fat2, *fatptr1, *fatptr2;
    char text[MAX_TEXT], text2[MAX_TEXT];
    unsigned short blkno;
    int rbn, rbn2, fd, i, j;
    fat1 = (fat *)(myvhard + BLOCK_SIZE);
    fat2 = (fat *)(myvhard + 3 * BLOCK_SIZE);
    if(strcmp(dirname, ".") == 0 || strcmp(dirname, "..") == 0)    /*??????? ??.?? ?? ??..??????? */
    {
        printf("Error,can't remove this directory.\n");
        return;
    }
    openfilelist[curdir].count = 0;
    rbn = do_read(curdir, openfilelist[curdir].length, text);
    fcbptr = (fcb *)text;
    for(i = 0; i < rbn / sizeof(fcb); i++)
    {
        if(strcmp(fcbptr->filename, dirname) == 0 && strcmp(fcbptr->exname, "") == 0)
            break;
        fcbptr++;
    }
    if(i == rbn / sizeof(fcb))
    {
        printf("Error: The directory is not exist.\n");
        return;
    }
    fd = my_open(dirname);
    rbn2 = do_read(fd, openfilelist[fd].length, text2);
    fcbptr2 = (fcb *)text2;
    for(j = 0; j < rbn2 / sizeof(fcb); j++)
    {
        if(strcmp(fcbptr2->filename, ".") && strcmp(fcbptr2->filename, "..") && strcmp(fcbptr2->filename, ""))   /*??????????????????????????? */
        {
            my_close(fd);
            printf("Error,the directory is not empty.\n");
            return;
        }
        fcbptr2++;
    }
    blkno = openfilelist[fd].first;
    while(blkno != END)
    {
        fatptr1 = fat1 + blkno;
        fatptr2 = fat2 + blkno;
        blkno = fatptr1->id;
        fatptr1->id = FREE;
        fatptr2->id = FREE;
    }
    my_close(fd);
    strcpy(fcbptr->filename, "");
    fcbptr->free = 0;
    openfilelist[curdir].count = i * sizeof(fcb);
    do_write(curdir, (char *)fcbptr, sizeof(fcb), 2);/*?????????????fcb????????????fcb.free?????  */
    openfilelist[curdir].fcbstate = 1;
}

/**
 * Command : ls
 * Command Format : ls
 * Function Interface : void my_ls(void)
 * Function : list the content in the current directory
 * Input : None
 * Output : None
 */
/*
函数需完成的工作：
① 调用do_read()读出当前目录文件内容到内存；
② 将读出的目录文件的信息按照一定的格式显示到屏幕上；
③ 返回。
 */
void my_ls()
{
    fcb *fcbptr;
    char text[MAX_TEXT];
    int rbn, i;
    openfilelist[curdir].count = 0;
    rbn = do_read(curdir, openfilelist[curdir].length, text);
    fcbptr = (fcb *)text;
    for(i = 0; i < rbn / sizeof(fcb); i++)
    {
        if(fcbptr->free)
        {
            if(fcbptr->attribute & 0x20)
                printf("%s\\\t\t<DIR>\t\t%d/%d/%d\t%02d:%02d:%02d\n", fcbptr->filename, (fcbptr->date >> 9) + 1980, (fcbptr->date >> 5) & 0x000f, fcbptr->date & 0x001f, fcbptr->time >> 11, (fcbptr->time >> 5) & 0x003f, fcbptr->time & 0x001f * 2);
            else
                printf("%s.%s\t\t%dB\t\t%d/%d/%d\t%02d:%02d:%02d\t\n", fcbptr->filename, fcbptr->exname, (int)(fcbptr->length), (fcbptr->date >> 9) + 1980, (fcbptr->date >> 5) & 0x000f, fcbptr->date & 0x1f, fcbptr->time >> 11, (fcbptr->time >> 5) & 0x3f, fcbptr->time & 0x1f * 2);
        }
        fcbptr++;
    }
}

/**
 * Command : cd
 * Command Format : cd dn
 * Function Interface : void my_cd(char * dirname)
 * Function : go to the directory
 * Input : dn : directory name
 * Output : None
 */
/*
函数需完成的工作：
① 调用my_open()打开指定目录名的父目录文件，并调用do_read()读入该父目录文件内容到内存中；
② 在父目录文件中检查新的当前目录名是否存在，如果存在则转③，否则返回，并显示出错信息；
③ 调用my_close()关闭①中打开的父目录文件；
④ 调用my_close()关闭原当前目录文件；
⑤ 如果新的当前目录文件没有打开，则打开该目录文件；并将ptrCurDir指向该打开文件表项；
⑥ 设置当前目录为该目录。
 */
void my_cd(char * dirname)
{
    char *dir[10];
    int fd ,i=0, j=0;
    dir[0] = strtok(dirname, "\\");/*???????????????????dirname?????????????"\\"??????????*/
    while(1){
        i++;
        dir[i]=strtok(nullptr, "\\");/*???·?????????е????????????????   */
        if(!dir[i])
            break;
    }
    if(strcmp(dir[0], ".") == 0)
    {
        j++;
    }
    else if(strcmp(dir[0], "..") == 0)
    {
        if(curdir)               /*???????????????????±?  */
            curdir = my_close(curdir);
        j++;
        /*    return;      */
    }
    else if(strcmp(dir[0], "root") == 0)
    {
        while(curdir)
            curdir = my_close(curdir);
        j++;
    }
    for(;j<i;j++)
    {
        fd = my_open(dir[j]);
        /*printf("fd=%d\n",fd);  */
        if(fd != -1)
            curdir = fd;
        else
            return;
    }
}

/**
 * Command : create
 * Command Format : create fn
 * Function Interface : int my_create(const char * name)
 * Function : create file
 * Input : fn : file name
 * Output : index if succeed otherwise FAILURE
 */
/*
函数需完成的工作：
① 为新文件分配一个空闲打开文件表项，如果没有空闲表项则返回-1，并显示错误信息；
② 若新文件的父目录文件还没有打开，则调用my_open()打开；若打开失败，则释放①中为新建文件分配的空闲文件打开表项，返回-1，并显示错误信息；
③ 调用do_read()读出该父目录文件内容到内存，检查该目录下新文件是否重名，若重名则释放①中分配的打开文件表项，并调用my_close()关闭②中打开的目录文件；然后返回-1，并显示错误信息；
④ 检查FAT是否有空闲的盘块，如有则为新文件分配一个盘块，否则释放①中分配的打开文件表项，并调用my_close()关闭②中打开的目录文件；返回-1，并显示错误信息；
⑤ 在父目录中为新文件寻找一个空闲的目录项或为其追加一个新的目录项;需修改该目录文件的长度信息，并将该目录文件的用户打开文件表项中的fcbstate置为1；
⑥ 准备好新文件的FCB的内容，文件的属性为数据文件，长度为0，以覆盖写方式调用do_write()将其填写到⑤中分配到的空目录项中；
⑦ 为新文件填写①中分配到的空闲打开文件表项，fcbstate字段值为0，读写指针值为0；
⑧ 调用my_close()关闭②中打开的父目录文件；
⑨ 将新文件的打开文件表项序号作为其文件描述符返回。
 */
int my_create(char * filename)
{
    fcb *fcbptr;
    fat *fat1, *fat2;
    char *fname, *exname, text[MAX_TEXT];
    unsigned short blkno;
    int rbn, i;
    time_t now;
    struct tm *nowtime;
    fat1 = (fat *)(myvhard + BLOCK_SIZE);
    fat2 = (fat *)(myvhard + 3*BLOCK_SIZE);
    fname = strtok(filename, ".");
    exname = strtok(nullptr, ".");
    if(strcmp(fname, "") == 0)
    {
        printf("Error,creating file must have a right name.\n");
        return -1;
    }
    if(!exname)
    {
        printf("Error,creating file must have a extern name.\n");
        return -1;
    }
    openfilelist[curdir].count = 0;
    rbn = do_read(curdir, openfilelist[curdir].length, text);
    fcbptr = (fcb *)text;
    for(i = 0; i < rbn / sizeof(fcb); i++)
    {
        if(strcmp(fcbptr->filename, fname) == 0 && strcmp(fcbptr->exname, exname) == 0)
        {
            printf("Error: The file name is already exist!\n");
            return -1;
        }
        fcbptr++;
    }
    fcbptr = (fcb *)text;
    for(i = 0; i < rbn / sizeof(fcb); i++)
    {
        if(fcbptr->free == 0)
            break;
        fcbptr++;
    }
    blkno = find_block();
    if(blkno == -1)
        return -1;
    (fat1 + blkno)->id = END;
    (fat2 + blkno)->id = END;        /*?????????????fat->id ?????  */

    now = time(nullptr);
    nowtime = localtime(&now);
    strcpy(fcbptr->filename, fname);
    strcpy(fcbptr->exname, exname);
    fcbptr->attribute = 0x00;    /*?????????? */
    fcbptr->time = nowtime->tm_hour * 2048 + nowtime->tm_min * 32 + nowtime->tm_sec / 2;
    fcbptr->date = (nowtime->tm_year - 80) * 512 + (nowtime->tm_mon + 1) * 32 + nowtime->tm_mday;
    fcbptr->first = blkno;
    fcbptr->length = 0;
    fcbptr->free = 1;
    openfilelist[curdir].count = i * sizeof(fcb);
    do_write(curdir, (char *)fcbptr, sizeof(fcb), 2);
    fcbptr = (fcb *)text;
    fcbptr->length = openfilelist[curdir].length;
    openfilelist[curdir].count = 0;
    do_write(curdir, (char *)fcbptr, sizeof(fcb), 2);      /*????????fcb . ??? ???????????fcb?????????*/
    openfilelist[curdir].fcbstate = 1;
}

/**
 * Command : open
 * Command : open
 * Command Format : open fn
 * Function Interface : int my_open(char * name)
 * Function : open file
 * Input : fn : file name
 * Output : index if succeed otherwise FAILURE
 */
/*
函数需完成的工作：
① 检查该文件是否已经打开，若已打开则返回-1，并显示错误信息；
② 调用do_read()读出父目录文件的内容到内存，检查该目录下欲打开文件是否存在，若不存在则返回-1，并显示错误信息；
③ 检查用户打开文件表中是否有空表项，若有则为欲打开文件分配一个空表项，若没有则返回-1，并显示错误信息；
④ 为该文件填写空白用户打开文件表表项内容，读写指针置为0；
⑤ 将该文件所分配到的空白用户打开文件表表项序号（数组下标）作为文件描述符fd返回。
 */
int my_open(char * filename)
{
    fcb *fcbptr;
    char *fname, exname[3], *str, text[MAX_TEXT];
    int rbn, fd, i;
    fname = strtok(filename, ".");     /*??? ?????.????? ?????????? */
    str = strtok(nullptr, ".");      /*??????????????????????? */
    if(str)
        strcpy(exname, str);
    else
        strcpy(exname, "");    /*???????????? */
    for(i = 0; i < MAX_OPEN_FILE; i++)     /*?????????????????????????????????????????????????openfilelist[].filename????? */
    {
        if(strcmp(openfilelist[i].filename, fname) == 0 && strcmp(openfilelist[i].exname, exname) == 0 && i != curdir)
        {
            printf("Error: The file is already open.\n");
            return -1;
        }
    }
    openfilelist[curdir].count = 0;   /*?????д????0  ??curdir??????????????????±?  */
    rbn = do_read(curdir, openfilelist[curdir].length, text);    /*??????????????????? text ?????У?rbn????????????????? */
    fcbptr = (fcb *)text;
    for(i = 0; i < rbn / sizeof(fcb); i++)     /*???????????????????????????????????????????????fcb?????  */
    {
        if(strcmp(fcbptr->filename, fname) == 0 && strcmp(fcbptr->exname, exname) == 0)
            break;
        fcbptr++;
    }
    if(i == rbn / sizeof(fcb))
    {
        printf("Error: The file is not exist.\n");
        return -1;
    }
    fd = find_openfile();
    if(fd == -1)
        return -1;
    strcpy(openfilelist[fd].filename, fcbptr->filename);
    strcpy(openfilelist[fd].exname, fcbptr->exname);
    openfilelist[fd].attribute = fcbptr->attribute;
    openfilelist[fd].time = fcbptr->time;
    openfilelist[fd].date = fcbptr->date;
    openfilelist[fd].first = fcbptr->first;
    openfilelist[fd].length = fcbptr->length;
    openfilelist[fd].free = fcbptr->free;
    openfilelist[fd].dirno = openfilelist[curdir].first;
    openfilelist[fd].diroff = i;    /*???????????????е?????????fcb??  */
    strcpy(openfilelist[fd].dir, openfilelist[curdir].dir);
    strcat(openfilelist[fd].dir, filename);
    if(fcbptr->attribute & 0x20)          /*??????????????????????????????????? "\\" */
        strcat(openfilelist[fd].dir, "\\");
    openfilelist[fd].father = curdir;
    openfilelist[fd].count = 0;
    openfilelist[fd].fcbstate = 0;
    openfilelist[fd].topenfile = 1;   /*???????????????? */
    return fd;
}

/**
 * Command : close
 * Command Format : close fd
 * Function Interface : void my_close(int fd)
 * Function : close file
 * Input : fd : file index in user open table
 * Output : None
 */
/*
函数需完成的工作：
① 检查fd的有效性（fd不能超出用户打开文件表所在数组的最大下标），如果无效则返回-1；
② 检查用户打开文件表表项中的fcbstate字段的值，如果为1则需要将该文件的FCB的内容保存到虚拟磁盘上该文件的目录项中，方法是：打开该文件的父目录文件，以覆盖写方式调用do_write()将欲关闭文件的FCB写入父目录文件的相应盘块中；
③ 回收该文件占据的用户打开文件表表项（进行清空操作），并将topenfile字段置为0；
④ 返回。
 */
int my_close(int fd)
{
    fcb *fcbptr;
    int father;
    if(fd < 0 || fd >= MAX_OPEN_FILE)
    {
        printf("Error: The file is not exist.\n");
        return -1;
    }
    father=openfilelist[fd].father;
    if(openfilelist[fd].fcbstate)
    {
        fcbptr = (fcb *)malloc(sizeof(fcb));  /*???????fcb??????fcbptr*/
        strcpy(fcbptr->filename, openfilelist[fd].filename);
        strcpy(fcbptr->exname, openfilelist[fd].exname);
        fcbptr->attribute = openfilelist[fd].attribute;
        fcbptr->time = openfilelist[fd].time;
        fcbptr->date = openfilelist[fd].date;
        fcbptr->first = openfilelist[fd].first;
        fcbptr->length = openfilelist[fd].length;
        fcbptr->free = openfilelist[fd].free;
        father = openfilelist[fd].father;
        openfilelist[father].count = openfilelist[fd].diroff * sizeof(fcb);  /*????close??fd????????????????????????????????????fcbд?????????????е??????fcb??*/
        do_write(father, (char *)fcbptr, sizeof(fcb), 2);   /*??do_write???????.count??*/
        free(fcbptr);
        openfilelist[fd].fcbstate = 0;  /*??????????? */
    }
    strcpy(openfilelist[fd].filename, "");
    strcpy(openfilelist[fd].exname, "");
    openfilelist[fd].topenfile = 0;
    return father;
}

/**
 * Command : write
 * Command Format : write fd
 * Function Interface : int my_write(int fd)
 * Function : write file
 * Input : fd : file index in user open table
 * Output : Bytes written
 */
/*
函数需完成的工作：
① 检查fd的有效性（fd不能超出用户打开文件表所在数组的最大下标），如果无效则返回-1，并显示出错信息；
② 提示并等待用户输入写方式：（1：截断写；2：覆盖写；3：追加写）
③ 如果用户要求的写方式是截断写，则释放文件除第一块外的其他磁盘空间内容（查找并修改FAT表），将内存用户打开文件表项中文件长度修改为0，将读写指针置为0并转④；如果用户要求的写方式是追加写，则修改文件的当前读写指针位置到文件的末尾，并转④；如果写方式是覆盖写，则直接转④；
④ 提示用户：整个输入内容通过CTR+Z键（或其他设定的键）结束；用户可分多次输入写入内容，每次用回车结束；
⑤ 等待用户从键盘输入文件内容，并将用户的本次输入内容保存到一临时变量text[]中，要求每次输入以回车结束，全部结束用CTR+Z键（或其他设定的键）；
⑥ 调用do_write()函数将通过键盘键入的内容写到文件中。
⑦ 如果do_write()函数的返回值为非负值，则将实际写入字节数增加do_write()函数返回值，否则显示出错信息，并转⑨；
⑧ 如果text[]中最后一个字符不是结束字符CTR+Z，则转⑦继续进行写操作；否则转⑨；
⑨ 如果当前读写指针位置大于用户打开文件表项中的文件长度，则修改打开文件表项中的文件长度信息，并将fcbstate置1；
⑩ 返回实际写入的字节数。
 */
int my_write(int fd)
{
    fat *fat1, *fat2, *fatptr1, *fatptr2;
    int wstyle, len, ll, tmp;
    char text[MAX_TEXT];
    unsigned short blkno;
    fat1 = (fat *)(myvhard + BLOCK_SIZE);
    fat2 = (fat *)(myvhard + 3 * BLOCK_SIZE);
    if(fd < 0 || fd >= MAX_OPEN_FILE)
    {
        printf("Error: The file is not exist!\n");
        return -1;
    }
    while(1)
    {
        printf("Please enter the number of write style:\n1.cut write\t2.cover write\t3.add write\n");
        scanf("%d", &wstyle);
        if(wstyle > 0 && wstyle < 4)
            break;
        printf("Input Error!");
    }
    getchar();
    switch(wstyle)
    {
        case 1:
            blkno = openfilelist[fd].first;
            fatptr1 = fat1 + blkno;
            fatptr2 = fat2 + blkno;
            blkno = fatptr1->id;
            fatptr1->id = END;
            fatptr2->id = END;
            while(blkno != END)
            {
                fatptr1 = fat1 + blkno;
                fatptr2 = fat2 + blkno;
                blkno = fatptr1->id;
                fatptr1->id = FREE;
                fatptr2->id = FREE;
            }
            openfilelist[fd].count = 0;
            openfilelist[fd].length = 0;
            break;
        case 2:
            openfilelist[fd].count = 0;
            break;
        case 3:
            openfilelist[fd].count = openfilelist[fd].length;
            break;
        default:
            break;
    }
    ll = 0;
    printf("Please input write data(end with Ctrl+Z):\n");
    while(gets(text))     /*?????豸??????????????????,????浽text???????????*/
    {
        len = strlen(text);
        text[len++] = '\n';
        text[len] = '\0';
        tmp = do_write(fd, text, len, wstyle);
        if(tmp != -1)
            ll += tmp;
        if(tmp < len)
        {
            printf("Write Error!");
            break;
        }
    }
    return ll;
}

/**
 * Command : None
 * Command Format : None
 * Function Interface : int do_write(int fd, char *text, int len, char wstyle)
 * Function : Called by my_write and write content in the file
 * Input : fd : file index in user open table, text : content pointer, len : content length, wstyle : write mode
 * Output : Bytes written
 */
/*
函数需完成的工作：
① 用malloc()申请1024B的内存空间作为读写磁盘的缓冲区buf，申请失败则返回-1，并显示出错信息；
② 将读写指针转化为逻辑块块号和块内偏移off，并利用打开文件表表项中的首块号及FAT表的相关内容将逻辑块块号转换成对应的磁盘块块号blkno；如果找不到对应的磁盘块，则需要检索FAT为该逻辑块分配一新的磁盘块，并将对应的磁盘块块号blkno登记到FAT中，若分配失败，则返回-1，并显示出错信息；
③ 如果是覆盖写，或者如果当前读写指针所对应的块内偏移off不等于0，则将块号为blkno的虚拟磁盘块全部1024B的内容读到缓冲区buf中；否则便用ASCII码0清空buf；
④ 将text中未写入的内容暂存到缓冲区buff的第off字节开始的位置，直到缓冲区满，或者接收到结束字符CTR+Z为止；将本次写入字节数记录到tmplen中；
⑤ 将buf中1024B的内容写入到块号为blkno的虚拟磁盘块中；
⑥将当前读写指针修改为原来的值加上tmplen；并将本次实际写入的字节数增加tmplen；
⑦ 如果tmplen小于len，则转②继续写入；否则转⑧；
⑧ 返回本次实际写入的字节数。
 */
int do_write(int fd, const char *text, int len, char wstyle)
{
    fat *fat1, *fat2, *fatptr1, *fatptr2;
    unsigned char *buf, *blkptr;
    unsigned short blkno, blkoff;
    int i, ll;
    fat1 = (fat *) (myvhard + BLOCK_SIZE);
    fat2 = (fat *) (myvhard + 3 * BLOCK_SIZE);
    buf = (unsigned char *) malloc(BLOCK_SIZE);
    if(buf == nullptr)
    {
        printf("malloc failed!\n");
        return -1;
    }
    blkno = openfilelist[fd].first;
    blkoff = openfilelist[fd].count;
    fatptr1 = fat1 + blkno;
    fatptr2 = fat2 + blkno;
    while(blkoff >= BLOCK_SIZE)
    {
        blkno = fatptr1->id;
        if(blkno == END)
        {
            blkno = find_block();
            if(blkno == -1)
            {
                free(buf);
                return -1;
            }
            fatptr1->id = blkno;
            fatptr2->id = blkno;
            fatptr1 = fat1 + blkno;
            fatptr2 = fat2 + blkno;
            fatptr1->id = END;
            fatptr2->id = END;
        }
        else
        {
            fatptr1 = fat1 + blkno;
            fatptr2 = fat2 + blkno;
        }
        blkoff -= BLOCK_SIZE;
    }
    ll = 0;
    while(ll < len)
    {
        blkptr = myvhard + blkno * BLOCK_SIZE;
        for(i = 0; i < BLOCK_SIZE; i++)
            buf[i] = blkptr[i];
        for(;blkoff < BLOCK_SIZE; blkoff++)
        {
            buf[blkoff] = text[ll++];
            openfilelist[fd].count++;
            if(ll == len)
                break;
        }
        for(i = 0; i < BLOCK_SIZE; i++)
            blkptr[i] = buf[i];
        if(ll < len)
        {
            blkno = fatptr1->id;
            if(blkno == END)
            {
                blkno = find_block();
                if(blkno == -1)
                    break;
                fatptr1->id = blkno;
                fatptr2->id = blkno;
                fatptr1 = fat1 + blkno;
                fatptr2 = fat2 + blkno;
                fatptr1->id = END;
                fatptr2->id = END;
            }
            else
            {
                fatptr1 = fat1 + blkno;
                fatptr2 = fat2 + blkno;
            }
            blkoff = 0;
        }
    }
    if(openfilelist[fd].count > openfilelist[fd].length)
        openfilelist[fd].length = openfilelist[fd].count;
    openfilelist[fd].fcbstate = 1;
    free(buf);
    return ll;
}

/**
 * Command : read
 * Command Format : read fd len
 * Function Interface : int myread (int fd, int len)
 * Function : read file
 * Input : fd : file index in user open table, len : read length
 * Output : Bytes read
 */
/*
函数需完成的工作：
① 定义一个字符型数组text[len]，用来接收用户从文件中读出的文件内容；
② 检查fd的有效性（fd不能超出用户打开文件表所在数组的最大下标），如果无效则返回-1，并显示出错信息；
③ 调用do_read()将指定文件中的len字节内容读出到text[]中；
④ 如果do_read()的返回值为负，则显示出错信息；否则将text[]中的内容显示到屏幕上；
⑤ 返回。
 */
int my_read (int fd, unsigned int len)
{
    char text[MAX_TEXT];
    int ll;
    if(fd < 0 || fd >= MAX_OPEN_FILE)
    {
        printf("Error: The File is not exist!\n");
        return -1;
    }
    openfilelist[fd].count = 0;
    ll = do_read(fd, len, text);
    if(ll != -1)
        printf("%s", text);
    else
        printf("Read Error!\n");
    return ll;
}

/**
 * Command : None
 * Command Format : None
 * Function Interface : int do_read (int fd, int len, char *text)
 * Function : Called by my_read and read content in the file
 * Input : fd : file index in user open table, len : content length, text : file to write content
 * Output : Bytes read
 */
/*
函数需完成的工作：
① 使用malloc()申请1024B空间作为缓冲区buf，申请失败则返回-1，并显示出错信息；
② 将读写指针转化为逻辑块块号及块内偏移量off，利用打开文件表表项中的首块号查找FAT表，找到该逻辑块所在的磁盘块块号；将该磁盘块块号转化为虚拟磁盘上的内存位置；
③ 将该内存位置开始的1024B（一个磁盘块）内容读入buf中；
④ 比较buf中从偏移量off开始的剩余字节数是否大于等于应读写的字节数len，如果是，则将从off开始的buf中的len长度的内容读入到text[]中；否则，将从off开始的buf中的剩余内容读入到text[]中；
⑤ 将读写指针增加④中已读字节数，将应读写的字节数len减去④中已读字节数，若len大于0，则转②；否则转⑥；
⑥ 使用free()释放①中申请的buf。
⑦ 返回实际读出的字节数。
 */
int do_read (int fd, unsigned int len, char *text)
{
    unsigned char *buf, *blkptr;
    fat *fat1, *fatptr;
    unsigned short blkno, blkoff;
    int i, ll;
    buf = (unsigned char *) malloc(BLOCK_SIZE);
    if(buf == nullptr)
    {
        printf("Error: Malloc failed!\n");
        return -1;
    }
    fat1 = (fat *) (myvhard + BLOCK_SIZE);
    blkno = openfilelist[fd].first;
    blkoff = openfilelist[fd].count;
    if(blkoff >= openfilelist[fd].length)
    {
        puts("Error: Read out of range!");
        free(buf);
        return -1;
    }
    fatptr = fat1 + blkno;
    while(blkoff >= BLOCK_SIZE)
    {
        blkno = fatptr->id;
        blkoff -= BLOCK_SIZE;
        fatptr = fat1 + blkno;
    }
    ll = 0;
    while(ll < len)
    {
        blkptr = myvhard + blkno * BLOCK_SIZE;
        for(i = 0; i < BLOCK_SIZE; i++)
            buf[i] = blkptr[i];
        for(; blkoff < BLOCK_SIZE; blkoff++)
        {
            text[ll++] = buf[blkoff];
            openfilelist[fd].count++;
            if(ll == len || openfilelist[fd].count == openfilelist[fd].length)
                break;
        }
        if(ll < len && openfilelist[fd].count != openfilelist[fd].length)
        {
            blkno = fatptr->id;
            if(blkno == END)
                break;
            blkoff = 0;
            fatptr = fat1 + blkno;
        }
    }
    text[ll] = '\0';
    free(buf);
    return ll;
}

/**
 * Command : rm
 * Command Format : rm fn
 * Function Interface : void my_rm(const char * name)
 * Function : delete file
 * Input : fn : file name
 * Output : None
 */
/*
函数需完成的工作：
① 若欲删除文件的父目录文件还没有打开，则调用my_open()打开；若打开失败，则返回，并显示错误信息；
② 调用do_read()读出该父目录文件内容到内存，检查该目录下欲删除文件是否存在，若不存在则返回，并显示错误信息；
③ 检查该文件是否已经打开，若已打开则关闭掉；
④ 回收该文件所占据的磁盘块，修改FAT；
⑤ 从文件的父目录文件中清空该文件的目录项，且free字段置为0：以覆盖写方式调用do_write()来实现；；
⑥ 修改该父目录文件的用户打开文件表项中的长度信息，并将该表项中的fcbstate置为1；
⑦ 返回。
 */
void my_rm(char * filename)
{
    fcb *fcbptr;
    fat *fat1, *fat2, *fatptr1, *fatptr2;
    char *fname, *exname, text[MAX_TEXT];
    unsigned short blkno;
    int rbn, i;
    fat1 = (fat *) (myvhard + BLOCK_SIZE);
    fat2 = (fat *) (myvhard + 3 * BLOCK_SIZE);
    fname = strtok(filename, ".");
    exname = strtok(nullptr, ".");
    if(strcmp(fname, "") == 0)
    {
        printf("Error: Removing file must have a right name.\n");
        return;
    }
    if(!exname)
    {
        printf("Error: Removing file must have a extern name.\n");
        return;
    }
    openfilelist[curdir].count = 0;
    rbn = do_read(curdir, openfilelist[curdir].length, text);
    fcbptr = (fcb *)text;
    for(i = 0; i < rbn / sizeof(fcb); i++)
    {
        if(strcmp(fcbptr->filename, fname) == 0 && strcmp(fcbptr->exname, exname) == 0)
            break;
        fcbptr++;
    }
    if(i == rbn / sizeof(fcb))
    {
        printf("Error, the file is not exist.\n");
        return;
    }
    blkno = fcbptr->first;
    while(blkno != END)
    {
        fatptr1 = fat1 + blkno;
        fatptr2 = fat2 + blkno;
        blkno = fatptr1->id;
        fatptr1->id = FREE;
        fatptr2->id = FREE;
    }
    strcpy(fcbptr->filename, "");
    fcbptr->free = 0;
    openfilelist[curdir].count = i * sizeof(fcb);
    do_write(curdir, (char *)fcbptr, sizeof(fcb), 2);
    openfilelist[curdir].fcbstate = 1;
}


/**
 * Command : None
 * Command Format : None
 * Function Interface : void my_startsys()
 * Function : Called by main and enter into file system
 * Input : None
 * Output : None
 */
/*
函数需完成的工作：
① 申请虚拟磁盘空间；
② 使用C语言的库函数fopen()打开myfsys文件：若文件存在，则转③；若文件不存在，则创建之，转⑤
③ 使用C语言的库函数fread()读入myfsys文件内容到用户空间中的一个缓冲区中，并判断其开始的8个字节内容是否为“10101010”（文件系统魔数），如果是，则转④；否则转⑤；
④ 将上述缓冲区中的内容复制到内存中的虚拟磁盘空间中；转⑦
⑤ 在屏幕上显示“myfsys文件系统不存在，现在开始创建文件系统”信息，并调用my_format()对①中申请到的虚拟磁盘空间进行格式化操作。转⑥；
⑥ 将虚拟磁盘中的内容保存到myfsys文件中；转⑦
⑦ 使用C语言的库函数fclose()关闭myfsys文件；
⑧ 初始化用户打开文件表，将表项0分配给根目录文件使用，并填写根目录文件的相关信息，由于根目录没有上级目录，所以表项中的dirno和diroff分别置为5（根目录所在起始块号）和0；并将ptrptrCurDir指针指向该用户打开文件表项。
⑨ 将当前目录设置为根目录。
 */
void my_startsys()
{
    FILE *fp;
    unsigned char buf[SIZE];
    fcb *root;
    int i;
    myvhard = (unsigned char *)malloc(SIZE);
    memset(myvhard, 0, SIZE);
    if((fp = fopen(myfilename, "r")) != nullptr)
    {
        fread(buf, SIZE, 1, fp);
        if(strcmp(((block0 *) buf)->magic, "10101010") != 0)
        {
            for(i = 0; i < SIZE; i++)
                myvhard[i] = buf[i];
        }
        else
        {
            fclose(fp);
            printf("myfilesys is not exist, begin to create the file...\n");
            my_format();
            fp = fopen(myfilename, "w");
            fwrite(myvhard, SIZE, 1, fp);
        }
    }
    else
    {
        printf("myfilesys is not exist, begin to create the file...\n");
        my_format();
        fp = fopen(myfilename, "w");
        fwrite(myvhard, SIZE, 1, fp);
    }
    fclose(fp);
    root = (fcb *)(myvhard + 5 * BLOCK_SIZE);
    strcpy(openfilelist[0].filename, root->filename);
    strcpy(openfilelist[0].exname, root->exname);
    openfilelist[0].attribute = root->attribute;
    openfilelist[0].time = root->time;
    openfilelist[0].date = root->date;
    openfilelist[0].first = root->first;
    openfilelist[0].length = root->length;
    openfilelist[0].free = root->free;
    openfilelist[0].dirno = 5;
    openfilelist[0].diroff = 0;
    strcpy(openfilelist[0].dir, "\\root\\");
    openfilelist[0].father = 0;
    openfilelist[0].count = 0;
    openfilelist[0].fcbstate = 0;
    openfilelist[0].topenfile = 1;
    for(i = 1; i < MAX_OPEN_FILE; i++)
        openfilelist[i].topenfile = 0;
    curdir = 0;
    strcpy(currentdir, "\\root\\");
    startp = ((block0 *) myvhard)->startblock;
}

/**
 * Command : exitsys
 * Command Format : exitsys
 * Function Interface : void my_exitsys()
 * Function : exit file system
 * Input : None
 * Output : None
 */
/*
函数需完成的工作：
① 使用C库函数fopen()打开磁盘上的myfsys文件；
② 将虚拟磁盘空间中的所有内容保存到磁盘上的myfsys文件中；
③ 使用c语言的库函数fclose()关闭myfsys文件；
④ 撤销用户打开文件表，释放其内存空间
⑤ 释放虚拟磁盘空间。
 */
void my_exitsys()
{
    FILE *fp;
    while(curdir)
        curdir = my_close(curdir);
    fp = fopen(myfilename, "w");
    fwrite(myvhard, SIZE, 1, fp);
    fclose(fp);
    free(myvhard);
}

int find_block()
{
    unsigned short i;
    fat *fat1, *fatptr;
    fat1 = (fat *)(myvhard + BLOCK_SIZE);
    for(i = 7; i < SIZE / BLOCK_SIZE; i++)
    {
        fatptr = fat1 + i;
        if(fatptr->id == FREE)
            return i;
    }
    printf("Error: Can't find free block!\n");
    return -1;
}

int find_openfile()
{
    int i;
    for(i = 0; i < MAX_OPEN_FILE; i++)
    {
        if(openfilelist[i].topenfile == 0)
            return i;
    }
    printf("Error: Open too many files!\n");
    return -1;
}


int main() {
    my_startsys();
    char cmd[15][10] = {"cd", "mkdir", "rmdir", "ls", "create", "rm", "open", "close", "write", "read", "exit"};
    char s[30], *sp;
    int cmdn, flag = 1, i;
    my_startsys();
    printf("*********************File System V1.0*******************************\n\n");
    printf("??????\t\t???????\t\t???????\n\n");
    printf("cd\t\t????(·????)\t\t?л?????????????\n");
    printf("mkdir\t\t????\t\t\t??????????????\n");
    printf("rmdir\t\t????\t\t\t??????????????\n");
    printf("ls\t\t??\t\t\t??????????????????\n");
    printf("create\t\t?????\t\t\t?????????????????\n");
    printf("rm\t\t?????\t\t\t?????????????????\n");
    printf("open\t\t?????\t\t\t???????????????\n");
    printf("write\t\t??\t\t\t???????????д?????\n");
    printf("read\t\t??\t\t\t???????????????????\n");
    printf("close\t\t??\t\t\t??????????????????\n");
    printf("exit\t\t??\t\t\t?????\n\n");
    printf("*********************************************************************\n\n");
    while(flag)
    {
        printf("%s>", openfilelist[curdir].dir);
        gets(s);
        cmdn = -1;
        if(strcmp(s, ""))
        {
            sp=strtok(s, " ");
            for(i = 0; i < 15; i++)
            {
                if(strcmp(sp, cmd[i]) == 0)
                {
                    cmdn = i;
                    break;
                }
            }
            switch(cmdn)
            {
                case 0:
                    sp = strtok(nullptr, " ");
                    if(sp && (openfilelist[curdir].attribute & 0x20))
                        my_cd(sp);
                    else
                        printf("Please input the right command.\n");
                    break;
                case 1:
                    sp = strtok(nullptr, " ");
                    if(sp && (openfilelist[curdir].attribute & 0x20))
                        my_mkdir(sp);
                    else
                        printf("Please input the right command.\n");
                    break;
                case 2:
                    sp = strtok(nullptr, " ");
                    if(sp && (openfilelist[curdir].attribute & 0x20))
                        my_rmdir(sp);
                    else
                        printf("Please input the right command.\n");
                    break;
                case 3:
                    if(openfilelist[curdir].attribute & 0x20)
                        my_ls();
                    else
                        printf("Please input the right command.\n");
                    break;
                case 4:
                    sp = strtok(nullptr, " ");
                    if(sp && (openfilelist[curdir].attribute & 0x20))
                        my_create(sp);
                    else
                        printf("Please input the right command.\n");
                    break;
                case 5:
                    sp = strtok(nullptr, " ");
                    if(sp && (openfilelist[curdir].attribute & 0x20))
                        my_rm(sp);
                    else
                        printf("Please input the right command.\n");
                    break;
                case 6:
                    sp = strtok(nullptr, " ");
                    if(sp && (openfilelist[curdir].attribute & 0x20))
                    {
                        if(strchr(sp, '.'))/*????sp??'.'??γ????λ??*/
                            curdir = my_open(sp);
                        else
                            printf("the openfile should have exname.\n");
                    }
                    else
                        printf("Please input the right command.\n");
                    break;
                case 7:
                    if(!(openfilelist[curdir].attribute & 0x20))
                        curdir = my_close(curdir);
                    else
                        printf("No files opened.\n");
                    break;
                case 8:
                    if(!(openfilelist[curdir].attribute & 0x20))
                        my_write(curdir);
                    else
                        printf("No files opened.\n");
                    break;
                case 9:
                    if(!(openfilelist[curdir].attribute & 0x20))
                        my_read(curdir, openfilelist[curdir].length);
                    else
                        printf("No files opened.\n");
                    break;
                case 10:
                    if(openfilelist[curdir].attribute & 0x20)
                    {
                        my_exitsys();
                        flag = 0;
                    }
                    else
                        printf("Please input the right command.\n");
                    break;
                default:
                    printf("Please input the right command.\n");
                    break;
            }
        }
    }
    return 0;
}
