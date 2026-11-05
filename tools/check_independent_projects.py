"""Check ownership of chapter sources, shader stages and independent CMake entry points."""
from pathlib import Path
import json
import re

ROOT = Path(__file__).resolve().parents[1]


def main():
    items = json.loads((ROOT / 'docs/independent-projects.json').read_text(encoding='utf-8'))
    assert len(items) == 256
    assert len({item['target'] for item in items}) == len(items)
    assert len({item['path'] for item in items}) == len(items)
    counts = {'lesson': 0, 'chapter': 0}
    source_count = 0
    shader_count = 0
    for item in items:
        directory = ROOT / item['path']
        meta = json.loads((directory / 'project.json').read_text(encoding='utf-8'))
        assert meta['independent'] and meta['target'] == item['target'], item
        counts[item['kind']] += 1
        for name in ('main.cpp', 'CMakeLists.txt', 'cmake/Dependencies.cmake', 'LICENSE', 'NOTICE', 'shaders/README.md'):
            assert (directory / name).is_file(), (item['target'], name)
        build = (directory / 'CMakeLists.txt').read_text(encoding='utf-8')
        assert re.search(r'add_executable\(\s*' + re.escape(item['target']) + r'\s+main.cpp', build), item
        assert 'COURSE_RUNTIME_NAME' in build and item['target'] + '.resources' in build, item
        assert not re.search(r'\b(?:vk_advanced|dx_advanced|vk_course|dx_course|course_images)\b', build), item
        sources = [directory / 'main.cpp'] + list((directory / 'support').glob('*.hpp')) + list((directory / 'support').glob('*.cpp'))
        for source in sources:
            text = source.read_text(encoding='utf-8')
            source_count += 1
            for include in re.findall(r'^\s*#include\s+"([^"]+)"', text, flags=re.M):
                candidates = [source.parent / include, directory / include, directory / 'support' / include]
                match = next((p.resolve() for p in candidates if p.is_file()), None)
                assert match and match.is_relative_to(directory.resolve()), (source, include)
            assert 'advanced_labs.hpp' not in text, source
        entry = (directory / 'main.cpp').read_text(encoding='utf-8')
        assert entry.count('//') >= 2 and 'int main' in entry, directory
        assert not re.search(r'\b(?:nativeLabMain|dxLabMain|nvproMain)\s*\(', entry), directory
        if item['kind'] == 'chapter':
            assert entry.count('本章检查') >= 3, directory
        stage_files = [p for p in (directory / 'shaders').iterdir() if p.suffix in {'.vert', '.frag', '.geom', '.comp', '.hlsl'}]
        shader_count += len(stage_files)
        for shader in stage_files:
            text = shader.read_text(encoding='utf-8')
            assert '//' in text, shader
            if shader.suffix in {'.vert', '.frag', '.geom', '.comp'}:
                assert '#version' in text, shader
            if meta['api'] == 'dx':
                assert shader.name != 'scene.hlsl' and shader.name != 'lab.hlsl', shader
    assert counts == {'lesson': 148, 'chapter': 108}, counts
    catalog = [line.split('\t') for line in (ROOT / 'catalog.tsv').read_text(encoding='utf-8').splitlines()]
    assert {(name, path) for name, folder, path in catalog} == {(v['target'], v['path']) for v in items}
    print(f'PASS: {len(items)} independent projects ({counts}); {source_count} local C++ files, {shader_count} local shader stages; no cross-chapter course includes or implementation libraries.')


if __name__ == '__main__':
    main()
