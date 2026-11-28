// Called only after successful, non-dry-run GitHub Release publication.
// The caller must serialize reconciliation for a given tag across all creators.
const registries = [
  { name: 'WinGet', keyword: 'publish-winget', label: 'winget-release', distro: 'winget', environment: 'winget-production', approval: 'release-winget-approval.yml', workflow: 'release-winget.yml' },
  { name: 'Docker', keyword: 'publish-docker', label: 'docker-release', distro: 'docker', environment: 'docker-production', approval: 'release-docker-approval.yml', workflow: 'release-docker-image.yml' },
];

// Distro approval wrappers currently dispatch to stable, so nightly issues must
// neither target those wrappers nor carry their approval labels.
const approvalLabels = ['winget-release', 'docker-release', 'linux-distro-review', 'windows-distro-review'];

const labelNames = issue => (issue.labels || []).map(label => typeof label === 'string' ? label : label.name);

function hasExactVersion(issue, tag) {
  const body = issue.body || '';
  const headings = body.split(/\r?\n/).filter(line => /^\s*##\s+Release Version:/i.test(line));
  // The approval gate searches the entire body, not just line-start headings.
  const gateMatches = [...body.matchAll(/##\s+Release Version:\s+v?(\d+\.\d+\.\d+(?:-[0-9A-Za-z][0-9A-Za-z.-]*)?)(?:\s|$)/g)];
  return headings.length === 1 && headings[0] === `## Release Version: ${tag}` &&
    gateMatches.length === 1 && gateMatches[0][1] === tag.slice(1);
}

function issueBody(registry, tag, releaseUrl) {
  const optIn = registry.name === 'WinGet'
    ? '**WinGet prerelease opt-in:** false\n\nSet this field to `true` only after explicitly reviewing and approving this prerelease for WinGet.\n\n'
    : '';
  const nightlyDocker = registry.name === 'Docker'
    ? 'The separate nightly Docker pipeline may already publish nightly tags; this issue tracks a distinct, human-approved downstream publication. Check existing runs and tags before requesting it.\n\n'
    : '';
  const verification = registry.name === 'WinGet'
    ? 'A dispatch or draft PR is not publication. Verify upstream review, merge, package availability and installation separately; upstream acceptance is not guaranteed.'
    : 'A dispatch is not publication. Verify the target channel, package or image, and installation independently.';
  return `<!-- registry-release:${tag}:${registry.keyword} -->
## Release Version: ${tag}

**Registry:** ${registry.name}
**GitHub Release:** [${tag}](${releaseUrl})
**Status:** Awaiting human review

${optIn}${nightlyDocker}<!-- registry-approval-instructions:v1 -->
### Manual approval and verification
1. Review the release artifacts, checksums, signatures, metadata, target channel and any packaging or security blockers. Do not approve if validation fails.
2. An admin or maintainer may post the standalone comment \`/${registry.keyword}\` on this issue to start \`${registry.approval}\`.
3. Approve the pending \`${registry.environment}\` GitHub Environment only after review.
4. Monitor \`${registry.workflow}\` and check its outcome. ${verification}

Do not treat this tracking issue as an automatic publication or upstream PR claim. Leave it open and resolve blockers if validation or publication fails. Check existing runs before retrying; repeated approval comments can dispatch multiple runs.`;
}

module.exports = async function trackNightlyDownstream({ github, context, core, tag }) {
  if (typeof tag !== 'string' || !/^v\d+\.\d+\.\d+-nightly\.\d{8}\.\d+$/.test(tag)) {
    throw new Error('Expected an exact nightly tag vX.Y.Z-nightly.YYYYMMDD.RUN.');
  }
  const repo = { owner: context.repo.owner, repo: context.repo.repo };
  const release = (await github.rest.repos.getReleaseByTag({ ...repo, tag })).data;
  if (release.tag_name !== tag || release.draft !== false || release.prerelease !== true) {
    throw new Error(`Expected a published GitHub prerelease for ${tag}.`);
  }
  const releaseUrl = `https://github.com/${encodeURIComponent(repo.owner)}/${encodeURIComponent(repo.repo)}/releases/tag/${encodeURIComponent(tag)}`;
  const issues = await github.paginate(github.rest.issues.listForRepo, { ...repo, state: 'all' });
  const plans = registries.map(registry => {
    const title = `📦 ${registry.name} Release: ${tag}`;
    const marker = `<!-- registry-release:${tag}:${registry.keyword} -->`;
    const candidates = issues.filter(issue => !issue.pull_request && issue.title === title);
    const legacy = candidates.filter(issue => labelNames(issue).includes(registry.label) && hasExactVersion(issue, tag));
    if (candidates.some(issue => !(issue.body || '').includes(marker) && !legacy.includes(issue))) {
      throw new Error(`Ambiguous ${registry.name} tracking issue for ${tag}; resolve it before retrying.`);
    }
    const matches = issues.filter(issue => !issue.pull_request && ((issue.body || '').includes(marker) || legacy.includes(issue)));
    if (matches.length > 1) throw new Error(`Multiple ${registry.name} tracking issues for ${tag}; resolve duplicates before retrying.`);
    const issue = matches[0];
    if (issue) validateIssue(issue, registry, tag);
    return { registry, title, issue };
  });
  const result = [];
  for (const { registry, title, issue } of plans) {
    let number;
    if (issue) {
      const current = (await github.rest.issues.get({ ...repo, issue_number: issue.number })).data;
      validateIssue(current, registry, tag);
      if (!(current.body || '').includes(`<!-- registry-release:${tag}:${registry.keyword} -->`) && current.title !== title) {
        throw new Error(`${registry.name} tracking issue #${issue.number} changed during reconciliation.`);
      }
      number = issue.number;
      core.info(`Reusing ${registry.name} tracking issue #${number}`);
    } else {
      const created = await github.rest.issues.create({
        ...repo, title, body: issueBody(registry, tag, releaseUrl), labels: [registry.label, registry.distro],
      });
      number = created.data.number;
      core.info(`Created ${registry.name} tracking issue #${number}`);
    }
    result.push({ name: registry.name, number, keyword: registry.keyword });
  }
  return result;
};

function validateIssue(issue, registry, tag) {
  if (issue.pull_request) throw new Error(`${registry.name} tracking issue #${issue.number} is a pull request.`);
  if (issue.state !== 'open') throw new Error(`${registry.name} tracking issue #${issue.number} is closed; review its outcome before reopening.`);
  const labels = labelNames(issue);
  if (!labels.includes(registry.label)) throw new Error(`${registry.name} tracking issue #${issue.number} lacks the required ${registry.label} label.`);
  if (labels.includes('release-review')) throw new Error(`${registry.name} tracking issue #${issue.number} has the release-review label; resolve the label conflict.`);
  if (approvalLabels.some(label => label !== registry.label && labels.includes(label))) {
    throw new Error(`${registry.name} tracking issue #${issue.number} has another registry approval label; resolve the label conflict.`);
  }
  if (!hasExactVersion(issue, tag)) throw new Error(`${registry.name} tracking issue #${issue.number} does not identify exactly ${tag}.`);
}
