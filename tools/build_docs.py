"""Regenerate the offline reader. Python standard library only; not needed to run lessons."""
from pathlib import Path
import html
import re
import json
from urllib.parse import unquote, urlsplit

ROOT=Path(__file__).resolve().parents[1]
STYLE='''body{margin:0;background:#101722;color:#dce5ef;font:17px/1.85 system-ui,"Microsoft YaHei",sans-serif}main{max-width:1100px;margin:35px auto;padding:24px}a{color:#65c8ff;overflow-wrap:anywhere}h1{line-height:1.35;color:#fff}h2{margin-top:2em;border-bottom:1px solid #354052}pre{white-space:pre-wrap;overflow-wrap:anywhere;tab-size:4;background:#080d14;padding:20px;overflow:auto;border-radius:10px;line-height:1.6}code{overflow-wrap:anywhere;font-family:Consolas,monospace;color:#ffd17d}.table-scroll{overflow-x:auto;max-width:100%}table{border-collapse:collapse;width:100%}td,th{border:1px solid #354052;padding:9px}p{margin:1em 0}nav{font-size:14px;color:#a8b8c8}img{max-width:100%;border-radius:10px}li{margin:.4em 0}'''

def inline(text):
    text=html.escape(text)
    text=re.sub(r'`([^`]+)`',r'<code>\1</code>',text)
    text=re.sub(r'\*\*([^*]+)\*\*',r'<strong>\1</strong>',text)
    text=re.sub(r'!\[([^\]]*)\]\(([^)]+)\)',lambda m:'<img alt="'+m[1]+'" src="'+m[2]+'">',text)
    def link(m):
        dest=m[2][:-3]+'.html' if m[2].endswith('.md') else m[2]
        return '<a href="'+dest+'">'+m[1]+'</a>'
    return re.sub(r'\[([^\]]+)\]\(([^)]+)\)',link,text)

def render(markdown):
    out=[];code=False;buf=[];table=False;listing=False
    for line in markdown.splitlines():
        if line.startswith('```'):
            if listing:out.append('</ul>');listing=False
            if code:out.append('<pre><code>'+html.escape('\n'.join(buf))+'</code></pre>');buf=[]
            code=not code;continue
        if code:buf.append(line);continue
        if line.startswith('<!--') and line.endswith('-->'):continue
        if line.startswith('|'):
            if not table:out.append('<div class="table-scroll"><table>');table=True
            if re.fullmatch(r'[| :\-]+',line):continue
            out.append('<tr>'+''.join('<td>'+inline(cell.strip())+'</td>' for cell in line.strip('|').split('|'))+'</tr>');continue
        if table:out.append('</table></div>');table=False
        bullet=re.match(r'^(?:- |\d+\. )(.*)',line)
        if bullet:
            if not listing:out.append('<ul>');listing=True
            out.append('<li>'+inline(bullet[1])+'</li>');continue
        if listing:out.append('</ul>');listing=False
        heading=re.match(r'^(#{1,6}) (.*)',line)
        if heading:out.append(f'<h{len(heading[1])}>'+inline(heading[2])+f'</h{len(heading[1])}>')
        elif line.strip():out.append('<p>'+inline(line)+'</p>')
    if table:out.append('</table></div>')
    if listing:out.append('</ul>')
    return '\n'.join(out)

def pages():
    result=list(ROOT.glob('*.md'))+list((ROOT/'docs').rglob('*.md'))
    for folder in ['OpenGL','Vulkan','Vulkan/native','Vulkan/nvpro','DirectX12']:
        result+=list((ROOT/folder).glob('*.md'))+list((ROOT/folder/'docs').glob('*.md'))+list((ROOT/folder/'lessons').glob('*/*.md'))+list((ROOT/folder/'chapters').glob('*/*.md'))
    return sorted(set(result))

def build():
    from build_curriculum import build as build_curriculum
    build_curriculum()
    from build_comparison import build as build_comparison
    build_comparison()
    for page in pages():
        prefix='../'*len(page.relative_to(ROOT).parts[:-1]);text=page.read_text(encoding='utf-8');heading=re.search(r'^# (.+)',text,re.M);title=heading[1] if heading else page.stem
        nav=f'<nav><a href="{prefix}index.html">图形 API 教程首页</a> · C++ / CMake · OpenGL / Vulkan / DirectX 12</nav>'
        rendered = render(text)
        def project_link(match):
            raw = html.unescape(match[1]); parsed = urlsplit(raw)
            if parsed.scheme or parsed.netloc: return match[0]
            target = (page.parent / unquote(parsed.path)).resolve()
            meta = target.parent / 'project.json'
            if meta.is_file():
                info = json.loads(meta.read_text(encoding='utf-8'))
                return '<a data-course-target="'+info['target']+'" href="'+match[1]+'">'
            return match[0]
        rendered = re.sub(r'<a href="([^"]+)">', project_link, rendered)
        project = page.parent / 'project.json'
        project_meta = '<meta name="course-target" content="'+json.loads(project.read_text(encoding='utf-8'))['target']+'">' if project.is_file() else ''
        content=f'<!doctype html><html lang="zh-CN"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>{html.escape(title)}</title><style>{STYLE}</style><link rel="stylesheet" href="{prefix}web/runner.css"><script defer src="{prefix}web/runner.js"></script><main>'+nav+project_meta+rendered+'</main></html>'
        page.with_suffix('.html').write_bytes(content.encode('utf-8'))
    (ROOT/'index.html').write_bytes((ROOT/'README.html').read_bytes())
    print('Rendered',len(pages()),'offline pages')

if __name__=='__main__':build()
