## Card

<!-- For example [C3] contacts and charge impact, plus Closes #<issue> -->

## What was done

## How it was verified

## Open issues, or conflicts with the spec

## Checklist

- [ ] Builds with no warnings (`-Wall -Wextra -Wpedantic` on GCC/Clang, `/W4` on MSVC), code formatted with clang-format
- [ ] ctest all passing, every new piece of logic has tests
- [ ] When rules change: the "Rule spec" in docs/DEVELOPMENT.md is updated and the prototype comparison tests are still within tolerance
- [ ] When data formats change: data/schema/ and the "Data formats" chapter are updated
- [ ] In-game text goes only through data/strings/, with Chinese in Traditional characters
- [ ] Code, docs, and this description are in English
