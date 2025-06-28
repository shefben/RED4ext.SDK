import os
import re
import yaml

GAME_HEADER = '# Plugin API\n\n## game module\n'
PLUG_HEADER = '\n## Plugins\n'

METHOD_RE = re.compile(r'{"(.*?)",')

def parse_game_methods():
    path = os.path.join('cp2077-coop', 'src', 'plugin', 'PythonVM.cpp')
    with open(path, 'r', encoding='utf-8') as f:
        txt = f.read()
    m = re.search(r'static PyMethodDef gameMethods\[\][^=]*=\s*\{([\s\S]*?)\};', txt)
    if not m:
        return []
    methods = [name for name in METHOD_RE.findall(m.group(1)) if not name.startswith('_')]
    return methods

def parse_plugins():
    plugins = []
    if not os.path.isdir('plugins'):
        return plugins
    for root, _, files in os.walk('plugins'):
        for fn in files:
            if not fn.endswith('.py'):
                continue
            path = os.path.join(root, fn)
            with open(path, 'r', encoding='utf-8') as f:
                text = f.read()
            m = re.search(r'__plugin__\s*=\s*(\{.*?\})', text, re.S)
            meta = yaml.safe_load(m.group(1)) if m else {}
            plugins.append({'file': os.path.relpath(path, "plugins"), **meta})
    return plugins

def main():
    lines = [GAME_HEADER]
    for m in parse_game_methods():
        lines.append(f'- `{m}`')
    lines.append(PLUG_HEADER)
    for p in parse_plugins():
        lines.append(f"* **{p.get('name', p['file'])}** v{p.get('version', '?')}")
    content = '\n'.join(lines) + '\n'
    with open('DOCS.md', 'w', encoding='utf-8') as f:
        f.write(content)

if __name__ == '__main__':
    main()
