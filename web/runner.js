(() => {
  'use strict';
  const bar = document.createElement('section');
  bar.className = 'course-launcher';
  bar.setAttribute('aria-label', '示例运行控制');
  const message = document.createElement('span'); message.setAttribute('role', 'status');
  message.textContent = '正在连接本地启动器…';
  const select = document.createElement('select'); select.setAttribute('aria-label', '运行版本');
  for (const value of ['Debug', 'Release']) { const option = new Option(value, value); select.add(option); }
  const close = document.createElement('button'); close.textContent = '关闭启动服务'; close.className = 'course-secondary';
  bar.append(message, select, close);
  const heading = document.querySelector('h1');
  if (heading) heading.after(bar); else document.querySelector('main').prepend(bar);
  let state = null, stopped = false, busy = false;
  const buttons = [];
  function addButton(name, container, label) {
    const button = document.createElement('button'); button.textContent = label; button.disabled = true;
    const status = document.createElement('span'); status.className = 'course-run-status';
    const log = document.createElement('a'); log.textContent = '查看运行输出'; log.target = '_blank'; log.hidden = true;
    container.append(button, status, log); buttons.push({name, button, status, log});
    button.addEventListener('click', async () => {
      if (!state) return; button.disabled = true; status.textContent = '正在启动…';
      try {
        const response = await fetch('/api/run', {method:'POST', headers:{'X-Course-Token':state.token, 'Content-Type':'text/plain'}, body:`${name}|${select.value}`});
        const result = await response.json(); if (!response.ok) throw new Error(result.message || '启动失败');
        message.textContent = result.message; await refresh();
      } catch (error) { status.textContent = error.message; button.disabled = false; }
    });
  }
  const chapter = location.pathname.match(/\/lessons\/((?:vk|nv|dx)?[0-9]{2}_[a-z0-9_]+)\//);
  if (chapter) {
    const controls = document.createElement('div'); controls.className = 'course-chapter-run';
    addButton(chapter[1], controls, '▶ 运行本课'); bar.after(controls);
    const tip = document.createElement('p'); tip.className = 'course-run-tip'; tip.textContent = '示例将在独立窗口打开。Esc 退出；有相机的课程按住右键转向，WASD 移动。'; controls.after(tip);
  }
  for (const table of document.querySelectorAll('table')) {
    let found = false;
    for (const row of table.rows) {
      const link = row.querySelector('a[href*="lessons/"]');
      const match = link && link.getAttribute('href').match(/lessons\/((?:vk|nv|dx)?[0-9]{2}_[a-z0-9_]+)\//);
      if (match) {const cell = row.insertCell(); cell.className = 'course-table-run';addButton(match[1], cell, '▶ 运行');found = true;}
    }
    if (found) {const cell = table.rows[0].insertCell();cell.textContent = '运行示例';}
  }
  function update() {
    for (const item of buttons) {
      const entry = state && state.lessons.find(e => e.name === item.name && e.config === select.value);
      item.button.disabled = !entry || !entry.available || entry.running;
      item.status.textContent = !entry ? '' : entry.running ? '运行中 · Esc 退出' : entry.failed ? `运行出错（${entry.exitCode}）` : !entry.available ? '此版本尚未构建' : entry.started ? '已结束' : '';
      item.log.hidden = !entry || !entry.started; item.log.href = `/api/log/${item.name}-${select.value}`;
    }
  }
  async function refresh() {
    if (busy || stopped || location.protocol === 'file:') return; busy = true;
    try {
      const response = await fetch('/api/status'); if (!response.ok) throw new Error();
      state = await response.json(); message.textContent = '本地启动器已连接 · 点击即可打开示例'; close.disabled = false;update();
    } catch (_) {state = null;message.textContent = '启动器未连接，请重新双击“开始学习.cmd”打开教程。';close.disabled = true;update();}
    finally {busy = false;}
  }
  select.addEventListener('change', update);
  close.addEventListener('click', async () => {
    if (!state) return;
    try {const response = await fetch('/api/shutdown', {method:'POST', headers:{'X-Course-Token':state.token}, body:''});if (!response.ok) throw new Error();stopped = true;state = null;close.disabled = true;message.textContent = '启动服务已关闭。重新双击“开始学习.cmd”可恢复；已打开的示例可继续运行。';update();}
    catch (_) {message.textContent = '未能关闭启动服务，请重试。';}
  });
  if (location.protocol === 'file:') {message.textContent = '要点击运行，请先双击教程目录里的“开始学习.cmd”，使用它打开的页面。';close.hidden = true;select.hidden = true;}
  else {refresh();setInterval(refresh, 2500);}
})();
