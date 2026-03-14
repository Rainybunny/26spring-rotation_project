static struct strategy *get_strategy(const char *name)
{
	int i;
	struct strategy *ret;
	static struct cmdnames main_cmds, other_cmds;
	static int loaded;
	char *default_strategy = getenv("GIT_TEST_MERGE_ALGORITHM");
	/* Hash table for built-in strategies */
	static struct strategy *strategy_hash[256];
	static int hash_initialized;

	if (!name)
		return NULL;

	/* Initialize strategy hash table if not done yet */
	if (!hash_initialized) {
		hash_initialized = 1;
		for (i = 0; i < ARRAY_SIZE(all_strategy); i++) {
			unsigned hash = all_strategy[i].name[0] % ARRAY_SIZE(strategy_hash);
			strategy_hash[hash] = &all_strategy[i];
		}
	}

	if (default_strategy &&
	    !strcmp(default_strategy, "ort") &&
	    !strcmp(name, "recursive")) {
		name = "ort";
	}

	/* Fast path for built-in strategies */
	if (name[0]) {
		unsigned hash = name[0] % ARRAY_SIZE(strategy_hash);
		if (strategy_hash[hash] && !strcmp(name, strategy_hash[hash]->name))
			return strategy_hash[hash];
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

	CALLOC_ARRAY(ret, 1);
	ret->name = xstrdup(name);
	ret->attr = NO_TRIVIAL;
	return ret;
}