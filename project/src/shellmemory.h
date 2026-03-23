#define MEM_SIZE 1000
#define LINE_SIZE 100
void mem_init();
const char *mem_get_value(char *var);
void mem_set_value(char *var, char *value);

void code_set_line(char *line, int n);
char *code_get_line(int n);
int load_code_mem(const char *path, int *start, int *end);
void reset_code_mem(int start, int end);
