const assert = require('node:assert/strict');
const fs = require('node:fs');
const { test } = require('node:test');

const workflow = fs.readFileSync('.github/workflows/release-mainline-approval.yml', 'utf8');
const AsyncFunction = Object.getPrototypeOf(async function () {}).constructor;

function stepScript(name, version = '2.4.0') {
  const start = workflow.indexOf(`      - name: ${name}\n`);
  assert.notEqual(start, -1);
  const end = workflow.indexOf('\n      - name:', start + 1);
  const step = workflow.slice(start, end === -1 ? undefined : end);
  const script = step.split('          script: |\n')[1];
  assert.ok(script);
  return new AsyncFunction('github', 'context', 'core', script.split('\n')
    .map(line => line.startsWith('            ') ? line.slice(12) : line)
    .join('\n')
    .replaceAll('${{ steps.extract_version.outputs.version }}', version)
    .replaceAll('${{ github.repository }}', 'owner/repo'));
}

function trackerFixture(existing = []) {
  const issues = existing;
  const outputs = {};
  const removed = [];
  let failName;
  const github = {
    rest: { issues: {
      async listForRepo() { return issues; },
      async create(data) {
        if (data.title.includes(failName || '\0')) throw new Error('creation failed');
        const issue = { ...data, number: issues.length + 100, state: 'open' };
        issues.push(issue);
        return { data: issue };
      },
      async get({ issue_number }) { return { data: issues.find(issue => issue.number === issue_number) }; },
      async update({ issue_number, body }) { issues.find(issue => issue.number === issue_number).body = body; },
      async removeLabel({ issue_number, name }) {
        removed.push(issue_number);
        const issue = issues.find(item => item.number === issue_number);
        issue.labels = issue.labels.filter(label => (label.name || label) !== name);
      },
    } },
    async paginate(method, args) { return [...await method(args)]; },
  };
  return {
    issues, outputs, removed, github,
    core: { info() {}, setOutput(key, value) { outputs[key] = value; } },
    context: { repo: { owner: 'owner', repo: 'repo' } },
    failOn(name) { failName = name; },
  };
}

test('registry issue creation is idempotent, preserves human edits and rejects closed matches', async () => {
  const run = stepScript('Create tracking issues for downstream registries', '2.4.0-rc1');
  const f = trackerFixture();
  await run(f.github, f.context, f.core);
  assert.equal(f.issues.length, 4);
  const winget = f.issues[0];
  assert.match(winget.body, /^\*\*WinGet prerelease opt-in:\*\* false$/m);
  assert.doesNotMatch(winget.body, /^\*\*WinGet prerelease opt-in:\*\* true$/m);
  assert.ok(winget.body.indexOf('post the exact comment') < winget.body.indexOf('Approve the pending'));
  assert.ok(f.issues.every(issue => !issue.labels.includes('release-review')));
  winget.body += '\nHuman note\n- [x] Reviewed';
  const saved = winget.body;
  await run(f.github, f.context, f.core);
  assert.equal(f.issues.length, 4);
  assert.equal(winget.body, saved);
  assert.equal(winget.state, 'open');
  assert.equal(JSON.parse(f.outputs.tracking_issues).length, 4);
  winget.state = 'closed';
  await assert.rejects(run(f.github, f.context, f.core), /is closed.*reopen/);
  assert.equal(winget.body, saved);
  assert.equal(winget.state, 'closed');
});

test('legacy matching removes only release-review and fails closed on ambiguous or duplicate issues', async () => {
  const legacy = { number: 7, state: 'open', title: '📦 Docker Release: v2.4.0',
    body: '## Release Version: v2.4.0\n\nHuman content', labels: ['docker-release', 'docker-approved', 'release-review'] };
  const run = stepScript('Create tracking issues for downstream registries');
  const f = trackerFixture([legacy]);
  await run(f.github, f.context, f.core);
  assert.equal(f.issues.length, 4);
  assert.deepEqual(f.removed, [7]);
  assert.deepEqual(legacy.labels, ['docker-release', 'docker-approved']);
  assert.equal(legacy.state, 'open');
  assert.match(legacy.body, /Human content/);
  assert.match(legacy.body, /registry-approval-instructions:v1/);
  const saved = legacy.body;
  await run(f.github, f.context, f.core);
  assert.equal(legacy.body, saved);

  const ambiguous = trackerFixture([{ ...legacy, labels: ['release-review'] }]);
  await assert.rejects(run(ambiguous.github, ambiguous.context, ambiguous.core), /Ambiguous/);
  assert.deepEqual(ambiguous.removed, []);

  const duplicate = trackerFixture([legacy, { ...legacy, number: 8 }]);
  await assert.rejects(run(duplicate.github, duplicate.context, duplicate.core), /Multiple/);
});

test('legacy WinGet preserves explicit opt-in and appends guidance only once', async () => {
  const legacy = { number: 10, state: 'open', title: '📦 WinGet Release: v2.4.0',
    body: '## Release Version: v2.4.0\n\n**WinGet prerelease opt-in:** true\n\nHuman evidence',
    labels: ['winget-release', 'release-review', 'winget-approved'] };
  const f = trackerFixture([legacy]);
  const run = stepScript('Create tracking issues for downstream registries');
  await run(f.github, f.context, f.core);
  assert.ok(legacy.body.startsWith('## Release Version: v2.4.0\n\n**WinGet prerelease opt-in:** true\n\nHuman evidence'));
  assert.equal((legacy.body.match(/^\*\*WinGet prerelease opt-in:\*\*/gm) || []).length, 1);
  assert.deepEqual(legacy.labels, ['winget-release', 'winget-approved']);
  const saved = legacy.body;
  await run(f.github, f.context, f.core);
  assert.equal(legacy.body, saved);

  const missing = trackerFixture([{ ...legacy, number: 11,
    body: '## Release Version: v2.4.0\n\nHuman evidence', labels: ['winget-release'] }]);
  await run(missing.github, missing.context, missing.core);
  assert.doesNotMatch(missing.issues[0].body, /^\*\*WinGet prerelease opt-in:\*\*/m);
});

test('marker matches with wrong version or missing labels fail rather than reuse', async () => {
  const run = stepScript('Create tracking issues for downstream registries');
  const issue = { number: 2, state: 'open', title: 'Human title',
    body: '<!-- registry-release:v2.4.0:publish-winget -->\n## Release Version: v2.4.1',
    labels: ['winget-release', 'release-review'] };
  const f = trackerFixture([issue]);
  await assert.rejects(run(f.github, f.context, f.core), /does not identify/);
  issue.body = '<!-- registry-release:v2.4.0:publish-winget -->\n## Release Version: v2.4.0';
  issue.labels = ['release-review'];
  await assert.rejects(run(f.github, f.context, f.core), /lacks the required/);
  assert.deepEqual(f.removed, []);
  issue.labels = ['winget-release'];
  issue.body += '\n## Release Version: v2.4.1';
  await assert.rejects(run(f.github, f.context, f.core), /does not identify/);
});

test('registry creation failures propagate and retry reuses partial results', async () => {
  const run = stepScript('Create tracking issues for downstream registries');
  const f = trackerFixture();
  f.failOn('Docker');
  await assert.rejects(run(f.github, f.context, f.core), /creation failed/);
  assert.equal(f.issues.length, 1);
  assert.equal(f.outputs.tracking_issues, undefined);
  f.failOn('');
  await run(f.github, f.context, f.core);
  assert.equal(f.issues.length, 4);
});

test('mainline accepts exactly one standalone command and rejects ambiguous or substring comments', async () => {
  const run = stepScript('Verify commenter is a maintainer');
  for (const [body, expected] of [
    ['/publish-release', 'approve'], [' /approve-release\n', 'approve'],
    ['@publish-release', 'approve'], ['/hold-release', 'hold'], ['/pause-release', 'hold'],
    ['/reject-release', 'reject'], ['/cancel-release', 'reject'], ['/rollback-release', 'rollback'],
    ['/publish-release-now', null], ['please /publish-release', null],
    ['/publish-release\n/hold-release', null], ['/publish-release /reject-release', null],
  ]) {
    const outputs = {};
    const failures = [];
    const context = { repo: { owner: 'o', repo: 'r' }, payload: { action: 'created',
      issue: { number: 1, state: 'open' }, comment: { body, user: { login: 'maintainer' } },
    } };
    const github = { rest: { repos: { async getCollaboratorPermissionLevel() {
      return { data: { permission: 'maintain' } };
    } } } };
    const core = { setOutput(key, value) { outputs[key] = value; }, setFailed(message) { failures.push(message); }, info() {} };
    await run(github, context, core);
    assert.equal(outputs.status, expected || undefined, body);
    assert.equal(failures.length, expected ? 0 : 1, body);
  }
});

test('closed release review issues cannot execute commands', async () => {
  const run = stepScript('Verify commenter is a maintainer');
  const failures = [];
  await run({}, { repo: {}, payload: { action: 'created', issue: { state: 'closed' },
    comment: { body: '/publish-release', user: { login: 'maintainer' } } } },
  { setFailed(message) { failures.push(message); } });
  assert.match(failures[0], /closed/);
});

test('non-maintainers cannot approve a release', async () => {
  const run = stepScript('Verify commenter is a maintainer');
  const outputs = {};
  const failures = [];
  await run({ rest: { repos: { async getCollaboratorPermissionLevel() {
    return { data: { permission: 'write' } };
  } } } }, { repo: {}, payload: { action: 'created', issue: { state: 'open' },
    comment: { body: '/publish-release', user: { login: 'contributor' } } } },
  { setOutput(key, value) { outputs[key] = value; }, setFailed(message) { failures.push(message); } });
  assert.equal(outputs.status, undefined);
  assert.match(failures[0], /does not have maintainer permissions/);
});

test('only created comments can initiate commands', async () => {
  assert.match(workflow, /issue_comment:\s*\n\s*types: \[created\]/);
  const run = stepScript('Verify commenter is a maintainer');
  const failures = [];
  await run({}, { repo: {}, payload: { action: 'edited', issue: { state: 'open' },
    comment: { body: '/publish-release', user: { login: 'maintainer' } } } },
  { setFailed(message) { failures.push(message); } });
  assert.match(failures[0], /newly created comment/);
});
