static struct strategy *get_strategy(const char *name)
{
	int i;
	static struct cmdnames main_cmds, other_cmds;
	static int loaded;
	char *default_strategy = getenv("GIT_TEST_MERGE_ALGORITHM");
	static struct hashmap strategy_cache;
	static int cache_initialized;

	if (!name)
		return NULL;

	/* Initialize cache on first call */
	if (!cache_initialized) {
		hashmap_init(&strategy_cache, (hashmap_cmp_fn)strcmp, NULL, 0);
		cache_initialized = 1;
	}

	/* Check cache first */
	struct strategy *cached = hashmap_get_from_hash(&strategy_cache,
					strhash(name), name);
	if (cached)
		return cached;

	if (default_strategy &&
	    !strcmp(default_strategy, "ort") &&
	    !strcmp(name, "recursive")) {
		name = "ort";
	}

	/* Check built-in strategies */
	for (i = 0; i < ARRAY_SIZE(all_strategy); i++) {
		if (!strcmp(name, all_strategy[i].name)) {
			/* Cache the result */
			hashmap_add(&strategy_cache,
				strategy_entry(&all_strategy[i], strhash(name)));
			return &all_strategy[i];
		}
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
				if (!strncmp(ent->name, all_strategy[j].name, ent->len)
						&& !all_strategy[j].name[ent->len])
					found = 1;
			if (!found)
				add_cmdname(&not_strategies, ent->name, ent->len);
		}
		exclude_cmds(&main_cmds, &not_strategies);
	}

	if (!is_in_cmdlist(&main_cmds, name) && !is_in_cmdlist(&other_cmds, name)) {
		fprintf(stderr, _("Could not find merge strategy '%s'.\n"), name);
		fprintf(stderr, _("Available strategies are:"));
		for (i = 0; i < main_cmds.cnt; i++)
			fprintf(stderr, " %s", main_cmds.names[i]->name);
		fprintf(stderr, ".\n");
		if (other_cmds.cnt) {
			fprintf(stderr, _("Available custom strategies are:"));
			for (i = 0; i < other_cmds.cnt; i++)
				fprintf(stderr, " %s", other_cmds.names[i]->name);
			fprintf(stderr, ".\n");
		}
		exit(1);
	}

	struct strategy *ret;
	CALLOC_ARRAY(ret, 1);
	ret->name = xstrdup(name);
	ret->attr = NO_TRIVIAL;

	/* Cache the result */
	hashmap_add(&strategy_cache,
		strategy_entry(ret, strhash(name)));

	return ret;
}