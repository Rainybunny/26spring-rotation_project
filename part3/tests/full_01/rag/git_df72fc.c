int cmd__genzeros(int argc, const char **argv)
{
	intmax_t count;
	static char zeros[8192]; // Static buffer of zeros

	if (argc > 2) {
		fprintf(stderr, "usage: %s [<count>]\n", argv[0]);
		return 1;
	}

	count = argc > 1 ? strtoimax(argv[1], NULL, 0) : -1;

	while (count < 0 || count > 0) {
		size_t chunk = count < 0 ? sizeof(zeros) : 
			(size_t)(count < sizeof(zeros) ? count : sizeof(zeros));
		if (fwrite(zeros, 1, chunk, stdout) != chunk)
			return -1;
		if (count > 0)
			count -= chunk;
	}

	return 0;
}