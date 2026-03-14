int recv_sideband(const char *me, int in_stream, int out)
{
	const char *term, *suffix;
	char buf[LARGE_PACKET_MAX + 1];
	struct strbuf outbuf = STRBUF_INIT;
	int retval = 0;
	size_t outbuf_len;

	term = getenv("TERM");
	if (isatty(2) && term && strcmp(term, "dumb"))
		suffix = ANSI_SUFFIX;
	else
		suffix = DUMB_SUFFIX;

	while (!retval) {
		const char *b, *brk;
		int band, len;
		len = packet_read(in_stream, NULL, NULL, buf, LARGE_PACKET_MAX, 0);
		if (len == 0)
			break;
		if (len < 1) {
			outbuf_len = outbuf.len;
			strbuf_addf(&outbuf,
				    "%s%s: protocol error: no band designator",
				    outbuf_len ? "\n" : "", me);
			retval = SIDEBAND_PROTOCOL_ERROR;
			break;
		}
		band = buf[0] & 0xff;
		buf[len] = '\0';
		len--;
		switch (band) {
		case 3:
			outbuf_len = outbuf.len;
			strbuf_addf(&outbuf, "%s%s%s", outbuf_len ? "\n" : "",
				    PREFIX, buf + 1);
			retval = SIDEBAND_REMOTE_ERROR;
			break;
		case 2:
			b = buf + 1;

			while ((brk = strpbrk(b, "\n\r"))) {
				int linelen = brk - b;
				outbuf_len = outbuf.len;

				if (!outbuf_len)
					strbuf_addf(&outbuf, "%s", PREFIX);
				if (linelen > 0) {
					strbuf_addf(&outbuf, "%.*s%s%c",
						    linelen, b, suffix, *brk);
				} else {
					strbuf_addf(&outbuf, "%c", *brk);
				}
				xwrite(2, outbuf.buf, outbuf.len);
				strbuf_reset(&outbuf);

				b = brk + 1;
			}

			outbuf_len = outbuf.len;
			if (*b)
				strbuf_addf(&outbuf, "%s%s",
					    outbuf_len ? "" : PREFIX, b);
			break;
		case 1:
			write_or_die(out, buf + 1, len);
			break;
		default:
			outbuf_len = outbuf.len;
			strbuf_addf(&outbuf, "%s%s: protocol error: bad band #%d",
				    outbuf_len ? "\n" : "", me, band);
			retval = SIDEBAND_PROTOCOL_ERROR;
			break;
		}
	}

	if (outbuf.len) {
		strbuf_addf(&outbuf, "\n");
		xwrite(2, outbuf.buf, outbuf.len);
	}
	strbuf_release(&outbuf);
	return retval;
}