const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const { test } = require('node:test');
const stage = require('../.github/scripts/stage-github-release.js');

function fixture(options = {}) {
  const releaseDir = '__release_fixture__';
  const files = new Map(['package.zip', 'SHA256SUMS.txt', 'RELEASE_MANIFEST.txt']
    .map(file => [file, Buffer.from('payload')]));
  const originalFs = { readdirSync: fs.readdirSync, statSync: fs.statSync, readFileSync: fs.readFileSync };
  fs.readdirSync = dir => dir === releaseDir ? [...files.keys()] : originalFs.readdirSync(dir);
  fs.statSync = file => file.startsWith(`${releaseDir}${path.sep}`)
    ? { isFile: () => true, size: files.get(path.basename(file)).length } : originalFs.statSync(file);
  fs.readFileSync = (...args) => typeof args[0] === 'string' && args[0].startsWith(`${releaseDir}${path.sep}`)
    ? files.get(path.basename(args[0])) : originalFs.readFileSync(...args);
  const uploads = [];
  const created = [];
  const issues = [];
  const comments = [];
  const assets = [];
  const repos = {
    async getReleaseByTag() {
      if (!options.existing) throw Object.assign(new Error('Not found'), { status: 404 });
      return { data: { id: 1, draft: !options.published, html_url: 'https://example.invalid/release' } };
    },
    async createRelease(data) {
      created.push(data);
      return { data: { id: 1, draft: data.draft, html_url: 'https://example.invalid/release' } };
    },
    async listReleaseAssets() { return assets; },
    async deleteReleaseAsset() {},
    async uploadReleaseAsset(data) {
      uploads.push(data);
      if (options.uploadFailure) throw new Error('Upload failed');
      if (!options.missingAssets) assets.push({ name: data.name, state: 'uploaded', size: data.data.length });
    },
  };
  const github = {
    rest: {
      repos,
      issues: {
        async listForRepo() { return options.existingIssues || []; },
        async create(data) { issues.push(data); },
        async createComment(data) { comments.push(data); },
      },
    },
    async paginate(method, args) { return method(args); },
  };
  return {
    releaseDir, uploads, created, issues, comments, files,
    args: { github, context: { repo: { owner: 'test', repo: 'test' } },
      core: { setOutput() {} }, tag: 'v2.4.0', sourceSha: 'a'.repeat(40), releaseDir },
    cleanup() { Object.assign(fs, originalFs); },
  };
}

test('complete assets are staged as draft and receive an approval issue', async () => {
  const f = fixture();
  try {
    await stage(f.args);
    assert.equal(f.created[0].draft, true);
    assert.equal(f.uploads.length, 3);
    assert.deepEqual(f.issues[0].labels, ['release-review']);
    assert.match(f.issues[0].body, /## Release Version: v2\.4\.0/);
    assert.match(f.issues[0].body, /- \[ \] Review unresolved security/);
  } finally { f.cleanup(); }
});

test('missing tag or package files fails before creating a release', async () => {
  for (const mode of ['tag', 'source', 'package', 'checksum', 'empty']) {
    const f = fixture();
    try {
      if (mode === 'tag') f.args.tag = '';
      if (mode === 'source') f.args.sourceSha = '';
      if (mode === 'package') f.files.delete('package.zip');
      if (mode === 'checksum') f.files.delete('SHA256SUMS.txt');
      if (mode === 'empty') f.files.set('package.zip', Buffer.alloc(0));
      await assert.rejects(stage(f.args));
      assert.equal(f.created.length, 0);
      assert.equal(f.issues.length, 0);
    } finally { f.cleanup(); }
  }
});

test('restaging preserves human review content and warns that approval must be renewed', async () => {
  const issue = { number: 42, state: 'open', body: '<!-- release-review:v2.4.0 -->\n- [x] Human review' };
  const f = fixture({ existing: true, existingIssues: [issue] });
  try {
    await stage(f.args);
    assert.equal(f.issues.length, 0);
    assert.equal(issue.body, '<!-- release-review:v2.4.0 -->\n- [x] Human review');
    assert.equal(f.comments[0].issue_number, 42);
    assert.match(f.comments[0].body, /fresh approval/);
  } finally { f.cleanup(); }
});

test('closed or duplicate review issues fail before overwriting assets', async () => {
  const issue = { number: 42, state: 'closed', body: '<!-- release-review:v2.4.0 -->' };
  for (const existingIssues of [[issue], [{ ...issue, state: 'open' }, { ...issue, number: 43 }]]) {
    const f = fixture({ existing: true, existingIssues });
    try {
      await assert.rejects(stage(f.args), /closed|Multiple/);
      assert.equal(f.uploads.length, 0);
    } finally { f.cleanup(); }
  }
});

test('upload and verification errors fail without creating an approval issue', async () => {
  for (const options of [{ uploadFailure: true }, { missingAssets: true }]) {
    const f = fixture(options);
    try {
      await assert.rejects(stage(f.args));
      assert.equal(f.issues.length, 0);
    } finally { f.cleanup(); }
  }
});

test('published releases are never overwritten', async () => {
  const f = fixture({ existing: true, published: true });
  try {
    await assert.rejects(stage(f.args), /overwrite published release/);
    assert.equal(f.uploads.length, 0);
  } finally { f.cleanup(); }
});
