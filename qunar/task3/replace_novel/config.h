#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

typedef int result_t;                 /* Function return type */
#define R_SUCCESS           0    /* operation success */
#define R_FAILED            1    /* operation failed */
#define DATA_SIZE           6110    //需要替换的字符串大小

#define BIGGER              1
#define SMALLER             0

#define TASK_DATA_PATH "task3.txt"
#define PROPERTY_DATA_PATH "task3.properties"
#define RESULT_DATA_PATH "task3_result.txt"
#define ERROR_LOG "task3_error.log"

#endif // CONFIG_H_INCLUDED
