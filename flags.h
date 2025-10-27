
#ifndef FLAGS_H_
#define FLAGS_H_

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

typedef enum {
    as_bool,
    as_i64,
    as_oct,
    as_double,
    as_size,
    as_str,
} flag_type_t;

typedef enum {
    success = 0,
    expected_parameter,
    expected_integer,
    expected_double,
    integer_overflow,
    double_overflow,
    invalid_size_suffix,
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
bool flag_size_calculate_multiplier(char* endptr,uint64_t* result);
void flags_help(FILE* out,flag_t* flags,uint32_t count);
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
                    case as_size: {
                        uint64_t val = 0;
                        if(!eq)
                        {
                            if(i+1 >= *argc)
                            {
                                error_value = (char*)cur.flag;
                                error_code = expected_parameter;
                                return false;
                            }
                            (*argv)[i] = 0;
                            i++;
                            val_ptr = (*argv)[i];
                        }
                        else { val_ptr = eq+1;}
                        val = strtoull(val_ptr,&end,10);
                        if (val == ULLONG_MAX && errno == ERANGE) {
                            error_value = val_ptr;
                            error_code = integer_overflow;
                            return false;
                        }
                        if(!flag_size_calculate_multiplier(end,&val))
                        {
                            error_value = end;
                            error_code = invalid_size_suffix;
                            return false;
                        }
                        if(!ignore)
                        {*(uint64_t*)cur.addr = val;}
                    } break;
                    case as_str: {
                        if(!eq)
                        {
                            if(i+1 >= *argc)
                            {
                                error_value = (char*)cur.flag;
                                error_code = expected_parameter;
                                return false;
                            }
                            (*argv)[i] = 0;
                            i++;
                            val_ptr = (*argv)[i];
                        }
                        else { val_ptr = eq+1;}
                        if(!ignore)
                        {*(char**)cur.addr = val_ptr;}
                    } break;
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
                            (*argv)[i] = 0;
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
                                uint64_t oct_val = 0;
                                uint64_t temp = val;
                                uint64_t shift = 0;
                                while (temp > 0) {
                                    oct_val |= (temp % 10) << shift;
                                    shift += 3;
                                    temp /= 10;
                                }
                                val = oct_val;
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
                            (*argv)[i] = 0;
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
        (*argv)[i] = 0;
    }
    flags_order(argc,argv);
    error_code = success;
    return true;
}

void flags_order(int32_t* argc, char*** argv)
{
    int32_t out = 0;
    for (int32_t i = 1; i < *argc; ++i) {
        if ((*argv)[i])
            (*argv)[out++] = (*argv)[i];
    }
    *argc = out;
}


void flags_help(FILE*out,flag_t* flags,uint32_t count)
{
    uint32_t i = 0;
    fprintf(out,"Usage:\n");
    for(;i < count;++i)
    {
        flag_t cur = flags[i];
        switch(cur.type)
        {
            case as_bool: {
                fprintf(out,"    -%s / -%s=n/y/t/f\n",cur.flag,cur.flag);
            } break;
            case as_i64: {
                fprintf(out,"    -%s=<int> / -%s <int>\n",cur.flag,cur.flag);
            } break;
            case as_str: {
                fprintf(out,"    -%s=<str> / -%s <str>\n",cur.flag,cur.flag);
            } break;
            case as_size: {
                fprintf(out,"    -%s=<size><suffix> / -%s <str><suffix>\n",cur.flag,cur.flag);
            } break;
            case as_double: {
                fprintf(out,"    -%s=<double> / -%s <double>\n",cur.flag,cur.flag);
            } break;
            case as_oct: {
                fprintf(out,"    -%s=<oct> / -%s <oct>\n",cur.flag,cur.flag);
            } break;
        }
        fprintf(out,"    Description:%s\n",cur.desc);
    }
}

void flags_error(void)
{
    switch(error_code)
    {
        case success: return;
        case expected_parameter: {
            fprintf(stderr,
            "Flag error: '%s' flag expects a parameter but got nothing!\n",
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
            "Flag error: '%s' caused and integer overflow!\n",
            error_value);
        } return;
        case double_overflow: {
            fprintf(stderr,
            "Flag error: '%s' caused and double overflow!\n",
            error_value);
        } return;
        case unknown_type: {
            fprintf(stderr,
            "Flag error: '%s' has an unknown type!\n",
            error_value);
        } return;
        case unknown_flag: {
            fprintf(stderr,
            "Flag error: '%s' is not a valid flag!\n",
            error_value);
        } return;
        default: fprintf(stderr,"Unknown error code: %d\n",error_code);return;
    }
}

bool flag_size_calculate_multiplier(char* endptr,uint64_t* result)
{
    if (strcmp(endptr, "c") == 0) {
        (*result) *= 1ULL;
    } else if (strcmp(endptr, "w") == 0) {
        (*result) *= 2ULL;
    } else if (strcmp(endptr, "b") == 0) {
        (*result) *= 512ULL;
    } else if (strcmp(endptr, "kB") == 0) {
        (*result) *= 1000ULL;
    } else if (strcmp(endptr, "K") == 0 || strcmp(endptr, "KiB") == 0) {
        (*result) *= 1024ULL;
    } else if (strcmp(endptr, "MB") == 0) {
        (*result) *= 1000ULL * 1000ULL;
    } else if (strcmp(endptr, "M") == 0 || strcmp(endptr, "MiB") == 0 || strcmp(endptr, "xM") == 0) {
        (*result) *= 1024ULL * 1024ULL;
    } else if (strcmp(endptr, "GB") == 0) {
        (*result) *= 1000ULL * 1000ULL * 1000ULL;
    } else if (strcmp(endptr, "G") == 0 || strcmp(endptr, "GiB") == 0) {
        (*result) *= 1024ULL * 1024ULL * 1024ULL;
    } else if (strcmp(endptr, "TB") == 0) {
        (*result) *= 1000ULL * 1000ULL * 1000ULL * 1000ULL;
    } else if (strcmp(endptr, "T") == 0 || strcmp(endptr, "TiB") == 0) {
        (*result) *= 1024ULL * 1024ULL * 1024ULL * 1024ULL;
    } else if (strcmp(endptr, "PB") == 0) {
        (*result) *= 1000ULL * 1000ULL * 1000ULL * 1000ULL * 1000ULL;
    } else if (strcmp(endptr, "P") == 0 || strcmp(endptr, "PiB") == 0) {
        (*result) *= 1024ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL;
    } else if (strcmp(endptr, "EB") == 0) {
        (*result) *= 1000ULL * 1000ULL * 1000ULL * 1000ULL * 1000ULL * 1000ULL;
    } else if (strcmp(endptr, "E") == 0 || strcmp(endptr, "EiB") == 0) {
        (*result) *= 1024ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL;
    } else if (strcmp(endptr, "ZB") == 0) {
        (*result) *= 1000ULL * 1000ULL * 1000ULL * 1000ULL * 1000ULL * 1000ULL * 1000ULL;
    } else if (strcmp(endptr, "Z") == 0 || strcmp(endptr, "ZiB") == 0) {
        (*result) *= 1024ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL;
    } else if (strcmp(endptr, "YB") == 0) {
        (*result) *= 1000ULL * 1000ULL * 1000ULL * 1000ULL * 1000ULL * 1000ULL * 1000ULL * 1000ULL;
    } else if (strcmp(endptr, "Y") == 0 || strcmp(endptr, "YiB") == 0) {
        (*result) *= 1024ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL;
     } else if (strcmp(endptr, "") != 0) {return false;}
     return true;
 }

#endif // FLAGS_IMPLEMENTATION

/*
 *  flags - simple command-line flag parser in C
 *  Copyright (C) 2025 Menderes Sabaz <sabazmenders@proton.me>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
