#include "util.h"
#include "list.h"

enum {
	BUFSIZE = 5,
};

int main()
{
	int i;
	char *str = "test", c, array[BUFSIZE];
	struct list_t *list;

	list_init(&list);

	printf("insert some string by list_insert(): \"%s\"\n", str);
	for (i = 0; i < strlen(str); i++)
		list_insert(&list, str[i]);

	printf("size:%d\n", list_size(&list));

	printf("pop all data by list_pop_array()\n");
	list_pop_array(&list, array, BUFSIZE);
	printf("str:%s\n", array);

	printf("size:%d\n", list_size(&list));

	printf("insert some string by list_insert_array(): \"%s\"\n", str);
	list_insert_array(&list, str, strlen(str));

	printf("size:%d\n", list_size(&list));

	printf("pop all data by list_pop()\n");
	while (list != NULL) {
		list_pop(&list, &c);
		printf("c:%c\n", c);
	}

	printf("size:%d\n", list_size(&list));
}
