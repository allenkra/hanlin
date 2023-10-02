#include "types.h"
#include "user.h"

int
main(int argc, char *argv[])
{
    char buf[256];
    if (getlastcat (buf) < 0){
        exit();
    }
    if (buf == 0)
        exit();
    printf(1, "XV6_TEST_OUTPUT Last catted filename: %s\n", buf);
    exit();
}
