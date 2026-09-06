"""Exercise the real C renderer, with ASan/UBSan, against the supplied corpus."""
import os
from pathlib import Path
import random
import re
import subprocess
import sys
from html.parser import HTMLParser

runner = sys.argv[1]
record = Path(sys.argv[2]) if len(sys.argv) > 2 else None
if record:
    record.mkdir(parents=True, exist_ok=True)
    for old in record.glob("case-*.*"):
        old.unlink()
checks = 0
env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
           UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")


class Tags(HTMLParser):
    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.stack = []
        self.tags = []

    def handle_starttag(self, tag, attrs):
        self.tags.append((tag, dict(attrs)))
        assert tag not in {"script", "iframe", "object"}, tag
        assert not any(k.startswith("on") for k, _ in attrs), attrs
        if tag not in {"hr", "br", "img"}:
            self.stack.append(tag)

    def handle_endtag(self, tag):
        assert self.stack and self.stack.pop() == tag, tag


def execute(source, task=None, reject=False):
    global checks
    data = source.encode("utf-8") if isinstance(source, str) else source
    command = [runner] + ([] if task is None else [str(task)])
    proc = subprocess.run(command, input=data, capture_output=True, timeout=8, env=env)
    checks += 1
    expected = (2 if task is None else 3) if reject else 0
    assert proc.returncode == expected, (command, data[:120], proc.returncode, proc.stderr.decode(errors="replace"))
    if record:
        stem = record / f"case-{checks:04d}"
        stem.with_suffix(".in").write_bytes(data)
        stem.with_suffix(".out").write_bytes(proc.stdout)
        stem.with_suffix(".status").write_text(str(expected) + "\n")
        stem.with_suffix(".task").write_text("" if task is None else str(task))
    if reject:
        assert b"Sanitizer" not in proc.stderr and b"runtime error:" not in proc.stderr
        return ""
    result = proc.stdout.decode("utf-8")
    if task is None:
        tags = Tags()
        tags.feed(result)
        assert not tags.stack, tags.stack
        assert result.startswith("<html><body>") and result.endswith("</body></html>")
    return result


def expect(source, *parts, absent=()):
    result = execute(source)
    for part in parts:
        assert part in result, (source, part, result)
    for part in absent:
        assert part not in result, (source, part, result)


expect("#no-space\n\n### title ###\n\n#\tTab", "<p>#no-space</p>", "<h3>title</h3>", "<h1>Tab</h1>")
expect("one\ntwo\n===", "<h1>one\ntwo</h1>")
expect("one\ntwo\n\nthree  \nfour\\\nfive", "<p>one\ntwo</p>", "three<br>four<br>five")
expect("> > inner\n\n> lazy\ncontinuation", "<blockquote><blockquote>", "lazy\ncontinuation")
expect("3. third\n4. fourth", '<ol start="3">', "<li>third</li><li>fourth</li>")
expect("0) zero\n1) one", '<ol start="0">')
expect("- one\n  - two\n    - three", "<ul><li>one<ul><li>two<ul>")
expect("- one\n\n- two", "<li><p>one</p></li>", "<li><p>two</p></li>")
expect("* a\n+ b\n- c", "</ul><ul>")
expect("~~~python\n  a < b\n\n~~~", "<pre>  a &lt; b\n\n</pre>", absent=("~~~",))
expect("```\nnever\n# swallowed", "<pre>never\n# swallowed\n</pre>", absent=("<h1>",))
expect("````\n```\n````", "<pre>```\n</pre>")
expect("    code\n        indent\n\nend", "<pre>code\n    indent\n</pre>", "<p>end</p>")
expect("`` a ` b ``", "<tt>a ` b</tt>")
expect("`  code  `", "<tt> code </tt>")
expect("***both*** **foo*bar*baz** *foo __bar__ baz*", "<i><b>both</b></i>", "<b>foo<i>bar</i>baz</b>", "<i>foo <b>bar</b> baz</i>")
expect("foo_bar_baz a * b * c ** spaced **", "foo_bar_baz a * b * c ** spaced **")
expect("~~has *em*~~", "<strike>has <i>em</i></strike>")
expect("[nested [label]](http://x/foo(bar))", 'href="http://x/foo(bar)"', "nested [label]")
expect('[label](http://x "title")', 'href="http://x" title="title"')
expect("[ref][ID] [id][] [id]\n\n[id]: http://x 'tip'", 'href="http://x" title="tip"', absent=("[id]:",))
expect("<https://example.com> www.example.com a@example.com", 'href="https://example.com"', 'href="http://www.example.com"', 'href="mailto:a@example.com"')
expect("http://example.com/path?a=1&b=2.", 'href="http://example.com/path?a=1&amp;b=2"')
expect('[![**alt**](img.png "tip")](https://x)', '<a href="https://x"><img src="img.png" title="tip" alt="alt"></a>')
expect("![ref][pic]\n\n[pic]: local.png", '<img src="local.png" alt="ref">')
expect("- - -\n\n___\n\n* * *", "<hr><hr><hr>")
expect("<script>alert(1)</script>\n\n<img src=x onerror='x'>", "&lt;script&gt;", "&lt;img", absent=("<script", "<img",))
expect("[x](javascript:alert%281%29) [fake](toggle:0) ![alt](data:bad)", "x", "fake", "[image: alt]", absent=("href=", "src=",))
expect("[x](java&#x09;script:bad)", absent=("href=",))
expect("&amp; &lt; &quot; &#65; &#x41; &#0; &#xFFFFFF; &notarealentity;", "&amp; &lt; &quot; A A \ufffd \ufffd &amp;notarealentity;")
expect("a | b\n:--- | ---:\nx | y", '<table border="1"', '<td align="left"><b>a</b></td>', '<td align="right">y</td>')
expect("| a \\| b | c |\n| --- | --- |\n| x | y | z |", "a | b", "<td align=\"left\">y</td>", absent=(">z<",))
expect("|a|b|\n|-|-|\n|x|", '<td align="left"></td>')
expect("|one\n|---\n|value", '<table border="1"', "value")
expect("- [ ] task\n  - [X] child", 'href="toggle:0"', 'href="toggle:1"', "[x]")
expect("`[ ]`\ntext [x] text\n- `[ ]`", absent=("toggle:",))
expect("$a * b$\n\n$$\nx * y\n$$", "<tt>$a * b$</tt>", "x * y", absent=("<i>",))
expect("```mermaid\ngraph TD; A-->B;\n```", "<pre>graph TD; A--&gt;B;\n</pre>")
expect(b"\xef\xbb\xbf# title\r\n\rbody\rtext", "<h1>title</h1>", "body\ntext")
expect(b"a\x00b", "a\ufffdb")
expect("中文 é مرحبا 𝕳 👨‍👩‍👧‍👦", "中文 é مرحبا 𝕳 👨‍👩‍👧‍👦")

# Offsets must address the exact UTF-8 checkbox byte in original line endings.
for source, line in [("中文\n> - [ ] nested", 1), ("a\r\n\r> - [X] nested", 2),
                     ("\ufeff- [ ] bom", 0), ("```\n- [ ] code\n```\n- [ ] real", 3)]:
    raw = source.encode()
    offset = int(execute(raw, task=line))
    assert raw[offset - 1:offset + 2] in (b"[ ]", b"[X]", b"[x]")
execute("`[ ]`\n- [ ] real", task=0, reject=True)
execute("```\n- [ ] code\n```", task=1, reject=True)

corpus = Path("tests/corpus/markdown-corpus.md").read_text()
whole = execute(corpus)
# Split by numbered section headings intentionally, independently of fence state.
# This keeps the unclosed-fence test from masking later cases.
sections = re.split(r"(?m)(?=^### [1-4]\.\d+ )", corpus)
for section in sections:
    assert execute(section) == execute(section), "nondeterministic output"

expect("x" * 102400, "x" * 102400)
expect("[a](b) " * 10000, '<a href="b">a</a>')
execute("> " * 1000 + "deep", reject=True)
execute("x" * (4 * 1024 * 1024 + 1), reject=True)
expect("[r] " * 1000 + "\n\n" + "\n".join(f"[r{i}]: https://x/{i}" for i in range(2000)), "[r]")
randomizer = random.Random(20260906)
alphabet = "*_~`[]()<>!#|&;:\\ \t\nabc123中文"
for _ in range(100):
    execute("".join(randomizer.choice(alphabet) for _ in range(randomizer.randrange(0, 3000))))

print(f"PASS {checks} render/task executions; {len(sections)} corpus sections; ASan/UBSan clean")
