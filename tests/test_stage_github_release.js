const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const { test } = require('node:test');
const stage = require('../.github/scripts/stage-github-release.js');

function fixture(options = {}) {
  const releaseDir = fs.mkdtempSync(path.join(os.tmpdir(), 'release-stage-'));
  for (const file of ['package.zip', 'SHA256SUMS.txt', 'RELEASE_MANIFEST.txt']) {
    fs.writeFileSync(path.join(releaseDir, file), 'payload');
  }
  const uploads = [];
  const created = [];
  const issues = [];
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
        async listForRepo() { return []; },
        async create(data) { issues.push(data); },
      },
    },
    async paginate(method, args) { return method(args); },
  };
  return {
    releaseDir, uploads, created, issues,
    args: { github, context: { repo: { owner: 'test', repo: 'test' } },
      core: { setOutput() {} }, tag: 'v2.4.0', sourceSha: 'a'.repeat(40), releaseDir },
    cleanup() { fs.rmSync(releaseDir, { recursive: true }); },
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
  } finally { f.cleanup(); }
});

test('missing tag or package files fails before creating a release', async () => {
  for (const mode of ['tag', 'source', 'package', 'checksum', 'empty']) {
    const f = fixture();
    try {
      if (mode === 'tag') f.args.tag = '';
      if (mode === 'source') f.args.sourceSha = '';
      if (mode === 'package') fs.unlinkSync(path.join(f.releaseDir, 'package.zip'));
      if (mode === 'checksum') fs.unlinkSync(path.join(f.releaseDir, 'SHA256SUMS.txt'));
      if (mode === 'empty') fs.writeFileSync(path.join(f.releaseDir, 'package.zip'), '');
      await assert.rejects(stage(f.args));
      assert.equal(f.created.length, 0);
      assert.equal(f.issues.length, 0);
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
