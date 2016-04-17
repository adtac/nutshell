struct dictionary {
	char variable[MAX_WORD_LEN], translation[MAX_LINE];
	struct dictionary *next;
};
