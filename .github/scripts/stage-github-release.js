const fs = require('fs');
const path = require('path');

module.exports = async ({ github, context, core, tag, sourceSha = process.env.RELEASE_SHA, releaseDir = 'release' }) => {
  if (!/^v\d+\.\d+\.\d+(?:-[0-9A-Za-z][0-9A-Za-z.-]*)?$/.test(tag || '')) {
    throw new Error('A canonical community release tag is required.');
  }
  if (!/^[0-9a-f]{40}$/.test(sourceSha || '')) {
    throw new Error('A resolved release source commit is required.');
  }
  const files = fs.readdirSync(releaseDir).filter(file =>
    ['.tar.gz', '.tgz', '.zip', '.deb', '.rpm', '.msi', '.txt'].some(ext => file.endsWith(ext))
  );
  if (!files.some(file => /\.(tar\.gz|tgz|zip|deb|rpm|msi)$/.test(file)) ||
      !files.includes('SHA256SUMS.txt') || !files.includes('RELEASE_MANIFEST.txt')) {
    throw new Error('Release packages, SHA256SUMS.txt and RELEASE_MANIFEST.txt are required.');
  }
  for (const file of files) {
    const stat = fs.statSync(path.join(releaseDir, file));
    if (!stat.isFile() || stat.size === 0) {
      throw new Error(`Release asset is empty or not a file: ${file}`);
    }
  }
  const repo = { owner: context.repo.owner, repo: context.repo.repo };
  let release;
  try {
    release = (await github.rest.repos.getReleaseByTag({ ...repo, tag })).data;
  } catch (error) {
    if (error.status !== 404) throw error;
    release = (await github.rest.repos.createRelease({
      ...repo,
      tag_name: tag,
      target_commitish: sourceSha,
      name: `ThemisDB ${tag}`,
      draft: true,
      prerelease: tag.includes('-'),
      generate_release_notes: true,
    })).data;
  }
  if (!release.draft) {
    throw new Error(`Refusing to overwrite published release ${tag}.`);
  }
  const marker = `<!-- release-review:${tag} -->`;
  const issues = await github.paginate(github.rest.issues.listForRepo, { ...repo, state: 'all' });
  const reviewIssues = issues.filter(issue => !issue.pull_request && (issue.body || '').includes(marker));
  if (reviewIssues.length > 1) {
    throw new Error(`Multiple release review issues found for ${tag}; resolve duplicates before staging.`);
  }
  const existingIssue = reviewIssues[0];
  if (existingIssue && existingIssue.state === 'closed') {
    throw new Error(`Release review issue #${existingIssue.number} is closed; resolve it before staging ${tag}.`);
  }
  const assets = await github.paginate(github.rest.repos.listReleaseAssets, {
    ...repo, release_id: release.id,
  });
  for (const file of files) {
    const existing = assets.find(asset => asset.name === file);
    if (existing) {
      await github.rest.repos.deleteReleaseAsset({ ...repo, asset_id: existing.id });
    }
    await github.rest.repos.uploadReleaseAsset({
      ...repo, release_id: release.id, name: file,
      data: fs.readFileSync(path.join(releaseDir, file)),
    });
  }
  const uploaded = await github.paginate(github.rest.repos.listReleaseAssets, {
    ...repo, release_id: release.id,
  });
  for (const file of files) {
    if (!uploaded.some(asset => asset.name === file && asset.state === 'uploaded' &&
        asset.size === fs.statSync(path.join(releaseDir, file)).size)) {
      throw new Error(`Release asset verification failed: ${file}`);
    }
  }
  const body = `${marker}\n## Release Version: ${tag}\n\n` +
    `Draft release: ${release.html_url}\n\n` +
    `Source commit: ${sourceSha}\n\n` +
    `All ${files.length} assets uploaded and verified for presence and size. ` +
    'Complete the review below before publication; asset upload verification is not a security or quality approval.\n\n' +
    '### Release review checklist\n' +
    '- [ ] Confirm the source commit, tag, build and test evidence match this release\n' +
    '- [ ] Download all expected platform packages and verify SHA256SUMS.txt\n' +
    '- [ ] Verify available signatures, signing identity and release manifest\n' +
    '- [ ] Inspect release notes, changelog, version and package metadata\n' +
    '- [ ] Smoke-test representative packages on supported platforms\n' +
    '- [ ] Review unresolved security, regression and release blockers\n' +
    '- [ ] Confirm rollback and downstream publication readiness\n\n' +
    'After review, a repository admin or maintainer may post the standalone comment `/publish-release`. ' +
    'Docker and other registries require their own approval after GitHub publication.';
  if (existingIssue) {
    await github.rest.issues.createComment({
      ...repo, issue_number: existingIssue.number,
      body: `Draft ${tag} was restaged from ${sourceSha}; ${files.length} assets were uploaded and verified for presence and size. ` +
        'Previous checklist results and approvals may no longer apply: review the updated draft, checksums and signatures again before posting a fresh approval.',
    });
  } else {
    await github.rest.issues.create({
      ...repo, title: `Release review: ${tag}`, body, labels: ['release-review'],
    });
  }
  core.setOutput('tag', tag);
  core.setOutput('release_id', String(release.id));
};
