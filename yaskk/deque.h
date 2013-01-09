struct list_t {
	char c;
	struct list_t *next, *prev;
};

void list_init(struct list_t *list)
{
	*list = NULL;
}

struct list_t *list_create(struct list_t *list, char c)
{
	if (list != NULL)
		return list;

	list = (struct list_t *) emalloc(sizeof(struct list_t));
	list->c = c;
	list->prev = list->next = list;

	return list;
}

struct list_t *list_push_front(struct list_t *list, char c)
{
	struct list_t *new;

	if (list == NULL)
		return list_create(list, c);
	else {
		new = (struct list_t *) emalloc(sizeof(struct list_t));
		new->c = c;
		new->prev = new->next = list;

		return new;
	}
}

void list_push_front_n(struct list_t **list, char c[], int n)
{
	int i;

	for (i = n - 1; i >= 0; i--)
		list_push_front(list, c[i]);
}

struct list_t *list_push_back(struct list_t **list, char c)
{
	struct list_t *new, *lp;

	if (*list == NULL) { /* empty list */
		return list_create(list, c);
	}
	else {
		for (lp = *list; lp->next != NULL; lp = lp->next);
		new = (struct list_t *) emalloc(sizeof(struct list_t));
		new->c = c;
		new->next = NULL;
		lp->next = new;

		return new;
	}
}

void list_push_back_n(struct list_t **list, char c[], int n)
{
	int i;

	for (i = 0; i < n; i++) /* need optimization */
		list_push_back(list, c[i]);
}

void list_erase_front(struct list_t **list)
{
	struct list_t *lp;

	if (*list != NULL) {
		lp = (*list)->next;
		free(*list);
		*list = lp;
	}
}

void list_erase_front_n(struct list_t **list, int n)
{
	int i;

	for (i = 0; i < n; i++)
		list_erase_front(list);
}

/* not implemented
void list_erase_back(struct list_t **list)
{
}
*/

/* not implemented
void list_erase_back_n(struct list_t **list, int n)
{
}
*/

char list_front(struct list_t **list)
{
	return (*list == NULL) ? 0: (*list)->c;
}

void list_front_n(struct list_t **list, char c[], int n)
{
	int i;
	struct list_t *lp = *list;

	for (i = 0; i < n; i++) {
		c[i] = list_front(&lp);
		lp = (lp == NULL) ? NULL: lp->next;
	}
}

char list_back(struct list_t **list)
{
	struct list_t *lp = *list;

	if (lp == NULL) /* empty list */
		return 0;

	while (lp->next != NULL)
		lp = lp->next;

	return lp->c;
}

/* not implemented
void list_back_n(struct list_t **list, char c[], int n)
{
}
*/

int list_size(struct list_t **list)
{
	int count = 0;
	struct list_t *lp;

	for (lp = *list; lp != NULL; lp = lp->next)
		count++;

	return count;
}
