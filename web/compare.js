(() => {
  'use strict';
  const topics = [...document.querySelectorAll('[data-topic]')];
  const panels = [...document.querySelectorAll('[data-topic-panel]')];
  const filters = [...document.querySelectorAll('[data-api-filter]')];
  const search = document.querySelector('#topic-search');
  const searchStatus = document.querySelector('#search-status');
  let api = 'all';

  function filterApi(value) {
    api = value;
    for (const button of filters) {
      button.setAttribute('aria-pressed', String(button.dataset.apiFilter === api));
    }
    for (const card of document.querySelectorAll('[data-api]')) {
      card.hidden = api !== 'all' && card.dataset.api !== api;
    }
  }

  function showTopic(id, updateHash = true) {
    if (!topics.some(button => button.dataset.topic === id)) return;
    for (const button of topics) {
      button.setAttribute('aria-pressed', String(button.dataset.topic === id));
    }
    for (const panel of panels) panel.hidden = panel.dataset.topicPanel !== id;
    if (updateHash) history.replaceState(null, '', '#' + id);
    filterApi(api);
  }

  for (const button of topics) button.addEventListener('click', () => showTopic(button.dataset.topic));
  for (const button of filters) button.addEventListener('click', () => filterApi(button.dataset.apiFilter));
  search.addEventListener('input', () => {
    const term = search.value.trim().toLocaleLowerCase();
    let count = 0;
    for (const button of topics) {
      const matches = button.textContent.toLocaleLowerCase().includes(term);
      button.hidden = !matches;
      if (matches) count += 1;
    }
    searchStatus.textContent = count ? `找到 ${count} 个主题` : '没有匹配主题，请换一个关键词。';
  });
  window.addEventListener('hashchange', () => showTopic(location.hash.slice(1), false));
  showTopic(location.hash.slice(1) || topics[0].dataset.topic, false);
})();
