#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    setpriority(atoi(argv[1]), atoi(argv[2]));
    return 0;
}