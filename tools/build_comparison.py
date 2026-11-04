"""Build the offline comparison lab from topic metadata and real repository snippets."""
from pathlib import Path
import html
import json
import os

ROOT = Path(__file__).resolve().parents[1]
NAMES = {'gl': 'OpenGL', 'vk': 'Vulkan 原生', 'nv': 'Vulkan / nvpro_core2', 'dx': 'DirectX 12'}


def excerpt(item):
    if not item['file']:
        return '', 0, 0
    lines = (ROOT / item['file']).read_text(encoding='utf-8').splitlines()
    start = next((i for i, line in enumerate(lines) if item['anchor'] in line), None)
    if start is None:
        raise ValueError(f"Missing source anchor: {item['file']}: {item['anchor']}")
    start = max(0, start - item.get('lead', 0))
    end = min(len(lines), start + item['count'])
    return '\n'.join(lines[start:end]), start + 1, end


def refresh_lesson_sources():
    """Keep the lesson excerpts and line locations synchronized with source edits."""
    entries = json.loads((ROOT / 'docs/lesson-source-map.json').read_text(encoding='utf-8'))
    for item in entries:
        code, start, end = excerpt(item)
        guide = ROOT / item['guide']
        relative = os.path.relpath(ROOT / item['file'], guide.parent).replace('\\', '/')
        suffix = Path(item['file']).suffix
        language = 'glsl' if suffix in {'.frag', '.vert', '.comp'} else 'hlsl' if suffix == '.hlsl' else 'cpp'
        section = '### 本课的真实源码切片\n\n'
        section += f'来自 [{item["file"]}]({relative}#L{start})，第 {start}—{end} 行；这是现有实现的局部摘录，末尾可能处于函数中间，不是可单独编译的完整程序。\n\n'
        section += f'```{language}\n{code}\n```\n\n'
        body = guide.read_text(encoding='utf-8')
        begin = body.index('### 本课的真实源码切片')
        finish = body.index('### 同主题跨 API 导航', begin)
        guide.write_bytes((body[:begin] + section + body[finish:]).encode('utf-8'))


def build():
    refresh_lesson_sources()
    topics = json.loads((ROOT / 'docs/comparison-topics.json').read_text(encoding='utf-8'))
    nav, sections = [], []
    destination = ROOT / 'docs/compare/index.html'
    def url(relative):
        return os.path.relpath(ROOT / relative, destination.parent).replace('\\', '/')
    for index, topic in enumerate(topics):
        slug = topic['id']
        nav.append(f'<button type="button" data-topic="{slug}" aria-pressed="{str(index == 0).lower()}">{html.escape(topic["title"])}</button>')
        cards = []
        appendix = '\n\n## 与本仓库实现并排核对\n\n以下切片直接取自当前仓库，省略了前后文。行号用于定位，不能把片段当作完整可编译程序。\n\n'
        appendix += '| 路线 | 可运行参照 / 范围 |\n| --- | --- |\n'
        for key, item in topic['tracks'].items():
            code, start, end = excerpt(item)
            lesson = item['lesson']
            lesson_path = f'{item["folder"]}/lessons/{lesson}/README.md' if lesson else None
            appendix += f'| {NAMES[key]} | ' + (f'[{lesson}]({url(lesson_path)})' if lesson else item['status']) + ' |\n'
            content = f'<p class="status-badge">{html.escape(item["status"])}</p><p>{html.escape(item["explanation"])}</p>'
            if lesson:
                href = url(lesson_path).replace('.md', '.html')
                content += f'<table><tr><td>示例</td></tr><tr><td><a href="{html.escape(href, quote=True)}">{lesson}</a></td></tr></table>'
            if code:
                href = url(item['file'])
                content += f'<p class="source-label"><a href="{href}" target="_blank" rel="noopener">{html.escape(item["file"])}</a> · {start}—{end} 行</p><pre><code>{html.escape(code)}</code></pre>'
            else:
                content += '<p class="scope-note">此项没有对应的已完成实现。请阅读精讲中的迁移步骤与验收条件。</p>'
            cards.append(f'<article class="api-card" data-api="{key}"><h3>{NAMES[key]}</h3>{content}</article>')
        for key, item in topic['tracks'].items():
            code, start, end = excerpt(item)
            appendix += f'\n### {NAMES[key]} · {item["status"]}\n\n{item["explanation"]}\n\n'
            if code:
                suffix = Path(item['file']).suffix
                language = 'glsl' if suffix in {'.frag', '.vert', '.comp'} else 'hlsl' if suffix == '.hlsl' else 'cpp'
                appendix += f'[{item["file"]}]({url(item["file"])}#L{start})，第 {start}—{end} 行：\n\n```{language}\n{code}\n```\n'
        chapter = ROOT / 'docs/compare' / topic['chapter']
        text = chapter.read_text(encoding='utf-8').split('\n<!-- generated-source-comparison -->')[0]
        chapter.write_bytes(((text.rstrip() + '\n\n<!-- generated-source-comparison -->' + appendix).rstrip() + '\n').encode('utf-8'))
        rows = ''.join('<tr>' + ''.join('<td>' + html.escape(cell) + '</td>' for cell in row) + '</tr>' for row in topic['rows'])
        matrix = '<div class="matrix-scroll"><table><tr><th>问题</th>' + ''.join(f'<th>{name}</th>' for name in NAMES.values()) + '</tr>' + rows + '</table></div>'
        hidden = '' if index == 0 else ' hidden'
        chapter_url = topic['chapter'].replace('.md', '.html')
        sections.append(f'<section class="comparison-topic" data-topic-panel="{slug}"{hidden}><h2>{index:02d} · {html.escape(topic["title"])}</h2><p>{html.escape(topic["goal"])}</p><p><a class="deep-link" href="{chapter_url}">阅读完整精讲、实验和参考答案 →</a></p><p class="question">先想一想：{html.escape(topic["question"])}</p>{matrix}<div class="api-grid">{"".join(cards)}</div></section>')
    page = '''<!doctype html>
<html lang="zh-CN"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>三种图形 API 对照实验室</title><link rel="stylesheet" href="../../web/compare.css">
<link rel="stylesheet" href="../../web/runner.css"><script defer src="../../web/compare.js"></script>
<script defer src="../../web/runner.js"></script></head><body><main>
<nav><a href="../../index.html">教程首页</a> · <a href="../COMPARE.html">主题路线</a> · <a href="../../Vulkan/native/COURSE.html">Vulkan 54 课</a> · <a href="../../DirectX12/COURSE.html">DX12 54 课</a> · <a href="../FEATURE_MATRIX.html">功能完成度</a> · <a href="../REFERENCES.html">参考资料</a></nav>
<h1>三种图形 API 对照实验室</h1><p class="intro">同一问题，三种 API 的责任分配。Vulkan 另列 nvpro_core2 框架路线；代码来自本仓库实际实现。</p>
<div class="scope-note">共 16 个精讲主题、148 个可运行实验，并新增 Vulkan/DX12 各 54 课主线。标为“迁移设计 / 基线外”的功能不计作已实现；运行参照不保证场景完全一致，也不用于比较 FPS。</div>
<div class="lab-layout"><aside><label for="topic-search">查找主题</label><input id="topic-search" type="search" placeholder="例如：纹理、同步、PBR"><nav id="topic-nav" aria-label="对照主题">TOPIC_NAV</nav><p id="search-status" role="status"></p></aside>
<div class="lab-content"><div class="api-filters" role="group" aria-label="源码显示范围"><button type="button" data-api-filter="all" aria-pressed="true">并排对照</button><button type="button" data-api-filter="gl" aria-pressed="false">OpenGL</button><button type="button" data-api-filter="vk" aria-pressed="false">Vulkan 原生</button><button type="button" data-api-filter="nv" aria-pressed="false">nvpro_core2</button><button type="button" data-api-filter="dx" aria-pressed="false">DX12</button></div>TOPIC_SECTIONS</div></div>
<p class="footer-note">选主题 → 读精讲 → 查真实代码 → 预测修改结果 → 运行与记录。直接打开 HTML 可阅读和切换；点击运行请从“开始学习.cmd”进入。</p></main></body></html>'''
    destination.write_bytes(page.replace('TOPIC_NAV', ''.join(nav)).replace('TOPIC_SECTIONS', ''.join(sections)).encode('utf-8'))
    print(f'Built comparison lab: {len(topics)} topics, real source snippets and lesson links.')


if __name__ == '__main__':
    build()
