static struct strategy *get_strategy(const char *name)
{
	int i;
	struct strategy *ret;
	static struct cmdnames main_cmds, other_cmds;
	static int loaded;
	char *default_strategy = getenv("GIT_TEST_MERGE_ALGORITHM");
	size_t len;

	if (!name)
		return NULL;

	/* Fast path for common case */
	if (default_strategy && !strcmp(default_strategy, "ort") &&
	    !strcmp(name, "recursive")) {
		name = "ort";
	}

	len = strlen(name);
	
	/* Optimized lookup in all_strategy */
	for (i = 0; i < ARRAY_SIZE(all_strategy); i++) {
		if (strlen(all_strategy[i].name) == len &&
		    !strcmp(all_strategy[i].name, name))
			return &all_strategy[i];
	}

	if (!loaded) {
		struct cmdnames not_strategies;
		loaded = 1;

		memset(&not_strategies, 0, sizeof(struct cmdnames));
		load_command_list("git-merge-", &main_cmds, &other_cmds);
		for (i = 0; i < main_cmds.cnt; i++) {
			int j, found = 0;
			struct cmdname *ent = main_cmds.names[i];
			for (j = 0; j < ARRAY_SIZE(all_strategy); j++)
				if (ent->len == strlen(all_strategy[j].name) &&
				    !strncmp(ent->name, all_strategy[j].name, ent->len))
					found = 1;
			if (!found)
				add_cmdname(&not_strategies, ent->name, ent->len);
		}
		exclude_cmds(&main_cmds, &not_strategies);
	}
	if (!is_in_cmdlist(&main_cmds, name) && !is_in_cmdlist(&other_cmds, name)) {
		const char *avail = _("Available strategies are:");
		const char *custom = _("Available custom strategies are:");
		
		fprintf(stderr, _("Could not find merge strategy '%s'.\n"), name);
		fprintf(stderr, "%s", avail);
		for (i = 0; i < main_cmds.cnt; i++)
			fprintf(stderr, " %s", main_cmds.names[i]->name);
		fputs(".\n", stderr);
		if (other_cmds.cnt) {
			fprintf(stderr, "%s", custom);
			for (i = 0; i < other_cmds.cnt; i++)
				fprintf(stderr, " %s", other_cmds.names[i]->name);
			fputs(".\n", stderr);
		}
		exit(1);
	}

	CALLOC_ARRAY(ret, 1);
	ret->name = xstrdup(name);
	ret->attr = NO_TRIVIAL;
	return ret;
}