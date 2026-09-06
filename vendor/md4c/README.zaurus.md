MD4C 0.5.3, upstream commit 093c3f45ce44bd6661849982b1dd7f0e7f385621.
Source: https://github.com/mity/md4c/tree/release-0.5.3
License: MIT (see LICENSE.md).

Imported md4c.c/.h and entity.c/.h. No runtime shared library is required.
Three compatibility changes support GCC 2.95:

- Use a one-element trailing array instead of a C99 flexible array. Existing
  sizeof-plus-capacity allocations retain at least the required storage.
- Store the dummy-mark pointer separately instead of in an anonymous union.
  This increases internal mark size but preserves its independent uses.
- Use abort for violated internal invariants on old GCC, which has no
  __builtin_unreachable intrinsic.

Qt2 HTML adaptation and application-specific task links live in
src/MdRichText.c, outside the vendored parser.
