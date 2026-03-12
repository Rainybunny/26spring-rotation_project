static struct strategy *get_strategy(const char *name)
{
	int i;
	struct strategy *ret;
	static struct cmdnames main_cmds, other_cmds;
	static int loaded;
	char *default_strategy = getenv("GIT_TEST_MERGE_ALGORITHM");

	if (!name)
		return NULL;

	/* Handle default strategy override first */
	if (default_strategy &&
	    !strcmp(default_strategy, "ort") &&
	    name == all_strategy[0].name) { /* "recursive" pointer comparison */
		name = "ort";
	} else if (default_strategy &&
		   !strcmp(default_strategy, "ort") &&
		   !strcmp(name, "recursive")) {
		name = "ort";
	}

	/* First try pointer comparison for builtin strategies */
	for (i = 0; i < ARRAY_SIZE(all_strategy); i++) {
		if (name == all_strategy[i].name || /* pointer comparison first */
		    !strcmp(name, all_strategy[i].name)) {
			return &all_strategy[i];
		}
	}

	/* Only load external commands if we didn't find a builtin strategy */
	if (!loaded) {
		struct cmdnames not_strategies;
		loaded = 1;

		memset(&not_strategies, 0, sizeof(struct cmdnames));
		load_command_list("git-merge-", &main_cmds, &other_cmds);
		for (i = 0; i < main_cmds.cnt; i++) {
			struct cmdname *ent = main_cmds.names[i];
			int j, found = 0;
			for (j = 0; j < ARRAY_SIZE(all_strategy); j++) {
				if (ent->name == all_strategy[j].name || /* pointer compare */
				    (!strncmp(ent->name, all_strategy[j].name, ent->len) &&
				     !all_strategy[j].name[ent->len])) {
					found = 1;
					break;
				}
			}
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