int init_aclip();
void write_aclip(char *data, size_t len);
char * read_aclip(size_t *len);
int filter_aclip(const SDL_Event *event);
