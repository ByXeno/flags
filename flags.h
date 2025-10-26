
#ifndef FLAGS_H_
#define FLAGS_H_

#include <limits.h>

typedef enum {
    as_bool,
    as_i64,
    as_oct,
    as_double,
} flag_type_t;

typedef enum {
    success = 0,
    expected_parameter,
    expected_integer,
    expected_double,
    integer_overflow,
    double_overflow,
    unknown_type,
    unknown_flag,
} flag_error_code_t;

typedef struct {
    const char* flag;
    const char* desc;
    void* addr;
    flag_type_t type;
} flag_t;

bool flags_parse(flag_t* flags,uint32_t count,int32_t* argc,char*** argv);
void flags_order(int32_t* argc,char*** argv);
void flags_error(void);
char* flags_program_name(void);
static char* program_name;
static char* error_value;
static flag_error_code_t error_code;

#endif // FLAGS_H_

#ifdef FLAGS_IMPLEMENTATION

char* flags_program_name(void)
{
    return program_name;
}

bool flags_parse
(flag_t* flags,uint32_t count,int32_t* argc,char*** argv)
{
    program_name = **argv;
    uint32_t i = 1;
    for(;i < *argc;++i)
    {
        char* item = (*argv)[i];
        bool ignore = false;
        if(*item != '-') continue;
        /* Support for both single or double - commands */
        if(*item == '-') {item++;}
        if(*item == '/') {item++;ignore = true;}
        if(*item == '-') {item++;}
        if(*item == '/') {item++;ignore = true;}
        uint32_t j = 0;
        for(;j < count;++j)
        {
            flag_t cur = flags[j];
            char* eq = strchr(item,'=');
            uint32_t cmp = 0;
            if(eq) {
                *eq = 0;
                cmp = strcmp(item,cur.flag);
                *eq = '=';
            }
            else
            {
                cmp = strcmp(item,cur.flag);
            }
            if(cmp == 0)
            {
                char* val_ptr = 0;
                char* end = 0;
                switch(cur.type)
                {
                    case as_bool:{
                        bool val = true;
                        if(eq)
                        {
                            if(strcmp(eq+1,"y") == 0) {val = true;}
                            if(strcmp(eq+1,"yes") == 0) {val = true;}
                            if(strcmp(eq+1,"true") == 0) {val = true;}
                            if(strcmp(eq+1,"t") == 0) {val = true;}

                            if(strcmp(eq+1,"n") == 0) {val = false;}
                            if(strcmp(eq+1,"no") == 0) {val = false;}
                            if(strcmp(eq+1,"false") == 0) {val = false;}
                            if(strcmp(eq+1,"f") == 0) {val = false;}
                        }
                        argv[i] = 0;
                        if(!ignore)
                        {*(bool*)cur.addr = val;}
                    } break;
                    case as_oct:
                    case as_i64:{
                        uint64_t val = 0;
                        if(!eq)
                        {
                            if(i+1 >= *argc)
                            {
                                error_value = (char*)cur.flag;
                                error_code = expected_parameter;
                                return false;
                            }
                            argv[i] = 0;
                            i++;
                            val_ptr = (*argv)[i];
                        }
                        else { val_ptr = eq+1;}
                        val = strtoull(val_ptr,&end,10);
                        if(*end != '\0') {
                            error_value = val_ptr;
                            error_code = expected_integer;
                            return false;
                        }
                        if (val == ULLONG_MAX && errno == ERANGE) {
                            error_value = val_ptr;
                            error_code = integer_overflow;
                            return false;
                        }
                        if(!ignore)
                        {
                            if(cur.type == as_oct)
                            {
                                val = ((val / 100) << 6) | (((val / 10) % 10) << 3) | (val % 10);
                            }
                            *(uint64_t*)cur.addr = val;
                        }
                    } break;
                    case as_double:{
                        double val = 0;
                        if(!eq)
                        {
                            if(i+1 >= *argc)
                            {
                                error_value = (char*)cur.flag;
                                error_code = expected_parameter;
                                return false;
                            }
                            argv[i] = 0;
                            i++;
                            val_ptr = (*argv)[i];
                        }
                        else
                        { val_ptr = eq+1;}
                        val = strtod(val_ptr,&end);
                        if(*end != '\0') {
                            error_value = val_ptr;
                            error_code = expected_double;
                            return false;
                        }
                        if (errno == ERANGE) {
                            error_value = val_ptr;
                            error_code = double_overflow;
                            return false;
                        }
                        if(!ignore)
                        {*(double*)cur.addr = val;}
                    } break;
                    default: {
                        error_value = (char*)cur.flag;
                        error_code =  unknown_type;
                        return false;
                    } break;
                }
                goto end;
            }
        }
        error_value = item;
        error_code = unknown_flag;
        return false;
    end:
        /* Marked as used */
        argv[i] = 0;
    }
    flags_order(argc,argv);
    error_code = success;
    return true;
}

void flags_order(int32_t* argc,char*** argv)
{
    int32_t i = 1;
    for(;i < *argc; ++i) {
        if(argv[i]) continue;
        for (int32_t j = i+1; j < *argc; ++j) {
            if(!(argv[j]))
            {
                argv[i] = argv[j];
                argv[j] = 0;
                break;
            }
        }
        if(!argv[i]) break;
    }
    *argc = i-1;
    (*argv)++;
}

void flags_error(void)
{
    switch(error_code)
    {
        case success: return;
        case expected_parameter: {
            fprintf(stderr,
            "Flag error: '%s' flag is expecting a parametr but got nothing!\n",
            error_value);
        } return;
        case expected_integer: {
            fprintf(stderr,
            "Flag error: '%s' is not an integer!\n",
            error_value);
        } return;
        case expected_double: {
            fprintf(stderr,
            "Flag error: '%s' is not a double!\n",
            error_value);
        } return;
        case integer_overflow: {
            fprintf(stderr,
            "Flag error: '%s' casued and integer overflow!\n",
            error_value);
        } return;
        case double_overflow: {
            fprintf(stderr,
            "Flag error: '%s' casued and double overflow!\n",
            error_value);
        } return;
        case unknown_type: {
            fprintf(stderr,
            "Flag error: '%s' has an unknown type!\n",
            error_value);
        } return;
        case unknown_flag: {
            fprintf(stderr,
            "Flag error: '%s' is not a vaild flag!\n",
            error_value);
        } return;
        default: fprintf(stderr,"Unknown error code: %d\n",error_code);return;
    }
}

#endif // FLAGS_IMPLEMENTATION
