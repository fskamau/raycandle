/*
 * Allows specifying figure axes using a string
 * create_figure("a b") would create figure with 2 vertical axes etc.
 * Fas.skel contains each axes relative {start_col,start_row,cols,rows}
 * rows are separated by any amount of spaces
 * Max number of axes is FAS_MAX_AXES
 */

typedef struct {
  unsigned int len, rows, cols, *skel;
  char *labels;
} Fas; // figure axes skeleton

Fas fas_parse(char *outline);
void fas_print(Fas fas);
void fas_destroy(Fas fas);

