"""Hold the INI Master metadata up against the code that reads the ini.

mod/data/StackMaster.inimeta is compiled into the plugin as the INIMETA
resource, and INI Master shows people what it says: the type of each key, its
default, its range and its help. None of that is read from the code, so a new
key, a changed default or a new clamp has to be copied across by hand, and a
copy that is wrong does not fail anywhere. This is the check that it was
copied.

The code is the authority. For every key StackMaster.ini holds:

  - WriteIni() in settings.cpp writes it, so it must be in the metadata, and
    every key in the metadata must be one ReadIni() recognises.
  - The type follows the reader: `atoi(val) != 0` is bool, a plain `atoi` is
    int.
  - The default is the matching member's initialiser in settings.h.
  - min and max are the bounds Clamp() enforces, since those are what the
    plugin actually accepts.
  - The top-level "live" flag has to agree with whether the code ever rereads
    the ini while the game runs. Stack Master's Load() runs once at startup
    and nothing calls an IniWatcher, so editing the ini only takes effect on
    the next launch; the metadata says so with "live": false on the mod and
    on every key.

Exits non-zero on any mismatch, so it can gate a build.

    py -3 scripts/check_inimeta.py
"""

import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..")
META = os.path.join(ROOT, "mod", "data", "StackMaster.inimeta")
SETTINGS_CPP = os.path.join(ROOT, "mod", "src", "core", "settings.cpp")
SETTINGS_H = os.path.join(ROOT, "mod", "src", "core", "settings.h")
SRC_DIR = os.path.join(ROOT, "mod", "src")


def read(p):
    with open(p, encoding="utf-8") as f:
        return f.read()


def body(src, head):
    """The brace-balanced body of the first function whose signature starts with head."""
    i = src.index(head)
    i = src.index("{", i)
    depth = 0
    for j in range(i, len(src)):
        if src[j] == "{":
            depth += 1
        elif src[j] == "}":
            depth -= 1
            if depth == 0:
                return src[i:j + 1]
    raise ValueError(head)


def parse_written(cpp):
    """The ini keys WriteIni()'s fprintf format string actually writes."""
    src = body(cpp, "bool WriteIni(")
    fmt = src[src.index('fprintf(f,'):src.index(');')]
    return set(re.findall(r"(\w+)=%[a-z]", fmt))


def parse_reader(cpp):
    """ini key -> (member, kind) from ReadIni(), skipping alias lines that
    read into a member another key already owns (StackMultiplier -> multiplier,
    kept for players who paste in a Private Storage Master line)."""
    src = body(cpp, "void ReadIni(")
    out, seen_members = {}, set()
    for m in re.finditer(r'_stricmp\(key,\s*"(\w+)"\)\s*==\s*0\)\s*out\.(\w+)\s*=\s*(.*?);', src):
        key, member, expr = m.group(1), m.group(2), m.group(3)
        if member in seen_members:
            continue                     # a compatibility alias for an already-covered member
        seen_members.add(member)
        kind = "bool" if "!= 0" in expr else "int" if "atoi" in expr else None
        if kind is None:
            raise ValueError("unrecognised reader for %s: %s" % (key, expr))
        out[key] = (member, kind)
    return out


def parse_clamp(cpp):
    """member -> (min, max) from Clamp(), resolving a named constant from settings.h."""
    src = body(cpp, "void Clamp(")
    bounds = {}
    for m in re.finditer(r"if \(v\.(\w+) < (\w+)\) v\.\1 = \2;", src):
        bounds.setdefault(m.group(1), [None, None])[0] = m.group(2)
    for m in re.finditer(r"if \(v\.(\w+) > (\w+)\) v\.\1 = \2;", src):
        bounds.setdefault(m.group(1), [None, None])[1] = m.group(2)
    return bounds


def resolve_constant(name, h):
    if re.match(r"^-?\d+$", name):
        return int(name)
    m = re.search(r"kMaxMultiplier\s*=\s*(\d+)", h) if name == "kMaxMultiplier" else None
    if m:
        return int(m.group(1))
    raise ValueError("cannot resolve clamp constant %s" % name)


def parse_defaults(h):
    """member -> default as the ini would write it, from the Values struct."""
    src = body(h, "struct Values")
    out = {}
    for m in re.finditer(r"^\s*(bool|int)\s+(\w+)\s*=\s*([^;]+);", src, re.M):
        typ, name, val = m.group(1), m.group(2), m.group(3).strip()
        if typ == "bool":
            out[name] = "1" if val == "true" else "0"
        else:
            out[name] = str(int(val, 0))
    return out


def uses_ini_watcher():
    for dirpath, _dirs, files in os.walk(SRC_DIR):
        for fn in files:
            if fn.endswith((".cpp", ".h")):
                if "IniWatcher" in read(os.path.join(dirpath, fn)):
                    return True
    return False


def load_meta():
    text = read(META)
    text = re.sub(r"^\s*//.*\n", "", text, flags=re.M)
    return json.loads(text)


def main():
    cpp, h = read(SETTINGS_CPP), read(SETTINGS_H)
    written = parse_written(cpp)
    reader = parse_reader(cpp)
    clamp = parse_clamp(cpp)
    defaults = parse_defaults(h)
    meta = load_meta()
    keys = meta["sections"]["StackMaster"]["keys"]
    errors = []

    watched = uses_ini_watcher()
    if meta.get("live") != watched:
        errors.append('mod-level live=%s, but the code %s an IniWatcher'
                       % (meta.get("live"), "uses" if watched else "does not use"))

    for k in sorted(written - set(keys)):
        errors.append("%s: the plugin writes it and the metadata does not describe it" % k)
    for k in sorted(set(keys) - set(reader)):
        errors.append("%s: in the metadata but ReadIni never reads it" % k)

    for k, spec in keys.items():
        if k not in reader:
            continue
        member, kind = reader[k]
        t = spec.get("type")
        if t != kind:
            errors.append("%s: type %s, but the plugin reads it as %s" % (k, t, kind))

        want = defaults.get(member)
        got = spec.get("default")
        if want is None:
            errors.append("%s: no default found for Values::%s in settings.h" % (k, member))
        elif str(got) != str(want):
            errors.append("%s: default %s, settings.h says %s" % (k, got, want))

        if member in clamp:
            lo_name, hi_name = clamp[member]
            if lo_name is not None:
                lo = resolve_constant(lo_name, h)
                if spec.get("min") != lo:
                    errors.append("%s: min %s, the plugin clamps to %s" % (k, spec.get("min"), lo))
            if hi_name is not None:
                hi = resolve_constant(hi_name, h)
                if spec.get("max") != hi:
                    errors.append("%s: max %s, the plugin clamps to %s" % (k, spec.get("max"), hi))

        if spec.get("live", meta.get("live")) is not False and not watched:
            errors.append("%s: live is not false, but the code never rereads the ini while the game runs" % k)

    for line in errors:
        print("error  " + line)
    print("%d keys in the metadata, %d written by the plugin, %d errors"
          % (len(keys), len(written), len(errors)))
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
