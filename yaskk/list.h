struct list_t {
	char c;
	struct list_t *next;
};

void list_init(struct list_t **list)
{
	*list = NULL;
}

void list_create(struct list_t **list, char c)
{
	struct list_t *new;

	new = (struct list_t *) emalloc(sizeof(struct list_t));
	new->c = c;
	new->next = NULL;
	*list = new;
}

void list_push_front(struct list_t **list, char c)
{
	struct list_t *new;

	if (*list == NULL) { /* empty list */
		list_create(list, c);
	}
	else {
		new = (struct list_t *) emalloc(sizeof(struct list_t));
		new->c = c;
		new->next = *list;
		*list = new;
	}
}

void list_push_front_n(struct list_t **list, char c[], int n)
{
	int i;

	for (i = n - 1; i >= 0; i--)
		list_push_front(list, c[i]);
}

void list_push_back(struct list_t **list, char c)
{
	struct list_t *new, *lp;

	if (*list == NULL) { /* empty list */
		list_create(list, c);
	}
	else {
		for (lp = *list; lp->next != NULL; lp = lp->next);
		new = (struct list_t *) emalloc(sizeof(struct list_t));
		new->c = c;
		new->next = NULL;
		lp->next = new;
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

void list_erase_back(struct list_t **list)
{
	struct list_t *lp, *prev;

	if (*list == NULL)
		return;
	
	if ((*list)->next == NULL) {
		free(*list);
		*list = NULL;
		return;
	}

	for (lp = prev = *list; lp->next != NULL; lp = lp->next)
		prev = lp;

	free(lp);
	prev->next = NULL;
}

void list_erase_back_n(struct list_t **list, int n)
{
	int i;

	for (i = 0; i < n; i++)
		list_erase_back(list);
}

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

void list_apply(struct list_t **list, int (*func)(int c))
{
	struct list_t *lp;

	for (lp = *list; lp != NULL; lp = lp->next)
		lp->c = func(lp->c);
}
