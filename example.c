#define FLAGS_IMPLEMENTATION
#include "flags.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool no_color;
    uint64_t off_count;
    double scale;
    uint64_t size;
    char* name;
} config_t;

config_t conf = {
    .no_color = false,
    .off_count = 0,
    .scale = 1.0,
    .size = 0,
    .name = NULL,
};
bool help = false;
flag_t flags[] = {
    { .flag = "nocolor", .desc = "Prints output without color", .addr = &conf.no_color, .type = as_bool },
    { .flag = "off",     .desc = "Sets the off count (integer)", .addr = &conf.off_count, .type = as_i64 },
    { .flag = "scale",   .desc = "Scaling factor (double)", .addr = &conf.scale, .type = as_double },
    { .flag = "size",    .desc = "Buffer size with suffix (e.g. 10K, 2M)", .addr = &conf.size, .type = as_size },
    { .flag = "name",    .desc = "Sets a name (string)", .addr = &conf.name, .type = as_str },
    { .flag = "help",    .desc = "prints the help text", .addr = &help, .type = as_bool },
};

int main(int argc, char** argv)
{
    if (!flags_parse(flags, sizeof(flags)/sizeof(flag_t), &argc, &argv)) {
        flags_error();
        fprintf(stderr, "Use --help for usage info.\n");
        return 1;
    }

    // Handle help flag manually
    if(help){
        flags_help(stdout, flags, sizeof(flags)/sizeof(flag_t));
        return 0;
    }

    printf("=== Parsed Configuration ===\n");
    printf("Program name : %s\n", flags_program_name());
    printf("No color     : %s\n", conf.no_color ? "true" : "false");
    printf("Off count    : %llu\n", (unsigned long long)conf.off_count);
    printf("Scale        : %.3f\n", conf.scale);
    printf("Size         : %llu bytes\n", (unsigned long long)conf.size);
    printf("Name         : %s\n", conf.name ? conf.name : "(none)");

    // Remaining args (non-flags)
    printf("\n=== Remaining Arguments ===\n");
    for (int i = 0; i < argc; ++i) {
        printf("[%d] %s\n", i, argv[i]);
    }

    return 0;
}
