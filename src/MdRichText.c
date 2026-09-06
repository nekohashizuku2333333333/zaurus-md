#include "MdRichText.h"
#include "md4c.h"
#include "entity.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#define INPUT_LIMIT (4u * 1024u * 1024u)
#define OUTPUT_LIMIT (8u * 1024u * 1024u)
#define DEPTH_LIMIT 64
#define PARSER_FLAGS (MD_DIALECT_GITHUB | MD_FLAG_LATEXMATHSPANS)

typedef struct {
    char *data;
    unsigned size, capacity;
    int error;
} Buffer;

typedef struct {
    Buffer out;
    const char *source;
    unsigned scan, line;
    int depth, images, image_tag;
    int links[DEPTH_LIMIT], link_count;
} Renderer;

static void append(Buffer *b, const char *text, unsigned size)
{
    unsigned capacity;
    char *data;
    if (b->error) return;
    if (size > OUTPUT_LIMIT - b->size) { b->error = -2; return; }
    if (b->size + size + 1 > b->capacity) {
        capacity = b->capacity ? b->capacity : 1024;
        while (capacity < b->size + size + 1) capacity *= 2;
        data = (char *)realloc(b->data, capacity);
        if (!data) { b->error = -1; return; }
        b->data = data;
        b->capacity = capacity;
    }
    if (size) memcpy(b->data + b->size, text, size);
    b->size += size;
    b->data[b->size] = 0;
}

#define LIT(b, s) append((b), (s), sizeof(s) - 1)

static void escaped(Buffer *b, const char *s, unsigned size)
{
    unsigned i, start = 0;
    for (i = 0; i < size; ++i) {
        if (s[i] != '&' && s[i] != '<' && s[i] != '>' && s[i] != '"' && s[i]) continue;
        append(b, s + start, i - start);
        switch (s[i]) {
        case '&': LIT(b, "&amp;"); break;
        case '<': LIT(b, "&lt;"); break;
        case '>': LIT(b, "&gt;"); break;
        case '"': LIT(b, "&quot;"); break;
        default: LIT(b, "\357\277\275"); break;
        }
        start = i + 1;
    }
    append(b, s + start, size - start);
}

static void codepoint(Buffer *b, unsigned cp)
{
    char bytes[4];
    unsigned n;
    if (!cp || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff)) cp = 0xfffd;
    if (cp < 0x80) { bytes[0] = (char)cp; n = 1; }
    else if (cp < 0x800) {
        bytes[0] = (char)(0xc0 | (cp >> 6)); bytes[1] = (char)(0x80 | (cp & 63)); n = 2;
    } else if (cp < 0x10000) {
        bytes[0] = (char)(0xe0 | (cp >> 12)); bytes[1] = (char)(0x80 | ((cp >> 6) & 63));
        bytes[2] = (char)(0x80 | (cp & 63)); n = 3;
    } else {
        bytes[0] = (char)(0xf0 | (cp >> 18)); bytes[1] = (char)(0x80 | ((cp >> 12) & 63));
        bytes[2] = (char)(0x80 | ((cp >> 6) & 63)); bytes[3] = (char)(0x80 | (cp & 63)); n = 4;
    }
    append(b, bytes, n);
}

static void entity(Buffer *b, const char *s, unsigned size)
{
    const ENTITY *named;
    if (size > 3 && s[1] == '#') {
        unsigned i = 2, base = 10, cp = 0;
        if (s[i] == 'x' || s[i] == 'X') { base = 16; ++i; }
        for (; i + 1 < size; ++i) {
            unsigned c = (unsigned char)s[i];
            unsigned digit = c >= '0' && c <= '9' ? c - '0' : (c | 32) - 'a' + 10;
            if (digit >= base || cp > 0x10ffff / base) { cp = 0xfffd; break; }
            cp = cp * base + digit;
        }
        codepoint(b, cp);
    } else if ((named = entity_lookup(s, size)) != NULL) {
        codepoint(b, named->codepoints[0]);
        if (named->codepoints[1]) codepoint(b, named->codepoints[1]);
    } else append(b, s, size);
}

static void attribute(Buffer *b, const MD_ATTRIBUTE *a)
{
    unsigned i;
    for (i = 0; a->size && a->substr_offsets[i] < a->size; ++i) {
        unsigned start = a->substr_offsets[i], size = a->substr_offsets[i + 1] - start;
        if (a->substr_types[i] == MD_TEXT_ENTITY) entity(b, a->text + start, size);
        else if (a->substr_types[i] == MD_TEXT_NULLCHAR) codepoint(b, 0);
        else append(b, a->text + start, size);
    }
}

/* Only generated task links may use the application's edit-command scheme. */
static int safe_url(const Buffer *b)
{
    unsigned i;
    char scheme[16];
    for (i = 0; i < b->size; ++i) {
        unsigned char ch = (unsigned char)b->data[i];
        if (ch < 32 || ch == 127) return 0;
        if (ch == '/' || ch == '#' || ch == '?') return 1;
        if (ch == ':') {
            if (i >= sizeof(scheme)) return 0;
            scheme[i] = 0;
            return !strcmp(scheme, "http") || !strcmp(scheme, "https")
                || !strcmp(scheme, "mailto") || !strcmp(scheme, "ftp")
                || !strcmp(scheme, "file");
        }
        if (i < sizeof(scheme)) scheme[i] = (char)tolower(ch);
    }
    return 1;
}

static unsigned source_line(const char *s, unsigned *scan, unsigned *line, unsigned offset)
{
    while (*scan < offset) {
        if (s[*scan] == '\r') {
            ++*line;
            if (*scan + 1 < offset && s[*scan + 1] == '\n') ++*scan;
        } else if (s[*scan] == '\n') ++*line;
        ++*scan;
    }
    return *line;
}

static int enter_block(MD_BLOCKTYPE type, void *detail, void *data)
{
    Renderer *r = (Renderer *)data;
    Buffer *b = &r->out;
    char tag[80];
    if (++r->depth > DEPTH_LIMIT) return -2;
    switch (type) {
    case MD_BLOCK_DOC: LIT(b, "<html><body>"); break;
    case MD_BLOCK_QUOTE: LIT(b, "<blockquote>"); break;
    case MD_BLOCK_UL: LIT(b, "<ul>"); break;
    case MD_BLOCK_OL:
        sprintf(tag, "<ol start=\"%u\">", ((MD_BLOCK_OL_DETAIL *)detail)->start);
        append(b, tag, strlen(tag)); break;
    case MD_BLOCK_LI: {
        MD_BLOCK_LI_DETAIL *li = (MD_BLOCK_LI_DETAIL *)detail;
        LIT(b, "<li>");
        if (li->is_task) {
            unsigned line = source_line(r->source, &r->scan, &r->line, li->task_mark_offset);
            sprintf(tag, "<a href=\"toggle:%u\">[%c]</a> ", line, li->task_mark == ' ' ? ' ' : 'x');
            append(b, tag, strlen(tag));
        }
        break;
    }
    case MD_BLOCK_HR: LIT(b, "<hr>"); break;
    case MD_BLOCK_H:
        sprintf(tag, "<h%u>", ((MD_BLOCK_H_DETAIL *)detail)->level);
        append(b, tag, strlen(tag)); break;
    case MD_BLOCK_CODE: LIT(b, "<pre>"); break;
    case MD_BLOCK_HTML: LIT(b, "<pre>"); break;
    case MD_BLOCK_P: LIT(b, "<p>"); break;
    case MD_BLOCK_TABLE: LIT(b, "<table border=\"1\" cellspacing=\"0\" cellpadding=\"3\" width=\"100%\">"); break;
    case MD_BLOCK_TR: LIT(b, "<tr>"); break;
    case MD_BLOCK_TH: case MD_BLOCK_TD: {
        MD_ALIGN align = ((MD_BLOCK_TD_DETAIL *)detail)->align;
        sprintf(tag, "<td align=\"%s\">", align == MD_ALIGN_CENTER ? "center" : align == MD_ALIGN_RIGHT ? "right" : "left");
        append(b, tag, strlen(tag));
        if (type == MD_BLOCK_TH) LIT(b, "<b>");
        break;
    }
    default: break; /* Qt2 has no thead/tbody layout objects. */
    }
    return b->error;
}

static int leave_block(MD_BLOCKTYPE type, void *detail, void *data)
{
    Renderer *r = (Renderer *)data;
    Buffer *b = &r->out;
    char tag[16];
    --r->depth;
    switch (type) {
    case MD_BLOCK_DOC: LIT(b, "</body></html>"); break;
    case MD_BLOCK_QUOTE: LIT(b, "</blockquote>"); break;
    case MD_BLOCK_UL: LIT(b, "</ul>"); break;
    case MD_BLOCK_OL: LIT(b, "</ol>"); break;
    case MD_BLOCK_LI: LIT(b, "</li>"); break;
    case MD_BLOCK_H:
        sprintf(tag, "</h%u>", ((MD_BLOCK_H_DETAIL *)detail)->level);
        append(b, tag, strlen(tag)); break;
    case MD_BLOCK_CODE: case MD_BLOCK_HTML: LIT(b, "</pre>"); break;
    case MD_BLOCK_P: LIT(b, "</p>"); break;
    case MD_BLOCK_TABLE: LIT(b, "</table>"); break;
    case MD_BLOCK_TR: LIT(b, "</tr>"); break;
    case MD_BLOCK_TH: LIT(b, "</b></td>"); break;
    case MD_BLOCK_TD: LIT(b, "</td>"); break;
    default: break;
    }
    return b->error;
}

static int enter_span(MD_SPANTYPE type, void *detail, void *data)
{
    Renderer *r = (Renderer *)data;
    Buffer *b = &r->out;
    if (++r->depth > DEPTH_LIMIT) return -2;
    if (r->images) { if (type == MD_SPAN_IMG) ++r->images; return 0; }
    switch (type) {
    case MD_SPAN_EM: LIT(b, "<i>"); break;
    case MD_SPAN_STRONG: LIT(b, "<b>"); break;
    case MD_SPAN_CODE: LIT(b, "<tt>"); break;
    case MD_SPAN_DEL: LIT(b, "<strike>"); break;
    case MD_SPAN_LATEXMATH: LIT(b, "<tt>$"); break;
    case MD_SPAN_LATEXMATH_DISPLAY: LIT(b, "<tt>$$"); break;
    case MD_SPAN_A: case MD_SPAN_IMG: {
        Buffer url = {0, 0, 0, 0}, title = {0, 0, 0, 0};
        int image = type == MD_SPAN_IMG, safe;
        MD_ATTRIBUTE *dest = image ? &((MD_SPAN_IMG_DETAIL *)detail)->src : &((MD_SPAN_A_DETAIL *)detail)->href;
        MD_ATTRIBUTE *tip = image ? &((MD_SPAN_IMG_DETAIL *)detail)->title : &((MD_SPAN_A_DETAIL *)detail)->title;
        attribute(&url, dest);
        attribute(&title, tip);
        safe = !url.error && !title.error && safe_url(&url);
        if (url.error || title.error) b->error = -1;
        if (image) {
            r->images = 1;
            r->image_tag = safe && url.size;
            if (r->image_tag) LIT(b, "<img src=\"");
            else LIT(b, "[image: ");
            safe = r->image_tag;
        } else {
            r->links[r->link_count++] = safe;
            if (safe) LIT(b, "<a href=\"");
        }
        if (safe) {
            if (url.size) escaped(b, url.data, url.size);
            LIT(b, "\"");
            if (title.size) { LIT(b, " title=\""); escaped(b, title.data, title.size); LIT(b, "\""); }
            if (image) LIT(b, " alt=\"");
            else LIT(b, ">");
        }
        free(url.data); free(title.data);
        break;
    }
    default: break;
    }
    return b->error;
}

static int leave_span(MD_SPANTYPE type, void *detail, void *data)
{
    Renderer *r = (Renderer *)data;
    Buffer *b = &r->out;
    (void)detail;
    --r->depth;
    if (r->images) {
        if (type == MD_SPAN_IMG && --r->images == 0) {
            if (r->image_tag) LIT(b, "\">"); else LIT(b, "]");
        }
        return b->error;
    }
    switch (type) {
    case MD_SPAN_EM: LIT(b, "</i>"); break;
    case MD_SPAN_STRONG: LIT(b, "</b>"); break;
    case MD_SPAN_CODE: LIT(b, "</tt>"); break;
    case MD_SPAN_DEL: LIT(b, "</strike>"); break;
    case MD_SPAN_A: if (r->links[--r->link_count]) LIT(b, "</a>"); break;
    case MD_SPAN_LATEXMATH: LIT(b, "$</tt>"); break;
    case MD_SPAN_LATEXMATH_DISPLAY: LIT(b, "$$</tt>"); break;
    default: break;
    }
    return b->error;
}

static int text_callback(MD_TEXTTYPE type, const char *s, MD_SIZE size, void *data)
{
    Renderer *r = (Renderer *)data;
    Buffer *b = &r->out;
    if (type == MD_TEXT_BR) {
        if (r->images) LIT(b, " "); else LIT(b, "<br>");
    } else if (type == MD_TEXT_SOFTBR) {
        LIT(b, "\n");
    } else if (type == MD_TEXT_ENTITY) {
        Buffer decoded = {0, 0, 0, 0};
        entity(&decoded, s, size);
        if (decoded.error) b->error = decoded.error;
        else if (decoded.size) escaped(b, decoded.data, decoded.size);
        free(decoded.data);
    } else if (type == MD_TEXT_NULLCHAR) codepoint(b, 0);
    else escaped(b, s, size);
    return b->error;
}

int md_rich_text(const char *source, unsigned size, char **html)
{
    Renderer r;
    MD_PARSER parser;
    int result;
    *html = NULL;
    if (size > INPUT_LIMIT) return -2;
    if (size >= 3 && !memcmp(source, "\357\273\277", 3)) { source += 3; size -= 3; }
    memset(&r, 0, sizeof(r));
    memset(&parser, 0, sizeof(parser));
    r.source = source;
    parser.flags = PARSER_FLAGS;
    parser.enter_block = enter_block; parser.leave_block = leave_block;
    parser.enter_span = enter_span; parser.leave_span = leave_span;
    parser.text = text_callback;
    result = md_parse(source, size, &parser, &r);
    if (result || r.out.error) { free(r.out.data); return result ? result : r.out.error; }
    *html = r.out.data;
    return 0;
}

typedef struct {
    const char *source;
    unsigned scan, line, target, offset;
    int found;
} TaskSearch;

static int task_block(MD_BLOCKTYPE type, void *detail, void *data)
{
    TaskSearch *s = (TaskSearch *)data;
    if (type == MD_BLOCK_LI) {
        MD_BLOCK_LI_DETAIL *li = (MD_BLOCK_LI_DETAIL *)detail;
        if (li->is_task && source_line(s->source, &s->scan, &s->line, li->task_mark_offset) == s->target) {
            s->offset = li->task_mark_offset;
            s->found = 1;
            return 1;
        }
    }
    return 0;
}

static int skip_block(MD_BLOCKTYPE type, void *detail, void *data)
{ (void)type; (void)detail; (void)data; return 0; }
static int skip_span(MD_SPANTYPE type, void *detail, void *data)
{ (void)type; (void)detail; (void)data; return 0; }
static int skip_text(MD_TEXTTYPE type, const char *s, MD_SIZE size, void *data)
{ (void)type; (void)s; (void)size; (void)data; return 0; }

int md_task_offset(const char *source, unsigned size, unsigned line, unsigned *offset)
{
    TaskSearch s;
    MD_PARSER parser;
    unsigned bom = 0;
    if (size > INPUT_LIMIT) return 0;
    if (size >= 3 && !memcmp(source, "\357\273\277", 3)) { source += 3; size -= 3; bom = 3; }
    memset(&s, 0, sizeof(s)); memset(&parser, 0, sizeof(parser));
    s.source = source; s.target = line;
    parser.flags = PARSER_FLAGS;
    parser.enter_block = task_block;
    parser.leave_block = skip_block;
    parser.enter_span = skip_span; parser.leave_span = skip_span;
    parser.text = skip_text;
    md_parse(source, size, &parser, &s);
    if (s.found) *offset = s.offset + bom;
    return s.found;
}
