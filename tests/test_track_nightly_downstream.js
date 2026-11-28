const assert = require('node:assert/strict');
const { test } = require('node:test');
const track = require('../.github/scripts/track-nightly-downstream.js');

const tag = 'v2.4.0-nightly.20261128.42';

function fixture() {
  const issues = [];
  const release = { tag_name: tag, draft: false, prerelease: true };
  let failTitle = '';
  let onGet = () => {};
  const github = {
    rest: {
      repos: { async getReleaseByTag(args) { assert.equal(args.tag, tag); return { data: release }; } },
      issues: {
        async listForRepo(args) { assert.equal(args.state, 'all'); return issues; },
        async create(data) {
          if (failTitle && data.title.includes(failTitle)) throw new Error('creation failed');
          const issue = { ...data, number: issues.length + 1, state: 'open' };
          issues.push(issue);
          return { data: issue };
        },
        async get({ issue_number }) {
          const issue = issues.find(item => item.number === issue_number);
          onGet(issue);
          return { data: issue };
        },
      },
    },
    async paginate(method, args) { return [...await method(args)]; },
  };
  const args = { github, context: { repo: { owner: 'owner', repo: 'repo' } }, core: { info() {} }, tag };
  return { issues, release, args, failOn(value) { failTitle = value; }, onGet(fn) { onGet = fn; } };
}

test('creates two isolated approval issues and preserves human edits on retry', async () => {
  const f = fixture();
  const result = await track(f.args);
  assert.equal(result.length, 2);
  assert.deepEqual(result.map(item => item.number), [1, 2]);
  assert.deepEqual(f.issues.map(issue => issue.labels[0]), [
    'winget-release', 'docker-release',
  ]);
  for (const issue of f.issues) {
    assert.ok(!issue.labels.includes('release-review'));
    assert.match(issue.body, new RegExp(`## Release Version: ${tag.replaceAll('.', '\\.')}`));
    assert.match(issue.body, /https:\/\/github\.com\/owner\/repo\/releases\/tag\/v2\.4\.0-nightly\.20261128\.42/);
    assert.match(issue.body, /admin or maintainer/);
    assert.match(issue.body, /GitHub Environment/);
    assert.match(issue.body, /not publication/);
  }
  assert.match(f.issues[0].body, /^\*\*WinGet prerelease opt-in:\*\* false$/m);
  assert.match(f.issues[0].body, /upstream review, merge/);
  assert.match(f.issues[1].body, /separate nightly Docker pipeline/);
  f.issues[0].body = f.issues[0].body.replace('opt-in:** false', 'opt-in:** true') + '\nHuman review';
  const saved = f.issues.map(issue => issue.body);
  assert.deepEqual(await track(f.args), result);
  assert.equal(f.issues.length, 2);
  assert.deepEqual(f.issues.map(issue => issue.body), saved);
});

test('invalid tag and non-public or mismatched release fail before listing or creating issues', async () => {
  for (const invalid of ['', 'v2.4.0', 'v2.4.0-nightly.20261128.42\nother', 'v2.4.0-nightly.20261128.42+extra']) {
    const f = fixture();
    f.args.tag = invalid;
    await assert.rejects(track(f.args), /exact nightly tag/);
    assert.equal(f.issues.length, 0);
  }
  for (const change of [{ draft: true }, { prerelease: false }, { tag_name: 'v2.4.0-nightly.20261128.43' }]) {
    const f = fixture();
    Object.assign(f.release, change);
    await assert.rejects(track(f.args), /published GitHub prerelease/);
    assert.equal(f.issues.length, 0);
  }
});

test('rejects closed, ambiguous, duplicate, mislabeled and wrong-version matches before creating anything', async () => {
  const base = { number: 9, state: 'open', title: `📦 Docker Release: ${tag}`,
    body: `<!-- registry-release:${tag}:publish-docker -->\n## Release Version: ${tag}`, labels: ['docker-release'] };
  for (const [issues, pattern] of [
    [[{ ...base, state: 'closed' }], /closed/],
    [[{ ...base, body: 'unrelated' }], /Ambiguous/],
    [[base, { ...base, number: 10 }], /Multiple/],
    [[{ ...base, labels: [] }], /lacks the required/],
    [[{ ...base, labels: ['docker-release', 'release-review'] }], /release-review/],
    [[{ ...base, labels: ['docker-release', 'winget-release'] }], /another registry/],
    [[{ ...base, labels: ['docker-release', 'linux-distro-review'] }], /another registry/],
    [[{ ...base, labels: ['docker-release', 'windows-distro-review'] }], /another registry/],
    [[{ ...base, body: `${base.body}\n## Release Version: v2.4.1` }], /does not identify exactly/],
    [[{ ...base, body: `prefix ## Release Version: v2.4.1\n${base.body}` }], /does not identify exactly/],
  ]) {
    const f = fixture();
    f.issues.push(...issues);
    await assert.rejects(track(f.args), pattern);
    assert.equal(f.issues.length, issues.length);
  }
});

test('reuses legacy issues and ignores unrelated versions and pull requests', async () => {
  const f = fixture();
  const legacy = { number: 10, state: 'open', title: `📦 WinGet Release: ${tag}`,
    body: `## Release Version: ${tag}\n\nHuman evidence`, labels: [{ name: 'winget-release' }] };
  f.issues.push(legacy, { ...legacy, number: 11, pull_request: {} },
    { ...legacy, number: 12, title: '📦 WinGet Release: v2.4.0-nightly.20261128.43',
      body: '## Release Version: v2.4.0-nightly.20261128.43' });
  const result = await track(f.args);
  assert.equal(result[0].number, 10);
  assert.equal(legacy.body, `## Release Version: ${tag}\n\nHuman evidence`);
  assert.equal(f.issues.length, 4);
});

test('never creates or changes Linux and Windows distro tracking issues for nightly', async () => {
  const f = fixture();
  const existing = ['Linux', 'Windows'].map((name, index) => ({
    number: index + 10, state: 'open', title: `📦 ${name} Distro Release: ${tag}`,
    body: `<!-- registry-release:${tag}:publish-${name.toLowerCase()} -->\n## Release Version: ${tag}\nHuman note`,
    labels: [`${name.toLowerCase()}-distro-review`],
  }));
  f.issues.push(...existing);
  const before = structuredClone(existing);
  const result = await track(f.args);
  assert.deepEqual(result.map(item => item.name), ['WinGet', 'Docker']);
  assert.deepEqual(existing, before);
  assert.equal(f.issues.length, 4);
  for (const issue of f.issues.slice(2)) {
    assert.ok(!issue.labels.includes('linux-distro-review'));
    assert.ok(!issue.labels.includes('windows-distro-review'));
    assert.doesNotMatch(issue.body, /\/publish-(?:linux|windows)/);
  }
});

test('partial failures can be retried and state is rechecked before reuse', async () => {
  const f = fixture();
  f.failOn('Docker');
  await assert.rejects(track(f.args), /creation failed/);
  assert.equal(f.issues.length, 1);
  f.failOn('');
  await track(f.args);
  assert.equal(f.issues.length, 2);
  f.onGet(issue => { issue.state = 'closed'; });
  await assert.rejects(track(f.args), /closed/);
  assert.equal(f.issues.length, 2);
});
