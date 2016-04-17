bool is_quote(char ch) {
	return ch == '\'' || ch == '\"';
}

bool is_whitespace(char ch) {
	return ch == ' ' || ch == '\t';
}

bool is_special(char ch) {
	return ch == '=' || ch == ';';
}

bool is_end(char ch) {
	return ch == EOF || ch == '\0';
}

/* 
 * Return true if the line is either empty or is a comment 
 */
bool is_empty_line(char *line) {
	while(*line && is_whitespace(*line))
		line++;
	return *line == '#' || *line == '\0';
}

int is_keyword(char *word) {
	int count = sizeof(keyword_list)/MAX_WORD_LEN;
	for(int i = 0; i < count; i++) {
		if(strcmp(word, keyword_list[i]) == 0)
			return keyword_list_symbol[i];
	}
	return LINE_EXEC;
}