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
��������ɵĹ�����
�� ��������̵�һ������Ϊ�����飬��ʼ��8���ֽ����ļ�ϵͳ��ħ������Ϊ��10101010������֮��д���ļ�ϵͳ��������Ϣ����FAT���С��λ�á���Ŀ¼��С��λ�á��̿��С���̿���������������ʼλ�õ���Ϣ��
�� �����������������ȫһ����FAT�����ڼ�¼�ļ���ռ�ݵĴ��̿鼰����������̿�ķ��䣬ÿ��FATռ���������̿飻����ÿ��FAT�У�ǰ��5��������Ϊ�ѷ��䣬����995��������Ϊ���У�
�� �ڵڶ���FAT�󴴽���Ŀ¼�ļ�root�����������ĵ�1�飨��������̵ĵ�6�飩�������Ŀ¼�ļ����ڸô����ϴ������������Ŀ¼���.���͡�..���������ݳ����ļ�����֮ͬ�⣬�����ֶ���ȫ��ͬ��
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
��������ɵĹ�����
�� ����do_read()���뵱ǰĿ¼�ļ����ݵ��ڴ棬��鵱ǰĿ¼���½�Ŀ¼�ļ��Ƿ��������������򷵻أ�����ʾ������Ϣ��
�� Ϊ�½���Ŀ¼�ļ�����һ�����д��ļ�������û�п��б����򷵻�-1������ʾ������Ϣ��
�� ���FAT�Ƿ��п��е��̿飬������Ϊ�½�Ŀ¼�ļ�����һ���̿飬�����ͷŢ��з���Ĵ��ļ�������أ�����ʾ������Ϣ��
�� �ڵ�ǰĿ¼��Ϊ�½�Ŀ¼�ļ�Ѱ��һ�����е�Ŀ¼���Ϊ��׷��һ���µ�Ŀ¼��;���޸ĵ�ǰĿ¼�ļ��ĳ�����Ϣ��������ǰĿ¼�ļ����û����ļ������е�fcbstate��Ϊ1��
�� ׼�����½�Ŀ¼�ļ���FCB�����ݣ��ļ�������ΪĿ¼�ļ����Ը���д��ʽ����do_write()������д����Ӧ�Ŀ�Ŀ¼���У�
�� ���½�Ŀ¼�ļ������䵽�Ĵ��̿��н������������Ŀ¼�.���͡�..��Ŀ¼������ǣ��������û��ռ���׼�������ݣ�Ȼ���Խض�д���߸���д��ʽ����do_write()����д�����з��䵽�Ĵ��̿��У�
�� ���ء�
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
    for(i = 0; i < rbn / sizeof(fcb); i++)    /*??????????????fcb???��??fcb???????????��????????fcb  */
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
    do_write(curdir, (char *)fcbptr, sizeof(fcb), 2); /*????????????????????fcb??��???????????fcb?? */

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
    fcbptr = (fcb *)text;     /*???????????��?? text???????��??????? */
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
��������ɵĹ�����
�� ����do_read()���뵱ǰĿ¼�ļ����ݵ��ڴ棬��鵱ǰĿ¼����ɾ��Ŀ¼�ļ��Ƿ���ڣ����������򷵻أ�����ʾ������Ϣ��
�� �����ɾ��Ŀ¼�ļ��Ƿ�Ϊ�գ����ˡ�.���͡�..����û��������Ŀ¼���ļ������ɸ�����Ŀ¼���м�¼���ļ��������жϣ�����Ϊ���򷵻أ�����ʾ������Ϣ��
�� ����Ŀ¼�ļ��Ƿ��Ѿ��򿪣����Ѵ������my_close()�رյ���
�� ���ո�Ŀ¼�ļ���ռ�ݵĴ��̿飬�޸�FAT��
�� �ӵ�ǰĿ¼�ļ�����ո�Ŀ¼�ļ���Ŀ¼���free�ֶ���Ϊ0���Ը���д��ʽ����do_write()��ʵ�֣�
�� �޸ĵ�ǰĿ¼�ļ����û��򿪱����еĳ�����Ϣ�����������е�fcbstate��Ϊ1��
�� ���ء�
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
��������ɵĹ�����
�� ����do_read()������ǰĿ¼�ļ����ݵ��ڴ棻
�� ��������Ŀ¼�ļ�����Ϣ����һ���ĸ�ʽ��ʾ����Ļ�ϣ�
�� ���ء�
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
��������ɵĹ�����
�� ����my_open()��ָ��Ŀ¼���ĸ�Ŀ¼�ļ���������do_read()����ø�Ŀ¼�ļ����ݵ��ڴ��У�
�� �ڸ�Ŀ¼�ļ��м���µĵ�ǰĿ¼���Ƿ���ڣ����������ת�ۣ����򷵻أ�����ʾ������Ϣ��
�� ����my_close()�رբ��д򿪵ĸ�Ŀ¼�ļ���
�� ����my_close()�ر�ԭ��ǰĿ¼�ļ���
�� ����µĵ�ǰĿ¼�ļ�û�д򿪣���򿪸�Ŀ¼�ļ�������ptrCurDirָ��ô��ļ����
�� ���õ�ǰĿ¼Ϊ��Ŀ¼��
 */
void my_cd(char * dirname)
{
    char *dir[10];
    int fd ,i=0, j=0;
    dir[0] = strtok(dirname, "\\");/*???????????????????dirname?????????????"\\"??????????*/
    while(1){
        i++;
        dir[i]=strtok(nullptr, "\\");/*???��?????????��????????????????   */
        if(!dir[i])
            break;
    }
    if(strcmp(dir[0], ".") == 0)
    {
        j++;
    }
    else if(strcmp(dir[0], "..") == 0)
    {
        if(curdir)               /*???????????????????��?  */
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
��������ɵĹ�����
�� Ϊ���ļ�����һ�����д��ļ�������û�п��б����򷵻�-1������ʾ������Ϣ��
�� �����ļ��ĸ�Ŀ¼�ļ���û�д򿪣������my_open()�򿪣�����ʧ�ܣ����ͷŢ���Ϊ�½��ļ�����Ŀ����ļ��򿪱������-1������ʾ������Ϣ��
�� ����do_read()�����ø�Ŀ¼�ļ����ݵ��ڴ棬����Ŀ¼�����ļ��Ƿ����������������ͷŢ��з���Ĵ��ļ����������my_close()�رբ��д򿪵�Ŀ¼�ļ���Ȼ�󷵻�-1������ʾ������Ϣ��
�� ���FAT�Ƿ��п��е��̿飬������Ϊ���ļ�����һ���̿飬�����ͷŢ��з���Ĵ��ļ����������my_close()�رբ��д򿪵�Ŀ¼�ļ�������-1������ʾ������Ϣ��
�� �ڸ�Ŀ¼��Ϊ���ļ�Ѱ��һ�����е�Ŀ¼���Ϊ��׷��һ���µ�Ŀ¼��;���޸ĸ�Ŀ¼�ļ��ĳ�����Ϣ��������Ŀ¼�ļ����û����ļ������е�fcbstate��Ϊ1��
�� ׼�������ļ���FCB�����ݣ��ļ�������Ϊ�����ļ�������Ϊ0���Ը���д��ʽ����do_write()������д�����з��䵽�Ŀ�Ŀ¼���У�
�� Ϊ���ļ���д���з��䵽�Ŀ��д��ļ����fcbstate�ֶ�ֵΪ0����дָ��ֵΪ0��
�� ����my_close()�رբ��д򿪵ĸ�Ŀ¼�ļ���
�� �����ļ��Ĵ��ļ����������Ϊ���ļ����������ء�
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
��������ɵĹ�����
�� �����ļ��Ƿ��Ѿ��򿪣����Ѵ��򷵻�-1������ʾ������Ϣ��
�� ����do_read()������Ŀ¼�ļ������ݵ��ڴ棬����Ŀ¼�������ļ��Ƿ���ڣ����������򷵻�-1������ʾ������Ϣ��
�� ����û����ļ������Ƿ��пձ��������Ϊ�����ļ�����һ���ձ����û���򷵻�-1������ʾ������Ϣ��
�� Ϊ���ļ���д�հ��û����ļ���������ݣ���дָ����Ϊ0��
�� �����ļ������䵽�Ŀհ��û����ļ��������ţ������±꣩��Ϊ�ļ�������fd���ء�
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
    openfilelist[curdir].count = 0;   /*?????��????0  ??curdir??????????????????��?  */
    rbn = do_read(curdir, openfilelist[curdir].length, text);    /*??????????????????? text ?????��?rbn????????????????? */
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
    openfilelist[fd].diroff = i;    /*???????????????��?????????fcb??  */
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
��������ɵĹ�����
�� ���fd����Ч�ԣ�fd���ܳ����û����ļ����������������±꣩�������Ч�򷵻�-1��
�� ����û����ļ�������е�fcbstate�ֶε�ֵ�����Ϊ1����Ҫ�����ļ���FCB�����ݱ��浽��������ϸ��ļ���Ŀ¼���У������ǣ��򿪸��ļ��ĸ�Ŀ¼�ļ����Ը���д��ʽ����do_write()�����ر��ļ���FCBд�븸Ŀ¼�ļ�����Ӧ�̿��У�
�� ���ո��ļ�ռ�ݵ��û����ļ�����������ղ�����������topenfile�ֶ���Ϊ0��
�� ���ء�
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
        openfilelist[father].count = openfilelist[fd].diroff * sizeof(fcb);  /*????close??fd????????????????????????????????????fcb��?????????????��??????fcb??*/
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
��������ɵĹ�����
�� ���fd����Ч�ԣ�fd���ܳ����û����ļ����������������±꣩�������Ч�򷵻�-1������ʾ������Ϣ��
�� ��ʾ���ȴ��û�����д��ʽ����1���ض�д��2������д��3��׷��д��
�� ����û�Ҫ���д��ʽ�ǽض�д�����ͷ��ļ�����һ������������̿ռ����ݣ����Ҳ��޸�FAT�������ڴ��û����ļ��������ļ������޸�Ϊ0������дָ����Ϊ0��ת�ܣ�����û�Ҫ���д��ʽ��׷��д�����޸��ļ��ĵ�ǰ��дָ��λ�õ��ļ���ĩβ����ת�ܣ����д��ʽ�Ǹ���д����ֱ��ת�ܣ�
�� ��ʾ�û���������������ͨ��CTR+Z�����������趨�ļ����������û��ɷֶ������д�����ݣ�ÿ���ûس�������
�� �ȴ��û��Ӽ��������ļ����ݣ������û��ı����������ݱ��浽һ��ʱ����text[]�У�Ҫ��ÿ�������Իس�������ȫ��������CTR+Z�����������趨�ļ�����
�� ����do_write()������ͨ�����̼��������д���ļ��С�
�� ���do_write()�����ķ���ֵΪ�Ǹ�ֵ����ʵ��д���ֽ�������do_write()��������ֵ��������ʾ������Ϣ����ת�᣻
�� ���text[]�����һ���ַ����ǽ����ַ�CTR+Z����ת�߼�������д����������ת�᣻
�� �����ǰ��дָ��λ�ô����û����ļ������е��ļ����ȣ����޸Ĵ��ļ������е��ļ�������Ϣ������fcbstate��1��
�� ����ʵ��д����ֽ�����
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
    while(gets(text))     /*?????��??????????????????,????��text???????????*/
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
��������ɵĹ�����
�� ��malloc()����1024B���ڴ�ռ���Ϊ��д���̵Ļ�����buf������ʧ���򷵻�-1������ʾ������Ϣ��
�� ����дָ��ת��Ϊ�߼����źͿ���ƫ��off�������ô��ļ�������е��׿�ż�FAT���������ݽ��߼�����ת���ɶ�Ӧ�Ĵ��̿���blkno������Ҳ�����Ӧ�Ĵ��̿飬����Ҫ����FATΪ���߼������һ�µĴ��̿飬������Ӧ�Ĵ��̿���blkno�Ǽǵ�FAT�У�������ʧ�ܣ��򷵻�-1������ʾ������Ϣ��
�� ����Ǹ���д�����������ǰ��дָ������Ӧ�Ŀ���ƫ��off������0���򽫿��Ϊblkno��������̿�ȫ��1024B�����ݶ���������buf�У��������ASCII��0���buf��
�� ��text��δд��������ݴ浽������buff�ĵ�off�ֽڿ�ʼ��λ�ã�ֱ���������������߽��յ������ַ�CTR+ZΪֹ��������д���ֽ�����¼��tmplen�У�
�� ��buf��1024B������д�뵽���Ϊblkno��������̿��У�
�޽���ǰ��дָ���޸�Ϊԭ����ֵ����tmplen����������ʵ��д����ֽ�������tmplen��
�� ���tmplenС��len����ת�ڼ���д�룻����ת�ࣻ
�� ���ر���ʵ��д����ֽ�����
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
��������ɵĹ�����
�� ����һ���ַ�������text[len]�����������û����ļ��ж������ļ����ݣ�
�� ���fd����Ч�ԣ�fd���ܳ����û����ļ����������������±꣩�������Ч�򷵻�-1������ʾ������Ϣ��
�� ����do_read()��ָ���ļ��е�len�ֽ����ݶ�����text[]�У�
�� ���do_read()�ķ���ֵΪ��������ʾ������Ϣ������text[]�е�������ʾ����Ļ�ϣ�
�� ���ء�
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
��������ɵĹ�����
�� ʹ��malloc()����1024B�ռ���Ϊ������buf������ʧ���򷵻�-1������ʾ������Ϣ��
�� ����дָ��ת��Ϊ�߼����ż�����ƫ����off�����ô��ļ�������е��׿�Ų���FAT���ҵ����߼������ڵĴ��̿��ţ����ô��̿���ת��Ϊ��������ϵ��ڴ�λ�ã�
�� �����ڴ�λ�ÿ�ʼ��1024B��һ�����̿飩���ݶ���buf�У�
�� �Ƚ�buf�д�ƫ����off��ʼ��ʣ���ֽ����Ƿ���ڵ���Ӧ��д���ֽ���len������ǣ��򽫴�off��ʼ��buf�е�len���ȵ����ݶ��뵽text[]�У����򣬽���off��ʼ��buf�е�ʣ�����ݶ��뵽text[]�У�
�� ����дָ�����Ӣ����Ѷ��ֽ�������Ӧ��д���ֽ���len��ȥ�����Ѷ��ֽ�������len����0����ת�ڣ�����ת�ޣ�
�� ʹ��free()�ͷŢ��������buf��
�� ����ʵ�ʶ������ֽ�����
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
��������ɵĹ�����
�� ����ɾ���ļ��ĸ�Ŀ¼�ļ���û�д򿪣������my_open()�򿪣�����ʧ�ܣ��򷵻أ�����ʾ������Ϣ��
�� ����do_read()�����ø�Ŀ¼�ļ����ݵ��ڴ棬����Ŀ¼����ɾ���ļ��Ƿ���ڣ����������򷵻أ�����ʾ������Ϣ��
�� �����ļ��Ƿ��Ѿ��򿪣����Ѵ���رյ���
�� ���ո��ļ���ռ�ݵĴ��̿飬�޸�FAT��
�� ���ļ��ĸ�Ŀ¼�ļ�����ո��ļ���Ŀ¼���free�ֶ���Ϊ0���Ը���д��ʽ����do_write()��ʵ�֣���
�� �޸ĸø�Ŀ¼�ļ����û����ļ������еĳ�����Ϣ�������ñ����е�fcbstate��Ϊ1��
�� ���ء�
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
��������ɵĹ�����
�� ����������̿ռ䣻
�� ʹ��C���ԵĿ⺯��fopen()��myfsys�ļ������ļ����ڣ���ת�ۣ����ļ������ڣ��򴴽�֮��ת��
�� ʹ��C���ԵĿ⺯��fread()����myfsys�ļ����ݵ��û��ռ��е�һ���������У����ж��俪ʼ��8���ֽ������Ƿ�Ϊ��10101010�����ļ�ϵͳħ����������ǣ���ת�ܣ�����ת�ݣ�
�� �������������е����ݸ��Ƶ��ڴ��е�������̿ռ��У�ת��
�� ����Ļ����ʾ��myfsys�ļ�ϵͳ�����ڣ����ڿ�ʼ�����ļ�ϵͳ����Ϣ��������my_format()�Ԣ������뵽��������̿ռ���и�ʽ��������ת�ޣ�
�� ����������е����ݱ��浽myfsys�ļ��У�ת��
�� ʹ��C���ԵĿ⺯��fclose()�ر�myfsys�ļ���
�� ��ʼ���û����ļ���������0�������Ŀ¼�ļ�ʹ�ã�����д��Ŀ¼�ļ��������Ϣ�����ڸ�Ŀ¼û���ϼ�Ŀ¼�����Ա����е�dirno��diroff�ֱ���Ϊ5����Ŀ¼������ʼ��ţ���0������ptrptrCurDirָ��ָ����û����ļ����
�� ����ǰĿ¼����Ϊ��Ŀ¼��
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
��������ɵĹ�����
�� ʹ��C�⺯��fopen()�򿪴����ϵ�myfsys�ļ���
�� ��������̿ռ��е��������ݱ��浽�����ϵ�myfsys�ļ��У�
�� ʹ��c���ԵĿ⺯��fclose()�ر�myfsys�ļ���
�� �����û����ļ����ͷ����ڴ�ռ�
�� �ͷ�������̿ռ䡣
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
    printf("cd\t\t????(��????)\t\t?��?????????????\n");
    printf("mkdir\t\t????\t\t\t??????????????\n");
    printf("rmdir\t\t????\t\t\t??????????????\n");
    printf("ls\t\t??\t\t\t??????????????????\n");
    printf("create\t\t?????\t\t\t?????????????????\n");
    printf("rm\t\t?????\t\t\t?????????????????\n");
    printf("open\t\t?????\t\t\t???????????????\n");
    printf("write\t\t??\t\t\t???????????��?????\n");
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
                        if(strchr(sp, '.'))/*????sp??'.'??��????��??*/
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
