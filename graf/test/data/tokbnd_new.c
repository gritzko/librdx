static int get_hash(const char *hex, unsigned char *hash,
		    const struct git_hash_algo *algop)
{
	for (size_t i = 0; i < algop->rawsz; i++) {
		int val = hex2chr(hex);
		if (val < 0)
			return -1;
		*hash++ = val;
		hex += 2;
	}
	return 0;
}
