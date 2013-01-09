#include <stdio.h>
#include <string.h>

int main()
{
	printf("ret:%d len:%d\n", strncmp("z", "z ", 2), strlen("z "));
}
