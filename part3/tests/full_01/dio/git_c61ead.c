int recv_sideband(const char *me, int in_stream, int out)
{
	const char *term, *suffix;
	char buf[LARGE_PACKET_MAX + 1];
	struct strbuf outbuf = STRBUF_INIT;
	int retval = 0;

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
			strbuf_addf(&outbuf,
				    "%s%s: protocol error: no band designator",
				    outbuf.len ? "\n" : "", me);
			retval = SIDEBAND_PROTOCOL_ERROR;
			break;
		}
		band = buf[0] & 0xff;
		buf[len] = '\0';
		len--;
		switch (band) {
		case 3:
			strbuf_addf(&outbuf, "%s%s%s", outbuf.len ? "\n" : "",
				    PREFIX, buf + 1);
			retval = SIDEBAND_REMOTE_ERROR;
			break;
		case 2:
			b = buf + 1;

			while ((brk = strpbrk(b, "\n\r"))) {
				int linelen = brk - b;

				if (!outbuf.len)
					strbuf_add(&outbuf, PREFIX, strlen(PREFIX));
				if (linelen > 0) {
					strbuf_add(&outbuf, b, linelen);
					strbuf_add(&outbuf, suffix, strlen(suffix));
					strbuf_addch(&outbuf, *brk);
				} else {
					strbuf_addch(&outbuf, *brk);
				}
				xwrite(2, outbuf.buf, outbuf.len);
				strbuf_reset(&outbuf);

				b = brk + 1;
			}

			if (*b) {
				if (!outbuf.len)
					strbuf_add(&outbuf, PREFIX, strlen(PREFIX));
				strbuf_add(&outbuf, b, strlen(b));
			}
			break;
		case 1:
			write_or_die(out, buf + 1, len);
			break;
		default:
			strbuf_addf(&outbuf, "%s%s: protocol error: bad band #%d",
				    outbuf.len ? "\n" : "", me, band);
			retval = SIDEBAND_PROTOCOL_ERROR;
			break;
		}
	}

	if (outbuf.len) {
		strbuf_addch(&outbuf, '\n');
		xwrite(2, outbuf.buf, outbuf.len);
	}
	strbuf_release(&outbuf);
	return retval;
}