#!/usr/bin/env python3
"""Draw the object graph of an Archetype universe as GraphViz dot.

Every attribute that holds an object is an edge, and `archetype --inspect`
already puts all of them in the RDF -- so this only restates in dot what the
dump has said.  Which edges are worth drawing depends on the game: Animal
keeps a decision tree in IfYes and IfNo, while Gorreven keeps a map in
location.  Name them with --attr, or pass none and get all of them.

Reads the Turtle that --inspect emits, in the shape it emits it: one subject
per block, one indented "; attr:name value" per line.  It is not a general
Turtle parser and does not try to be.

Nothing outside the Python standard library is needed.  The dot text goes to
standard output, so GraphViz is wanted only if you want a picture of it.
"""

import argparse
import re
import subprocess
import sys

SUBJECT = re.compile(r'^(\S+)\s+a\s+(\S+)\s*\.?\s*$')
ATTRIBUTE = re.compile(r'^\s*;\s*attr:(\S+)\s+(.*?)(\s\.)?$')
REFERENCE = re.compile(r'^(obj:|type:|_:)')

ESCAPES = {'"': '"', '\\': '\\', 'n': '\n', 'r': '\r', 't': '\t'}

# Assigned to attributes in order of first appearance, so a drawing is stable
# for a given dump.  --attr name:colour overrides one.
PALETTE = ['#2f7d4f', '#a5432f', '#4a6fa5', '#8a5fa8', '#b08a3e', '#3f8f8a']


def inspect(archetype, acx):
    """Run the interpreter over a compiled game and return its Turtle."""
    finished = subprocess.run([archetype, '--inspect=' + acx],
                              capture_output=True, text=True)
    if finished.returncode != 0:
        raise SystemExit((finished.stderr or 'inspection failed').rstrip())
    return finished.stdout


def unquote(raw):
    """A Turtle literal to the string it stands for; anything else unchanged."""
    if not raw.startswith('"'):
        return raw
    closed = raw.rfind('"')
    if closed <= 0:
        return raw
    text, out, pending = raw[1:closed], [], False
    for character in text:
        if pending:
            out.append(ESCAPES.get(character, character))
            pending = False
        elif character == '\\':
            pending = True
        else:
            out.append(character)
    return ''.join(out)


def parse(turtle):
    """{subject: {attribute: raw value}}, in the order the dump gave them."""
    objects, subject = {}, None
    for line in turtle.splitlines():
        if not line.strip() or line.startswith(('@', '#')):
            continue
        found = SUBJECT.match(line)
        if found:
            subject = found.group(1)
            objects.setdefault(subject, {})
            if line.rstrip().endswith('.'):
                subject = None
            continue
        if subject is None:
            continue
        attribute = ATTRIBUTE.match(line)
        if attribute:
            objects[subject][attribute.group(1)] = attribute.group(2)
        if line.rstrip().endswith(' .'):
            subject = None
    return objects


def shorten(subject):
    """obj:cave -> cave; a blank node keeps only its number."""
    if subject.startswith('_:object_'):
        return '#' + subject[len('_:object_'):]
    _, _, name = subject.partition(':')
    return name or subject


def wrap(text, width):
    lines, line = [], ''
    for word in text.split():
        if line and len(line) + 1 + len(word) > width:
            lines.append(line)
            line = word
        else:
            line = (line + ' ' + word).strip()
    if line:
        lines.append(line)
    return lines or ['']


def quote(text):
    return '"' + text.replace('\\', '\\\\').replace('"', '\\"') + '"'


def node_id(subject):
    return 'n_' + re.sub(r'\W', '_', subject)


def colours_for(wanted, objects):
    """Fix an edge colour per attribute, honouring any the caller named."""
    chosen, unclaimed, taken = {}, list(PALETTE), 0
    for name, colour in wanted.items():
        if colour:
            chosen[name] = colour
            if colour in unclaimed:
                unclaimed.remove(colour)
    # Naming every colour in the palette would otherwise leave nothing to hand
    # the attributes that were not named.
    unclaimed = unclaimed or list(PALETTE)
    for attributes in objects.values():
        for name, value in attributes.items():
            if name in chosen or not REFERENCE.match(value):
                continue
            if wanted and name not in wanted:
                continue
            chosen[name] = unclaimed[taken % len(unclaimed)]
            taken += 1
    return chosen


def build(objects, wanted, labels, width, title, rankdir):
    colours = colours_for(wanted, objects)

    edges, drawn = [], set()
    for subject, attributes in objects.items():
        for name, value in attributes.items():
            if name not in colours or value not in objects:
                continue
            edges.append((subject, name, value))
            drawn.update((subject, value))

    out = ['digraph universe {',
           '  graph [rankdir=' + rankdir + ', bgcolor="#fbfbfa", '
           'fontname="Helvetica",',
           '         labelloc=t, fontsize=11, nodesep=0.35, ranksep=0.55];',
           '  node  [fontname="Helvetica", fontsize=11];',
           '  edge  [fontname="Helvetica", fontsize=9, arrowsize=0.7];']
    if title:
        out.append('  label=' + quote(title) + ';')
    out.append('')

    for subject in objects:
        # An object earns a node by being at one end of an edge.  A label only
        # decides what a node says, never whether there is one, or asking for
        # a widely-held attribute would fill the drawing with lone boxes.
        if subject not in drawn:
            continue
        attributes = objects[subject]
        described = [attributes[name] for name in labels if name in attributes]
        # A blank node was made while the game ran; a named one was written
        # down in source.  Worth telling apart at a glance -- and worth naming
        # only in the second case, where the name means something to a reader.
        anonymous = subject.startswith('_:')
        name = shorten(subject)
        if not described:
            lines = wrap(name, width)
        elif anonymous:
            lines = wrap(unquote(described[0]), width)
        else:
            lines = wrap(unquote(described[0]), width) + ['(' + name + ')']
        out.append('  {} [label="{}", shape=box, style="rounded,filled", '
                   'fillcolor="{}", color="{}"];'
                   .format(node_id(subject), '\\n'.join(lines),
                           '#f4f4f2' if anonymous else '#e8eef7',
                           '#9a9a94' if anonymous else '#4a6fa5'))

    out.append('')
    for subject, name, value in edges:
        colour = colours[name]
        out.append('  {} -> {} [label=" {}", color="{}", fontcolor="{}"];'
                   .format(node_id(subject), node_id(value), name,
                           colour, colour))

    out.append('}')
    return '\n'.join(out)


def main():
    parser = argparse.ArgumentParser(
        description='Draw an Archetype universe as a GraphViz digraph.',
        epilog='Reads Turtle from standard input when given "-" as the file.')
    parser.add_argument('file', metavar='GAME.acx',
                        help='a compiled game or save file, or "-" for Turtle '
                             'on standard input')
    parser.add_argument('--archetype', default='./build/archetype',
                        metavar='PATH', help='the interpreter (default: %(default)s)')
    parser.add_argument('--attr', action='append', default=[],
                        metavar='NAME[:COLOUR]',
                        help='draw only this attribute, optionally in this '
                             'colour; repeatable')
    parser.add_argument('--label', action='append', default=[], metavar='NAME',
                        help='label nodes with this attribute where they have '
                             'it; repeatable, first match wins')
    parser.add_argument('--width', type=int, default=22, metavar='N',
                        help='wrap labels at N characters (default: %(default)s)')
    parser.add_argument('--title', default='', help='caption for the drawing')
    parser.add_argument('--rankdir', default='TB', choices=['TB', 'LR'],
                        help='lay out top-to-bottom or left-to-right; a wide '
                             'forest reads better as LR (default: %(default)s)')
    parser.add_argument('-o', '--output', metavar='FILE',
                        help='write dot here instead of standard output')
    arguments = parser.parse_args()

    wanted = {}
    for given in arguments.attr:
        name, _, colour = given.partition(':')
        wanted[name] = colour

    turtle = (sys.stdin.read() if arguments.file == '-'
              else inspect(arguments.archetype, arguments.file))
    objects = parse(turtle)
    if not objects:
        raise SystemExit('no objects found -- was that an --inspect dump?')

    dot = build(objects, wanted, arguments.label, arguments.width,
                arguments.title, arguments.rankdir) + '\n'
    if arguments.output:
        with open(arguments.output, 'w') as handle:
            handle.write(dot)
    else:
        sys.stdout.write(dot)


if __name__ == '__main__':
    main()
