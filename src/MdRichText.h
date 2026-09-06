#ifndef MD_RICH_TEXT_H
#define MD_RICH_TEXT_H

#ifdef __cplusplus
extern "C" {
#endif

/* UTF-8 input/output. Caller frees *html with free(). Zero means success. */
int md_rich_text(const char *source, unsigned size, char **html);
/* Finds a parsed task, not an arbitrary checkbox-looking substring. */
int md_task_offset(const char *source, unsigned size, unsigned line, unsigned *offset);

#ifdef __cplusplus
}
#endif
#endif
