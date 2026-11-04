"""Generate the paired 54-chapter curricula and advanced experiment guides."""
from pathlib import Path
import json
import os

from build_comparison import excerpt

ROOT = Path(__file__).resolve().parents[1]
TRACKS = {'vk': ('Vulkan 原生', 'Vulkan/native', 'vulkan-native'),
          'dx': ('DirectX 12', 'DirectX12', 'dx12')}


def relative(target, directory):
    return os.path.relpath(ROOT / target, directory).replace('\\', '/')


def source_block(source, directory):
    code, start, end = excerpt(source)
    suffix = Path(source['file']).suffix
    language = 'glsl' if suffix in {'.vert', '.frag', '.comp'} else 'hlsl' if suffix == '.hlsl' else 'cpp'
    return (f'来自 [{source["file"]}]({relative(source["file"], directory)}#L{start})，'
            f'第 {start}—{end} 行。下列是当前仓库的局部切片，可能在函数中间结束；'
            f'完整上下文请打开文件，切片不作为单独可编译程序。\n\n```{language}\n{code}\n```\n')


def experiment_rows(chapter, key, directory, experiments):
    rows = '| 路线 | 实际实验 |\n| --- | --- |\n'
    for route in (key, 'dx' if key == 'vk' else 'vk'):
        name = TRACKS[route][0]
        for number in chapter['labs']:
            lab = experiments[str(number)][route]
            rows += f'| {name} | [{lab["name"]}]({relative(lab["path"]+"/README.md", directory)}) |\n'
    return rows


def build():
    content = json.loads((ROOT / 'docs/curriculum.json').read_text(encoding='utf-8'))
    chapters, experiments = content['chapters'], content['experiments']
    assert len(chapters) == 54 and len(experiments) == 54
    assert {n for chapter in chapters for n in chapter['labs']} == set(range(1, 55))
    maps = json.loads((ROOT / 'docs/lesson-source-map.json').read_text(encoding='utf-8'))
    maps = [item for item in maps if not any(f'/lessons/{key}' in item['guide'] and
            int(item['guide'].split('/lessons/')[1][2:4]) > 10 for key in TRACKS)]
    # Preserve the original 60 guides; advanced guides are regenerated from the curriculum data.
    for key, (label, folder, preset) in TRACKS.items():
        index = f'# {label} · 54 课完整主线\n\n'
        index += (f'[路线首页](README.md) · [三 API 对照](../../docs/COMPARE.md)\n\n' if key == 'vk'
                  else '[路线首页](README.md) · [三 API 对照](../docs/COMPARE.md)\n\n')
        index += ('按照指定 Vulkan 教程的基础顺序组织，再加入工程扩展。每课包括目的、接口推导、'
                  '真实源码、可运行实验、失败定位和参考答案。54 章与 54 个实验是两套索引：'
                  '章号用于学习次序，实验号用于保持已有链接；同一实验可能服务多个相关小节。\n\n'
                  '| 章 | 主题 | 验收关注 |\n| --- | --- | --- |\n')
        for i, chapter in enumerate(chapters):
            path = ROOT / folder / 'chapters' / chapter['slug'] / 'README.md'
            path.parent.mkdir(parents=True, exist_ok=True)
            directory = path.parent
            nav = [f'[54 课目录]({relative(folder+"/COURSE.md", directory)})',
                   f'[对照精讲]({relative("docs/compare/"+chapter["comparison"]+".md", directory)})']
            if i:
                nav.append('[上一课](../' + chapters[i - 1]['slug'] + '/README.md)')
            if i + 1 < len(chapters):
                nav.append('[下一课](../' + chapters[i + 1]['slug'] + '/README.md)')
            text = f'# 第 {i+1:02} 课 · {chapter["title"]} · {label}\n\n' + ' · '.join(nav) + '\n\n'
            text += '## 本课目标与先修\n\n'
            text += f'学习目标：解释“{chapter["title"]}”解决的问题，找到实际对象和参数，运行对应实验并用数据/画面证明。'
            text += ('先完成前一课，并能说明它交付的对象与完成条件；本课在该依赖上增加新契约。\n\n' if i
                     else '先了解 C++ 对象作用域和基本向量/矩阵；环境配置在下一课逐项检查。\n\n')
            text += '## 为什么需要这一层\n\n' + '\n\n'.join(chapter['common']) + '\n\n'
            text += f'## {label} 的对象、参数与执行推导\n\n{chapter[key]}\n\n'
            other = 'dx' if key == 'vk' else 'vk'
            text += f'## 对照 {TRACKS[other][0]}：相同问题的另一种分工\n\n{chapter[other]}\n\n'
            text += '## 关键参数与可检查不变量\n\n'
            text += '| 检查项 | Vulkan 表达 | DX12 表达 | 不变量 / 证据 |\n| --- | --- | --- | --- |\n'
            for row in chapter['contracts']:
                text += '| ' + ' | '.join(row) + ' |\n'
            text += '\n先为每项指出源码中的创建/绑定位置，以及证明它成立的实际日志、字节或图像。数值中的设备限制以查询为准；固定实验尺寸不当作所有设备的上限。\n\n'
            text += f'## nvpro_core2 框架对照\n\n{chapter["nv"]}\n\n'
            text += ('这部分是原生 Vulkan 各课的框架对照，框架可运行入口仍为原有六课。'
                     '如果本节需要新接入工作，会按正文说明；不把原生实验当作已完成的框架实现。\n\n')
            text += '## 运行与实际检查\n\n' + experiment_rows(chapter, key, directory, experiments) + '\n'
            lab = experiments[str(chapter['labs'][0])][key]
            text += ('从合集根目录执行；普通图形程序 Esc 退出，设备/计算/诊断程序输出结果后结束。'
                     '从“开始学习.cmd”进入本页时，表格可直接点击运行。\n\n')
            text += (f'```powershell\ncmake --preset {preset}\n'
                     f'cmake --build --preset {preset}-debug --target {lab["name"]} --parallel 4\n'
                     f'.\\{folder.replace("/", chr(92))}\\build\\bin\\Debug\\{lab["name"]}.exe\n'
                     f'ctest --preset {preset}-debug -R "^{lab["name"]}$" --output-on-failure\n```\n\n')
            text += ('检查三个层次：创建/绑定/提交的接口约束；数据读回或目标画面；资源使用结束后的回收。'
                     '诊断实验用合法信息或查询证明渠道可用，不故意保留 API 错误。'
                     '图形 smoke 的像素检查与数值实验的逐项检查不同，具体实现见源码。\n\n')
            text += '## 关键实现：从当前源码定位\n\n' + source_block(chapter['sources'][key], directory) + '\n'
            text += ('阅读时按“前置对象 → 参数/布局 → 录制/调用 → 完成/可见性 → 释放”五步标注。'
                     '共享 helper 减少重复代码，但不会取消每课的 API 契约。\n\n')
            text += f'## 受控修改实验\n\n{chapter["task"]}\n\n'
            text += ('先记录预测，再只改一个因素；保留基线与修改后的输出。不要同时改变 shader、'
                     '资源范围和同步，这样即使结果变好也无法归因。\n\n')
            text += f'### 参考推导与通过标准\n\n{chapter["answer"]}\n\n'
            text += f'## 常见失败与排查顺序\n\n{chapter["error"]}\n\n'
            text += ('先看第一条错误和当前执行路径，再看实际数据/绑定，最后看 shader 与画面。'
                     '需要 GPU 证据时抓取正常交互帧；仅四帧的 smoke 适合自动检查，不适合手动抓帧。\n\n')
            text += f'## 自测与本课交付\n\n能否不看源码回答：{chapter["task"]}\n\n'
            text += ('提交一份简短记录：输入与假设、唯一修改、预期、实际证据、原因、是否恢复基线。'
                     '如果只能复述函数名而说不清何时允许访问/释放，先回到本课的数据流。\n\n')
            text += (f'## 官方阅读位置与当前工程差异\n\n'
                     f'[指定 Vulkan 教程对应位置](https://docs.vulkan.net.cn/tutorial/latest/{chapter["reference"]})。'
                     'DX12 路线按同一概念顺序建立映射，其对象与约束以 Microsoft 文档和本后端代码为准。'
                     '49—54 课包含在基础/compute 之后加入的工程扩展。\n\n')
            text += (f'[版本与资料说明]({relative("docs/REFERENCES.md", directory)}) · '
                     f'[调试]({relative("docs/DEBUGGING.md", directory)}) · '
                     f'[功能完成度]({relative("docs/FEATURE_MATRIX.md", directory)})\n')
            preview = ROOT / 'docs/previews' / (lab['name'] + '.png')
            if preview.is_file():
                text += f'\n## 固定输入的实际 GPU 读回\n\n![{lab["name"]}]({relative(str(preview.relative_to(ROOT)), directory)})\n'
            path.write_bytes(text.encode('utf-8'))
            index += f'| {i+1:02} | [{chapter["title"]}](chapters/{chapter["slug"]}/README.md) | {chapter["task"]} |\n'
        (ROOT / folder / 'COURSE.md').write_bytes(index.encode('utf-8'))
        for number in range(11,55):
            lab = experiments[str(number)][key]
            linked = [c for c in chapters if number in c['labs']]
            guide = ROOT / lab['path'] / 'README.md'
            directory = guide.parent
            title = json.loads((directory / 'advanced.json').read_text(encoding='utf-8'))['title']
            text = f'# {lab["name"]} · {title}\n\n[54 课目录]({relative(folder+"/COURSE.md", directory)})\n\n'
            text += '## 目的与实际执行\n\n'
            text += '\n\n'.join(c[key] for c in linked) + '\n\n'
            text += ('本程序有独立实验选择与验证路径，公共实现集中在 advanced_labs.cpp。'
                     '设备/API 检查、数值 compute 与图形渲染采用不同证据；不是每个程序都会弹出窗口。\n\n')
            text += '## 构建和运行\n\n'
            text += (f'```powershell\ncmake --preset {preset}\ncmake --build --preset {preset}-debug '
                     f'--target {lab["name"]} --parallel 4\n.\\{folder.replace("/",chr(92))}\\build\\bin\\Debug\\{lab["name"]}.exe\n```\n\n')
            text += '## 深入学习：数据、接口与执行\n\n'
            for chapter in linked:
                text += f'- [第 {chapter["slug"][:2]} 课：{chapter["title"]}]({relative(folder+"/chapters/"+chapter["slug"]+"/README.md",directory)})\n'
            source = content['sources'][str(number)][key]
            text += '\n### 本课的真实源码切片\n\n' + source_block(source,directory) + '\n'
            peer = experiments[str(number)]['dx' if key=='vk' else 'vk']
            text += ('### 同主题跨 API 导航\n\n| 路线 | 程序 |\n| --- | --- |\n'
                     f'| {label} | [{lab["name"]}](README.md) |\n'
                     f'| 对照后端 | [{peer["name"]}]({relative(peer["path"]+"/README.md",directory)}) |\n\n')
            text += '### 先预测，再验证\n\n' + '\n\n'.join(c['task']+'\n\n参考推导：'+c['answer'] for c in linked) + '\n\n'
            text += ('## 验证与边界\n\n运行 `--smoke` 执行有限自动检查。数值实验读取实际 GPU 结果，'
                     '图形实验检查非空图像并可查看读回。交互重建与性能还需额外证据；'
                     '窗口 renderer 当前保留串行等待，双槽计算实验不会被描述为完整多帧呈现引擎。\n')
            preview=ROOT/'docs/previews'/(lab['name']+'.png')
            if preview.is_file():text += f'\n## 实际画面\n\n![{lab["name"]}]({relative(str(preview.relative_to(ROOT)),directory)})\n'
            guide.write_bytes(text.encode('utf-8'))
            maps.append({'guide':lab['path']+'/README.md',**source})
    (ROOT/'docs/lesson-source-map.json').write_bytes((json.dumps(maps,ensure_ascii=False,indent=2)+'\n').encode('utf-8'))
    table = '# Vulkan / DX12 · 54 课逐章对照\n\n'
    table += ('[Vulkan 主线](../Vulkan/native/COURSE.md) · [DX12 主线](../DirectX12/COURSE.md) · '
              '[nvpro 框架对照](../Vulkan/nvpro/CHAPTER_MAP.md)\n\n'
              '两条主线各 54 章与 54 个独立可执行实验；章号依官方概念顺序，实验号保留旧入口。'
              '框架路线按用户选择作为逐章对照，仍有六个独立框架例子。\n\n'
              '| 章 | 主题 | Vulkan | DX12 |\n| --- | --- | --- | --- |\n')
    nv='# nvpro_core2 · 原生 Vulkan 54 课框架对照\n\n'
    nv+='[框架路线首页](README.md) · [原生 54 课](../native/COURSE.md)\n\n'
    nv+='按每章列出 Context、Allocator、Application、RenderTarget 与自定义 pass 的边界。保留现有六个可运行框架示例；未接入的高级功能明确作为移植工作。\n\n'
    nv+='| 章 / 原生教材 | 框架职责与边界 |\n| --- | --- |\n'
    for i,c in enumerate(chapters,1):
        table+=f'| {i:02} | {c["title"]} | [阅读](../Vulkan/native/chapters/{c["slug"]}/README.md) | [阅读](../DirectX12/chapters/{c["slug"]}/README.md) |\n'
        nv+=f'| [{i:02} {c["title"]}](../native/chapters/{c["slug"]}/README.md) | {c["nv"]} |\n'
    (ROOT/'docs/CURRICULUM.md').write_bytes(table.encode('utf-8'))
    (ROOT/'Vulkan/nvpro/CHAPTER_MAP.md').write_bytes(nv.encode('utf-8'))
    print('Built 108 detailed curriculum chapters, 88 advanced experiment guides and nvpro crosswalk.')


if __name__ == '__main__':
    build()
