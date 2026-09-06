#include "MdParser.h"
#include <stdio.h>

static int failures = 0;
static void check(bool ok, const char *name)
{
    printf("%s %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok) ++failures;
}

int main()
{
    QString html = MdParser::toRichText("```cpp\n  a < b && c;\n\n  next();\n```\n# Heading", 0);
    check(html.find("<pre>  a &lt; b &amp;&amp; c;\n\n  next();\n</pre>") >= 0,
          "fenced code keeps indentation and blank lines");
    check(html.find("```") < 0 && html.find("<h1>Heading</h1>") >= 0,
          "fence is hidden and following Markdown renders");
    html = MdParser::toRichText("  ~~~~lang\r\n  **literal**\r\n  ~~~\r\n  ~~~~\r\n", 0);
    check(html.find("<pre>**literal**\n~~~\n</pre>") >= 0,
          "CRLF tilde fence strips opening indent and requires matching length");
    html = MdParser::toRichText("```\n~~~\n```not a closer\n- [ ] literal", 0);
    check(html.find("<pre>~~~\n```not a closer\n- [ ] literal\n</pre>") >= 0
          && html.find("toggle:") < 0, "unclosed fence remains literal and closes HTML");
    html = MdParser::toRichText("``a ` b`` and `x < y` and \\*plain\\*", 0);
    check(html.find("<tt>a ` b</tt>") >= 0 && html.find("<tt>x &lt; y</tt>") >= 0
          && html.find("*plain*") >= 0, "inline code delimiters and escaped punctuation");
    html = MdParser::toRichText("`[ ]`\ntext [x] text\n- `[ ]`", 0);
    check(html.find("toggle:") < 0 && html.find("<tt>[ ]</tt>") >= 0,
          "checkbox text inside inline code is not a task");
    html = MdParser::toRichText("[name](a\"b)", 0);
    check(html.find("href=\"a&quot;b\"") >= 0, "attribute quotes are escaped");
    html = MdParser::toRichText("    a < b\n\n\t**literal**\nend", 0);
    check(html.find("<pre>a &lt; b\n\n**literal**\n</pre><p>end</p>") >= 0,
          "indented code preserves literal content and blank lines");
    QValueList<MdBlockMap> map;
    MdParser::toRichText("old\nmap", &map);
    MdParser::toRichText("new", &map);
    check(map.count() == 1, "block map is cleared between renders");
    QString md = "```\n- [ ] code\n```\n- [ ] task";
    html = MdParser::toRichText(md, 0);
    check(html.find("toggle:3") >= 0 && html.find("toggle:1") < 0,
          "task links retain source line numbers after code");
    check(MdParser::toggleTaskLine(&md, 3) && md.right(10) == "- [x] task",
          "task toggle updates the source line");
    return failures ? 1 : 0;
}
