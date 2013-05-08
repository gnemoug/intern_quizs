#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <semaphore.h>
#include <pthread.h>
//regex
#include <regex.h>
#include "config.h"

//cns_reg函数的返回类型
typedef struct _reg_rtn_struct
{
   int rtn;       //成功与否标志0 成功， 1失败
   int pstart;    //匹配到的子串开始位移
   int pend;      //匹配到的子串尾部位移
} reg_rtn_struct;

typedef struct _property_node
{
    int index;  //原文件的index
    char* data; //原文件的data
}property_node,* pproperty_node;

typedef struct _arraynode
{
    char* data;  //数组数据指针
}arraynode,* parraynode;

FILE *ResultFile;   //存储结果信息

parraynode natureOrder;//存储natureOrder索引信息
parraynode indexOrder;//存储indexOrder索引信息
parraynode charOrder;//存储charOrder索引信息
parraynode charOrderDESC;//存储charOrderDESC索引信息
pproperty_node property_data;//存储PROPERTY_DATA_PATH中的所有信息

const size_t nmatch = 10;  //The size of array pm[]
regmatch_t   pm[10];       //pattern matches 0-9

/*定义函数指针类型*/  
typedef result_t (*CompFun)(char *, char *); 

//compare functions
static result_t
charOrderComp(char *a,char *b);
static result_t
charOrderDESCComp(char *a,char *b);
//init property_data
static result_t
InitPropertyData(void);
//index replace functions
static result_t
orderIndex(void);
static result_t
natureOrderIndex(void);
static result_t
indexOrderIndex(void);
static result_t
charOrderIndex(void);
static result_t
charOrderDESCIndex(void);
// Function declare.
static result_t
ReadFromFile(void);
// write logs
static void 
DBLogging(const char *filePath,const char *logString);
//free memory
static void 
destory(void);

static result_t
charOrderComp(char *a,char *b){
    if(strcmp(a,b) < 0){
        return SMALLER;
    }
    else{
        return BIGGER;
    }
}

static result_t
charOrderDESCComp(char *a,char *b){
    if(strcmp(a,b) < 0){
        return BIGGER;
    }
    else{
        return SMALLER;
    }
}

int 
partion(parraynode data, int s, int e,CompFun compareFun){
    int start = s;
    int end = e;

    char* temp = (data + s)->data;
    while( start < end ){
        while(start < end && compareFun((data + end)->data,temp)){
            end--;
        }

        if(start < end){
            (data + start)->data = (data + end)->data;
            start++;
        }

        while(start < end && compareFun(temp,(data + start)->data)){
            start++;
        }

        if(start < end){
            (data + end)->data = (data + start)->data;
            end--;
        }
    }

    (data + start)->data = temp;
    return start;
}

void 
quickSort(parraynode data, int s, int e,CompFun compareFun){
    if(s < e){
        int temp = partion(data,s,e,compareFun);
        quickSort(data,s,temp-1,compareFun);
        quickSort(data, temp+1, e,compareFun);
    }
}

static result_t
InitPropertyData(void){
    char *lim = NULL; 
    char *tmp = NULL;
    FILE * property_file;
    char buffer[256];
    char logString[256];

    if(access(PROPERTY_DATA_PATH, F_OK) == -1){        //用于判断文件是否存在
        memset(logString, 0, 256);
        snprintf(logString,256,"[ERROR] replace_novel.c @ InitPropertyData @ access --- %s", strerror(errno));
        DBLogging(ERROR_LOG, logString);
        return R_FAILED;
    }

    property_file = fopen(PROPERTY_DATA_PATH,"r");
    if(property_file == NULL){        //访问被禁止
        memset(logString, 0, 256);
        snprintf(logString,256,"[ERROR] replace_novel.c @ InitPropertyData @ fopen --- %s", strerror(errno));
        DBLogging(ERROR_LOG, logString);
        return R_FAILED;
    }

    property_data = (property_node*)calloc(sizeof(property_node),DATA_SIZE);
    
    int i = 0;//the index
    while(fgets(buffer, sizeof(buffer),property_file) != NULL){
        lim = strtok(buffer,"\t");
        (property_data + i)->index = atoi((const char*)lim);
        lim = strtok(NULL,"\t");
        tmp = (char*)calloc(sizeof(char), strlen(lim)-1);
        strncpy(tmp, lim, strlen(lim)-2);
        tmp[strlen(lim)-2] = '\0';
        (property_data + i)->data = tmp;
        i++;
    }
    
    //For test
/*    for(i = 0;i < DATA_SIZE;i++){*/
/*        printf("%d",(property_data + i)->index);*/
/*        printf("%s",(property_data + i)->data);*/
/*    }*/
    fclose(property_file);

    return R_SUCCESS;
}

static result_t
natureOrderIndex(void){
    int i = 0;
    
    for(i = 0;i < DATA_SIZE;i++){
        (natureOrder + i)->data = (property_data + i)->data;
    }
    
    //For test
/*    for(i = 0;i < DATA_SIZE;i++){*/
/*        printf("%s\n",(natureOrder + i)->data);*/
/*    }*/
    return R_SUCCESS;
}

static result_t
indexOrderIndex(void){
    int i = 0;
    
    for(i = 0;i < DATA_SIZE;i++){
        (indexOrder + (property_data + i)->index)->data = (property_data + i)->data;
    }
    
    //For test
/*    for(i = 0;i < DATA_SIZE;i++){*/
/*        printf("%s\n",(indexOrder + i)->data);*/
/*    }*/
    
    return R_SUCCESS;
}

static result_t
charOrderIndex(void){
    int i = 0;
    
    for(i = 0;i < DATA_SIZE;i++){
        (charOrder + i)->data = (property_data + i)->data;
    }
    
    quickSort(charOrder,0,DATA_SIZE-1,charOrderComp);

    //For test
/*    for(i = 0;i < DATA_SIZE;i++){*/
/*        printf("%s\n",(charOrder + i)->data);*/
/*    }*/
    return R_SUCCESS;
}

static result_t
charOrderDESCIndex(void){
    int i = 0;
    
    for(i = 0;i < DATA_SIZE;i++){
        (charOrderDESC + i)->data = (property_data + i)->data;
    }
    
    quickSort(charOrderDESC,0,DATA_SIZE-1,charOrderDESCComp);

    //For test
/*    for(i = 0;i < DATA_SIZE;i++){*/
/*        printf("%s\n",(charOrderDESC + i)->data);*/
/*    }*/
    return R_SUCCESS;
}

static result_t
orderIndex(void){
/*    pid_t natureOrderpid,indexOrderpid,charOrderpid,charOrderDESCpid;*/
    
    pthread_t natureOrderpid,indexOrderpid,charOrderpid,charOrderDESCpid;
    
    natureOrder = (arraynode*)calloc(sizeof(arraynode),DATA_SIZE);
    charOrder = (arraynode*)calloc(sizeof(arraynode),DATA_SIZE);
    indexOrder = (arraynode*)calloc(sizeof(arraynode),DATA_SIZE);
    charOrderDESC = (arraynode*)calloc(sizeof(arraynode),DATA_SIZE);

    pthread_create(&natureOrderpid,NULL,(void*)natureOrderIndex,NULL);
    pthread_create(&indexOrderpid,NULL,(void*)indexOrderIndex,NULL);
    pthread_create(&charOrderpid,NULL,(void*)charOrderIndex,NULL);
    pthread_create(&charOrderDESCpid,NULL,(void*)charOrderDESCIndex,NULL);

    pthread_join(natureOrderpid,NULL);
    pthread_join(indexOrderpid,NULL);
    pthread_join(charOrderpid,NULL);
    pthread_join(charOrderDESCpid,NULL);

    return R_SUCCESS;
    
    //TODO:错误原因：复制出来的子进程的task_struct结构与系统的堆栈空间是与父进程独立的，但其他资源却是与父进程共享的，比如文件指针，socket描述符等。
/*    natureOrderpid = fork();*/
/*    if(natureOrderpid < 0){// fork错误*/
/*        printf("natureOrderpid fork error\n");*/
/*        exit(1);*/
/*    }else if(natureOrderpid == 0){//在子进程中*/
/*        natureOrderIndex();*/
/*        exit(0);*/
/*    }else{*/
/*        indexOrderpid = fork();*/
/*        if(indexOrderpid < 0){// fork错误*/
/*            printf("indexOrderpid fork error\n");*/
/*            exit(1);*/
/*        }else if(indexOrderpid == 0){//在子进程中*/
/*            indexOrderIndex();*/
/*            exit(0);*/
/*        }else{*/
/*            charOrderpid = fork();*/
/*            if(charOrderpid < 0){// fork错误*/
/*                printf("charOrderpid fork error\n");*/
/*                exit(1);*/
/*            }else if(charOrderpid == 0){//在子进程中*/
/*                charOrderIndex();*/
/*                exit(0);*/
/*            }else{*/
/*                charOrderDESCpid = fork();*/
/*                if(charOrderDESCpid < 0){// fork错误*/
/*                    printf("charOrderDESCpid fork error\n");*/
/*                    exit(1);*/
/*                }else if(charOrderDESCpid == 0){//在子进程中*/
/*                    charOrderDESCIndex();*/
/*                    exit(0);*/
/*                }else{*/
/*                    waitpid(natureOrderpid,NULL,0);*/
/*                    waitpid(indexOrderpid,NULL,0);*/
/*                    waitpid(charOrderpid,NULL,0);*/
/*                    waitpid(charOrderDESCpid,NULL,0);*/
/*                    return R_SUCCESS;*/
/*                }*/
/*            }*/
/*        }*/
/*    }*/
}

/**
*
* 正则表达式查找函数
*/
reg_rtn_struct 
cns_reg(const char *str,  const char *pattern){
    reg_rtn_struct reg_rtn_struct_var;

    int          z;            //status
    int          pos;          //配置处的位置
    int          cflags = REG_EXTENDED;   //compile flags
    regex_t      reg;          //compiled regular expression
    char         ebuf[128];    //error buffer
    char         logString[256];//log error
    bzero(ebuf, sizeof(ebuf));
    bzero(pm, sizeof(pm));


    //编译正则表达式
    /**
     *
     * @param const char*  pattern         将要被编译的正则表达式
     * @param regex_t*     reg             用来保存编译结果
     * @param int          cflags          决定正则表达式将如何被处理的细节
     *
     * @return  success    int        0    并把编译结果填充到reg结构中
     *          fail       int        非0
     *
     */
    z = regcomp(&reg, (const char*)pattern, cflags);

    if(z){   //此处为 if(z != 0), 因为C语言里0永远为非(False), 任何非0值都为真(True)
        regerror(z, &reg, ebuf, sizeof(ebuf));
        perror("regcomp");
        fprintf(stderr, "%s: pattern '%s'\n", ebuf, pattern);
        reg_rtn_struct_var.rtn    = 1;
        reg_rtn_struct_var.pstart = -1;
        reg_rtn_struct_var.pend   = -1;
        
        memset(logString, 0, 256);
        snprintf(logString,256,"[ERROR] replace_novel.c @ cns_reg @ regcomp --- %s", strerror(errno));
        DBLogging(ERROR_LOG, logString);
        
        regfree(&reg);
        return reg_rtn_struct_var;
    }

    /**
     *
     * reg     指向编译后的正则表达式
     * str     指向将要进行匹配的字符串
     * pm      str字符串中可能有多处和正则表达式相匹配， pm数组用来保存这些位置
     * nmacth  指定pm数组最多可以存放的匹配位置数
     *
     * @return 函数匹配成功后，str+pm[0].rm_so到str+pm[0].rm_eo是第一个匹配的子串
     *                           str+pm[1].rm_so到str+pm[1].rm_eo是第二个匹配的子串
     *                           ....
     */
    z = regexec(&reg, str, nmatch, pm, REG_EXTENDED);

    //没有找到匹配数据
    if(z == REG_NOMATCH){
        reg_rtn_struct_var.rtn    = 1;
        reg_rtn_struct_var.pstart = -1;
        reg_rtn_struct_var.pend   = -1;

        regfree(&reg);
        return reg_rtn_struct_var;
    }
    else if(z){
        perror("regexec");
        regerror(z, &reg, ebuf, sizeof(ebuf));
        fprintf(stderr, "%s: regcomp('%s')\n", ebuf, str);

        reg_rtn_struct_var.rtn    = 1;
        reg_rtn_struct_var.pstart = -1;
        reg_rtn_struct_var.pend   = -1;
        regfree(&reg);

        memset(logString, 0, 256);
        snprintf(logString,256,"[ERROR] replace_novel.c @ cns_reg @ regexec --- %s", strerror(errno));
        DBLogging(ERROR_LOG, logString);

        return reg_rtn_struct_var;
    }

    /*列出匹配的位置*/
    if(pm[0].rm_so != -1){
       reg_rtn_struct_var.rtn    = 0;
       reg_rtn_struct_var.pstart = pm[0].rm_so;
       reg_rtn_struct_var.pend   = pm[0].rm_eo;
    }

    regfree(&reg);
    return reg_rtn_struct_var;
}


/*
* 正则表达式替换函数
*/
static result_t 
cns_str_ereplace(char *src, const char *pattern){
    int i = 0;
    char *tmp;
    char *num;
    const char *newsubstr  = NULL;
/*     char **order = {"natureOrder","indexOrder","charOrder","charOrderDESC"};*/

    //定义cns_reg的返回类型结构变量
    reg_rtn_struct  reg_rtn_struct_var;
    int rtn    = 0;   //reg_rtn_struct_var.rtn
    int pstart = 0;   //reg_rtn_struct_var.pstart
    int pend   = 0;   //reg_rtn_struct_var.pend
    
    //newsubstr
    while(!rtn){
        reg_rtn_struct_var = cns_reg(src, pattern);

        rtn    = reg_rtn_struct_var.rtn;
        pstart = reg_rtn_struct_var.pstart;
        pend   = reg_rtn_struct_var.pend;

        if(!rtn){
            if((pm[1].rm_eo - pm[1].rm_so) == 9){//charOrder
                num = (char*)calloc(sizeof(char),pm[3].rm_eo - pm[3].rm_so);
                for(i = 0;i <= pm[3].rm_eo - pm[3].rm_so;i++){
                    num[i] = src[pm[3].rm_so + i];
                }
                newsubstr = (charOrder + atoi((const char*)num))->data;
            }else if((pm[1].rm_eo - pm[1].rm_so) == 10){//indexOrder
                num = (char*)calloc(sizeof(char),pm[3].rm_eo - pm[3].rm_so);
                for(i = 0;i <= pm[3].rm_eo - pm[3].rm_so;i++){
                    num[i] = src[pm[3].rm_so + i];
                }
                newsubstr = (indexOrder + atoi((const char*)num))->data;
            }else if((pm[1].rm_eo - pm[1].rm_so) == 11){//natureOrder
                num = (char*)calloc(sizeof(char),pm[3].rm_eo - pm[3].rm_so);
                for(i = 0;i <= pm[3].rm_eo - pm[3].rm_so;i++){
                    num[i] = src[pm[3].rm_so + i];
                }
                newsubstr = (natureOrder + atoi((const char*)num))->data;
            }else{//charOrderDESC
                num = (char*)calloc(sizeof(char),pm[3].rm_eo - pm[3].rm_so);
                for(i = 0;i <= pm[3].rm_eo - pm[3].rm_so;i++){
                    num[i] = src[pm[3].rm_so + i];
                }
                newsubstr = (charOrderDESC + atoi((const char*)num))->data;
            }
            
            //For debug
/*            printf("%d ",pm[0].rm_so);*/
/*            printf("%d ",pm[1].rm_so);*/
/*            printf("%d ",pm[2].rm_so);*/
/*            printf("%d ",pm[3].rm_so);*/
/*            printf("\n");*/
            tmp = (char*)calloc(sizeof(char),pstart + 1);
            strncpy(tmp, src, pstart);
            tmp[pstart] = '\0';
            fprintf(ResultFile,"%s%s",tmp,newsubstr);
/*            fwrite(src,1,pstart,ResultFile);*/
/*            fwrite("shit",1,sizeof("shit"),ResultFile);*/
            //释放malloc分配的内存空间
            src = src + pend;
            free(tmp);
        }
        else{
/*            fwrite(src,1,sizeof(src),ResultFile);*/
            fprintf(ResultFile,"%s",src);
        }
    }
    
    return R_SUCCESS;
}

static result_t 
ReadFromFile(void){
    FILE * fin;
    void *start;
    char buffer[256];
    struct stat sb;
    result_t result;
    char logString[256];

    if(access(TASK_DATA_PATH, F_OK) == -1){        //用于判断文件是否存在
        memset(logString, 0, 256);
        snprintf(logString,256,"[ERROR] replace_novel.c @ ReadFromFile @ access --- %s", strerror(errno));
        DBLogging(ERROR_LOG, logString);
        return R_FAILED;
    }

    fin = fopen(TASK_DATA_PATH,"r");
    if(fin == NULL){        //访问被禁止
        memset(logString, 0, 256);
        snprintf(logString,256,"[ERROR] replace_novel.c @ ReadFromFile @ open --- %s", strerror(errno));
        DBLogging(ERROR_LOG, logString);
        return R_FAILED;
    }
    
/*    fstat(fin,&sb);     //获取文件信息*/
/*    if(sb.st_size > 0){  //判断文件大小*/
/*        start = mmap(NULL,sb.st_size,PROT_READ,MAP_PRIVATE,fin,0);*/
/*    }*/
/*    else{*/
/*        close(fin);*/
/*        return R_FAILED;*/
/*    }*/

/*    if(start == MAP_FAILED){    // 判断是否映射成功
/*        close(fin);*/
/*        memset(logString, 0, 256);*/
/*        snprintf(logString,256,"[ERROR] dbDomain.c @ CreateFromFile @ mmap --- %s", strerror(errno));*/
/*        DBLogging(ERROR_LOG, logString);*/
/*        return R_FAILED;*/
/*    }*/

    if((ResultFile = fopen(RESULT_DATA_PATH,"w")) == NULL){ 
        perror("fopen");
        memset(logString, 0, 256);
        snprintf(logString,256,"[ERROR] replace_novel.c @ ReadFromFile @ open2 --- %s", strerror(errno));
        DBLogging(ERROR_LOG, logString);
        return R_FAILED;
    }

    while(fgets(buffer, sizeof(buffer),fin) != NULL){
        if(cns_str_ereplace(buffer, "\\$(.{4,6}Order(DESC)?)\\(([0-9]{1,4})\\)") != R_SUCCESS){
            perror("cns_str_ereplace");
            memset(logString, 0, 256);
            snprintf(logString,256,"[ERROR] replace_novel.c @ ReadFromFile @ cns_str_ereplace --- %s", strerror(errno));
            DBLogging(ERROR_LOG, logString);
        }
    }
    fclose(ResultFile);
/*    munmap(start,sb.st_size); //解除映射*/

    return R_SUCCESS;
}

static void 
destory(void){
    free(natureOrder);
    free(indexOrder);
    free(charOrder);
    free(charOrderDESC);
    free(property_data);
}

static void
DBLogging(const char *filePath, const char *logString){
    char timeLog[32];
    FILE *LogFile;
    time_t now = time(NULL);
    strftime(timeLog,sizeof(timeLog),"%d/%b/%Y:%H:%M:%S %Z",localtime(&now));

    if((LogFile = fopen(filePath,"a+")) == NULL){ 
        perror("fopen");
        return;
    }
    fprintf(LogFile,"[%s]  ",timeLog);
    fprintf(LogFile,"%s\n",logString);
    fclose(LogFile);
}

int main(int argc, char** argv){
    struct timeval tv_start;
    struct timeval tv_end;
/*    char timeLog[32];*/
/*    time_t start = time(NULL);*/
/*    strftime(timeLog,sizeof(timeLog),"%d/%b/%Y:%H:%M:%S %Z",localtime(&start));*/
/*    puts(timeLog);*/
    gettimeofday(&tv_start,NULL);
    InitPropertyData();
    gettimeofday(&tv_end,NULL);
    if((unsigned int)tv_start.tv_usec < (unsigned int)tv_end.tv_usec){
       printf("the time of InitPropertyData is:%u seconds and %u microseconds.\n",(unsigned int)tv_end.tv_sec - (unsigned int)tv_start.tv_sec,(unsigned int)tv_end.tv_usec - (unsigned int)tv_start.tv_usec);  
    }else{
       printf("the time of InitPropertyData is:%u seconds and %u microseconds.\n",(unsigned int)tv_end.tv_sec - (unsigned int)tv_start.tv_sec - 1,(unsigned int)tv_end.tv_usec - (unsigned int)tv_start.tv_usec + 1000000);
    }
    
    gettimeofday(&tv_start,NULL);
    orderIndex();
    gettimeofday(&tv_end,NULL);
    if((unsigned int)tv_start.tv_usec < (unsigned int)tv_end.tv_usec){
       printf("the time of orderIndex is:%u seconds and %u microseconds.\n",(unsigned int)tv_end.tv_sec - (unsigned int)tv_start.tv_sec,(unsigned int)tv_end.tv_usec - (unsigned int)tv_start.tv_usec);  
    }else{
       printf("the time of orderIndex is:%u seconds and %u microseconds.\n",(unsigned int)tv_end.tv_sec - (unsigned int)tv_start.tv_sec - 1,(unsigned int)tv_end.tv_usec - (unsigned int)tv_start.tv_usec + 1000000);
    }
    
    gettimeofday(&tv_start,NULL);
    ReadFromFile();
    gettimeofday(&tv_end,NULL);
    if((unsigned int)tv_start.tv_usec < (unsigned int)tv_end.tv_usec){
       printf("the time of orderIndex is:%u seconds and %u microseconds.\n",(unsigned int)tv_end.tv_sec - (unsigned int)tv_start.tv_sec,(unsigned int)tv_end.tv_usec - (unsigned int)tv_start.tv_usec);  
    }else{
       printf("the time of orderIndex is:%u seconds and %u microseconds.\n",(unsigned int)tv_end.tv_sec - (unsigned int)tv_start.tv_sec - 1,(unsigned int)tv_end.tv_usec - (unsigned int)tv_start.tv_usec + 1000000);
    }
    
    destory();
/*    time_t end = time(NULL);*/
/*    strftime(timeLog,sizeof(timeLog),"%d/%b/%Y:%H:%M:%S %Z",localtime(&end));*/
/*    puts(timeLog);*/
    
    return 0;
}
