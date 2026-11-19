#!/usr/bin/env python3
"""Checks the documentation against the code (docs/README.md, "Rules for documentation").

For every document under docs/ (and the front page's links):
  * each relative link resolves;
  * each file path cited in `code` exists in the repository (build outputs, runtime paths on the
    console and names listed in allow.txt are exempt);
  * each symbol cited in `code` (an identifier with an underscore or a lowerUpper join) exists in
    the code;
  * reference and guide documents cite no `file:line` (line numbers go stale; cite the symbol);
  * every document starts with the "Type · Audience · Status" / "Verified against" header;
  * a guide and its translation (X.md, X.<lang>.md) have the same headings count and the same
    commands and paths (their code spans without spaces).

Errors in docs/reference/, docs/guides/ and docs/README.md fail the check (--strict); findings
elsewhere (plans, reports, proposals, history) are only reported.

Usage: check_docs.py [--strict] [--report FILE]
"""

import argparse
import fnmatch
import os
import posixpath
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, '..', '..'))

SKIP_DIRS = {'.git', 'build-vita', 'voxelgen', 'node_modules', '.trail', 'TEST UI', 'imagenes', 'logs'}
CODE_EXTS = ('.c', '.h', '.py', '.ps1', '.sh', '.txt', '.mk', '.s', '.inc', '.json', '.html', '.js',
             '.cmake', '.yaml')
PATH_EXTS = ('c', 'h', 'py', 'ps1', 'sh', 'md', 'yaml', 'bin', 'txt', 's', 'inc', 'json', 'png', 'pak',
             'cfg', 'csv', 'log', 'sav', 'mk', 'html', 'js', 'rgba5551')
STRICT_PREFIXES = ('docs/reference/', 'docs/guides/')
STRICT_FILES = ('docs/README.md',)
LINE_REF = re.compile(r'^[\w./-]+\.\w+:L?\d+')


def rel(p):
    return os.path.relpath(p, ROOT).replace(os.sep, '/')


def load_allow():
    names = []
    path = os.path.join(HERE, 'allow.txt')
    if os.path.exists(path):
        for line in open(path, encoding='utf-8'):
            line = line.split('#', 1)[0].strip()
            if line:
                names.append(line)
    return names


def scan_repo():
    paths, idents = set(), set()
    for dp, dns, fns in os.walk(ROOT):
        dns[:] = [d for d in dns if d not in SKIP_DIRS]
        for fn in fns:
            full = os.path.join(dp, fn)
            r = rel(full)
            paths.add(r)
            if fn.endswith(CODE_EXTS) or fn == 'CMakeLists.txt':
                try:
                    idents.update(re.findall(r'[A-Za-z_][A-Za-z0-9_]*', open(full, encoding='utf-8', errors='replace').read()))
                except OSError:
                    pass
        for d in dns:
            paths.add(rel(os.path.join(dp, d)))
    return paths, idents


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--strict', action='store_true', help='exit 1 on an error in reference or guides')
    ap.add_argument('--report', default=os.path.join(ROOT, 'build-vita', 'doc_check_report.txt'))
    args = ap.parse_args()

    allow = load_allow()
    paths, idents = scan_repo()
    basenames = {}
    for p in paths:
        basenames.setdefault(posixpath.basename(p), []).append(p)

    def allowed(name):
        return any(fnmatch.fnmatchcase(name, a) for a in allow)

    docs = sorted(p for p in paths if p.endswith('.md') and (p.startswith('docs/') or '/' not in p))
    errors, warnings = [], []

    def finding(doc, msg):
        strict = doc.startswith(STRICT_PREFIXES) or doc in STRICT_FILES
        (errors if strict else warnings).append(f'{doc}: {msg}')

    for doc in docs:
        text = open(os.path.join(ROOT, doc), encoding='utf-8', errors='replace').read()
        in_docs = doc.startswith('docs/')
        history = doc.startswith('docs/history/')
        # header
        if in_docs and not history:
            head = text.split('\n', 6)
            if not any(l.startswith('Type:') or l.startswith('**Type**') for l in head[:5]) \
                    or 'Verified against' not in '\n'.join(head[:6]):
                finding(doc, 'missing the "Type · Audience · Status" / "Verified against" header')
        if history:
            continue
        # links
        for target in re.findall(r'\]\(([^)\s#]+)', text):
            if re.match(r'^[a-z]+:', target) or target.startswith('/') or target.startswith('../../issues'):
                continue
            resolved = posixpath.normpath(posixpath.join(posixpath.dirname(doc), target))
            if resolved not in paths:
                finding(doc, f'broken link: {target}')
        if not in_docs:
            continue
        # code spans
        for span in set(re.findall(r'`([^`\n]{2,160})`', text)):
            s = span.strip()
            if allowed(s):
                continue
            if LINE_REF.match(s) and doc.startswith(STRICT_PREFIXES):
                finding(doc, f'line reference (cite the symbol instead): {s}')
                continue
            m = re.match(r'^(\.?/)?([\w.\-/]+\.(%s))(:L?\d+.*)?$' % '|'.join(PATH_EXTS), s)
            if m:
                p = m.group(2)
                if ':' in s.split('.', 1)[0] or p.startswith('build-vita/') or allowed(p):
                    continue
                if p in paths or any(x.endswith('/' + p) or x == p for x in basenames.get(posixpath.basename(p), [])):
                    continue
                finding(doc, f'file not found: {p}')
                continue
            m = re.match(r'^([A-Za-z_][A-Za-z0-9_]{3,})(\(\))?$', s)
            if m and ('_' in m.group(1) or re.search(r'[a-z][A-Z]', m.group(1))):
                if m.group(1) not in idents and not allowed(m.group(1)):
                    finding(doc, f'symbol not found in the code: {m.group(1)}')

    # translations: X.md and X.<lang>.md
    for doc in docs:
        m = re.match(r'^(.*)\.([a-z]{2})\.md$', doc)
        if not m or m.group(1) + '.md' not in paths:
            continue
        a = open(os.path.join(ROOT, m.group(1) + '.md'), encoding='utf-8').read()
        b = open(os.path.join(ROOT, doc), encoding='utf-8').read()
        spans_a = {s for s in re.findall(r'`([^`\n]+)`', a) if ' ' not in s}
        spans_b = {s for s in re.findall(r'`([^`\n]+)`', b) if ' ' not in s}
        heads_a = len(re.findall(r'^#', a, re.M))
        heads_b = len(re.findall(r'^#', b, re.M))
        if heads_a != heads_b:
            finding(doc, f'{heads_b} headings against {heads_a} in {m.group(1)}.md: the translation is out of step')
        for s in sorted(spans_a ^ spans_b):
            where = doc if s in spans_b else m.group(1) + '.md'
            finding(doc, f'command or path only in {posixpath.basename(where)}: {s}')

    lines = [f'Documentation check: {len(errors)} errors, {len(warnings)} warnings', '']
    lines += ['ERRORS (reference, guides, index):'] + [f'  {e}' for e in errors] + ['']
    lines += ['WARNINGS (plans, reports, proposals, front page):'] + [f'  {w}' for w in warnings]
    report = '\n'.join(lines) + '\n'
    os.makedirs(os.path.dirname(args.report), exist_ok=True)
    with open(args.report, 'w', encoding='utf-8') as f:
        f.write(report)
    print(f'Documentation check: {len(errors)} errors, {len(warnings)} warnings (report: {args.report})')
    for e in errors:
        print('  error:', e)
    if args.strict and errors:
        sys.exit(1)


if __name__ == '__main__':
    main()
