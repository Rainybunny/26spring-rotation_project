int cmd__genzeros(int argc, const char **argv)
{
	intmax_t count;
	static const char zeros[4096] = {0};

	if (argc > 2) {
		fprintf(stderr, "usage: %s [<count>]\n", argv[0]);
		return 1;
	}

	count = argc > 1 ? strtoimax(argv[1], NULL, 0) : -1;

	if (count < 0) {
		/* Infinite case */
		while (1) {
			if (fwrite(zeros, 1, sizeof(zeros), stdout) < sizeof(zeros))
				return -1;
		}
	} else {
		/* Finite case */
		while (count >= sizeof(zeros)) {
			if (fwrite(zeros, 1, sizeof(zeros), stdout) < sizeof(zeros))
				return -1;
			count -= sizeof(zeros);
		}
		if (count > 0) {
			if (fwrite(zeros, 1, count, stdout) < count)
				return -1;
		}
	}

	return 0;
}