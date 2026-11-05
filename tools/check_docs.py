"""Check offline page coverage, lesson study sections and local link/asset integrity."""
from pathlib import Path
from html.parser import HTMLParser
from urllib.parse import urlsplit, unquote
import importlib.util
import json
import re

root=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('course_docs',root/'tools/build_docs.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
pages=module.pages()
broken=[]
class Links(HTMLParser):
    def __init__(self):super().__init__();self.values=[]
    def handle_starttag(self,tag,attrs):
        for key,value in attrs:
            if key in ('href','src') and value:self.values.append(value)
for md in pages:
    assert md.with_suffix('.html').is_file(),md
    text=md.read_text(encoding='utf-8')
    assert text.count('```')%2==0,md
    parser=Links();parser.feed(md.with_suffix('.html').read_text(encoding='utf-8'))
    for url in parser.values:
        parsed=urlsplit(url)
        if parsed.scheme or parsed.netloc or not parsed.path:continue
        dest=(md.parent/unquote(parsed.path)).resolve()
        if not dest.exists():broken.append((str(md.relative_to(root)),url))
topics=json.loads((root/'docs/comparison-topics.json').read_text(encoding='utf-8'))
assert len(topics)==16
for lesson,folder,path in [l.split('\t') for l in (root/'catalog.tsv').read_text(encoding='utf-8-sig').splitlines()]:
    if '/chapters/' in path:continue
    text=(root/path/'README.md').read_text(encoding='utf-8')
    for phrase in ['深入学习：数据、接口与执行','本课的真实源码切片','同主题跨 API 导航','先预测，再验证']:
        assert phrase in text,(lesson,phrase)
    if folder=='OpenGL':
        assert f'--preset opengl-debug --target {lesson}' in text,lesson
        assert '.\\scripts\\run.ps1' not in text,lesson
lab=Links();lab.feed((root/'docs/compare/index.html').read_text(encoding='utf-8'))
for url in lab.values:
    parsed=urlsplit(url)
    if not parsed.scheme and parsed.path and not (root/'docs/compare'/unquote(parsed.path)).resolve().exists():broken.append(('comparison-lab',url))

catalog_rows = [row.split('\t') for row in (root/'catalog.tsv').read_text(encoding='utf-8').splitlines()]
counts = {folder: sum(row[1] == folder for row in catalog_rows) for folder in ['OpenGL','Vulkan/native','Vulkan/nvpro','DirectX12']}
assert counts == {'OpenGL':34,'Vulkan/native':108,'Vulkan/nvpro':6,'DirectX12':108}, counts
curriculum = json.loads((root/'docs/curriculum.json').read_text(encoding='utf-8'))
assert len(curriculum['chapters']) == 54
assert {n for chapter in curriculum['chapters'] for n in chapter['labs']} == set(range(1,55))
for folder in ['Vulkan/native','DirectX12']:
    assert len(list((root/folder/'chapters').glob('*/README.md'))) == 54, folder
for chapter in curriculum['chapters']:
    for key in ['vk','dx','nv','task','answer','error']:
        assert len(chapter[key]) > (10 if key == "task" else 30), (chapter['slug'],key)

if broken:
    print(json.dumps(broken,ensure_ascii=False,indent=2));raise SystemExit(1)
print(f'PASS: {len(pages)} offline Markdown/HTML pairs; all local links/assets exist; 148 experiments + 108 independent chapter targets, 54+54 chapters; 16 comparison topics.')
