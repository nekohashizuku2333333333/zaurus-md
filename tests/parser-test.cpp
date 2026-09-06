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
    html = MdParser::toRichText("#no-space\n\nname\n===", 0);
    check(html.find("<p>#no-space</p>") >= 0 && html.find("<h1>name</h1>") >= 0,
          "CommonMark heading boundaries and Setext");
    html = MdParser::toRichText("a | b\n:--- | ---:\nx | y", 0);
    check(html.find("<table") >= 0 && html.find("<td align=\"right\">y</td>") >= 0,
          "GFM table alignment uses Qt2 attributes");
    html = MdParser::toRichText("3) third\n   - child\n4) fourth", 0);
    check(html.find("<ol start=\"3\">") >= 0 && html.find("<ul><li>child</li></ul>") >= 0,
          "nested lists preserve ordered start");
    html = MdParser::toRichText("[label][REF]\n\n[ref]: https://x \"tip\"", 0);
    check(html.find("<a href=\"https://x\" title=\"tip\">label</a>") >= 0,
          "reference links resolve without rendering definitions");
    html = MdParser::toRichText("[fake](toggle:0) <script>x</script>", 0);
    check(html.find("href=") < 0 && html.find("<script>") < 0,
          "source text cannot synthesize application commands or raw HTML");
    QString unicode = QString::fromUtf8("\344\270\255\346\226\207");
    check(MdParser::toRichText(unicode, 0) == "<html><body><p>" + unicode + "</p></body></html>",
          "Qt2 UTF8 conversion has no trailing bytes");
    QString nul = "before";
    nul += QChar(0);
    nul += "after";
    html = MdParser::toRichText(nul, 0);
    check(html.find("after") >= 0 && html.find(QChar(0xfffd)) >= 0,
          "embedded NUL does not truncate Qt2 input");
    md = "line\r\n> - [ ] nested\r\n";
    check(MdParser::toggleTaskLine(&md, 1) && md == "line\r\n> - [x] nested\r\n",
          "nested task toggles preserve CRLF source");
    md = "```\n- [ ] code\n```";
    check(!MdParser::toggleTaskLine(&md, 1), "task toggle rejects fenced code");
    return failures ? 1 : 0;
}
