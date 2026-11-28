import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

import yaml


ROOT = Path(__file__).resolve().parents[1]
NIGHTLY = yaml.safe_load((ROOT / ".github/workflows/release-nightly.yml").read_text())
MATRIX = yaml.safe_load((ROOT / ".github/workflows/release-build-matrix.yml").read_text())
SHA_EXPR = "${{ github.event.workflow_run.head_sha || github.sha }}"


class NightlyReleaseIdentityTests(unittest.TestCase):
    def plan(self, directory, override):
        script = MATRIX["jobs"]["prepare-release-matrix"]["steps"][0]["run"]
        output = directory / "output"
        result = subprocess.run(
            ["bash", "-euo", "pipefail", "-c", script], cwd=directory,
            env={**os.environ, "GITHUB_OUTPUT": str(output), "BUILD_MATRIX": "all",
                 "OS_SELECTOR": "all", "ARCH_SELECTOR": "all", "EDITION": "community",
                 "BUILD_TYPE": "Release", "PACKAGING": "auto", "DISTRIBUTION": "all",
                 "RUN_NUMBER": "42", "SOURCE_SHA": "a" * 40,
                 "RELEASE_TYPE_OVERRIDE": override},
            capture_output=True, text=True,
        )
        return result, dict(line.split("=", 1) for line in output.read_text().splitlines()) if output.exists() else {}

    def test_override_applies_in_every_lane_and_changes_reuse_identity(self):
        with tempfile.TemporaryDirectory(dir=ROOT) as temp:
            directory = Path(temp)
            normal, original = self.plan(directory, "")
            self.assertEqual(normal.returncode, 0, normal.stderr)
            nightly, changed = self.plan(directory, "nightly")
            self.assertEqual(nightly.returncode, 0, nightly.stderr)
            before = json.loads(original["matrix"])["include"]
            after = json.loads(changed["matrix"])["include"]
            self.assertEqual({lane["id"] for lane in after},
                             {"linux-amd64", "linux-arm64", "windows-amd64", "macos-arm64"})
            for old, lane in zip(before, after):
                with self.subTest(lane=lane["id"]):
                    self.assertNotEqual(old["release_manifest"]["config_id"], lane["release_manifest"]["config_id"])
                    self.assertEqual(lane["release_manifest"]["configuration"]["source_sha"], "a" * 40)
                    self.assertNotIn("nightly > RELEASE_TYPE", old["configure_command"])
                    prefix = lane["configure_command"].split("set -euo pipefail\n", 2)[1]
                    (directory / "VERSION").write_text("2.4.0\n")
                    (directory / "RELEASE_TYPE").write_text("alpha\n")
                    run = subprocess.run(["bash", "-euo", "pipefail", "-c", prefix], cwd=directory)
                    self.assertEqual(run.returncode, 0)
                    self.assertEqual((directory / "RELEASE_TYPE").read_text(), "nightly\n")
                    (directory / "VERSION").write_text("2.4.0-alpha\n")
                    (directory / "RELEASE_TYPE").write_text("alpha\n")
                    run = subprocess.run(["bash", "-euo", "pipefail", "-c", prefix], cwd=directory,
                                         capture_output=True, text=True)
                    self.assertNotEqual(run.returncode, 0)
                    self.assertEqual((directory / "RELEASE_TYPE").read_text(), "alpha\n")
            invalid, _ = self.plan(directory, "stable")
            self.assertNotEqual(invalid.returncode, 0)

    def test_actual_cpack_version_uses_nightly_channel_not_tag_run(self):
        with tempfile.TemporaryDirectory(dir=ROOT) as temp:
            directory = Path(temp)
            (directory / "VERSION").write_text("2.4.0\n")
            (directory / "RELEASE_TYPE").write_text("nightly\n")
            original = (ROOT / "CMakeLists.txt").read_text()
            suffix = original.split("# --- Common metadata ---", 1)[1].split(
                'set(CPACK_PACKAGE_DESCRIPTION_SUMMARY', 1)[0]
            (directory / "CMakeLists.txt").write_text(
                'cmake_minimum_required(VERSION 3.20)\nproject(Nightly NONE)\n'
                f'include("{ROOT / "cmake/Versions.cmake"}")\n'
                + suffix + '\ninstall(FILES VERSION DESTINATION share)\ninclude(CPack)\n'
            )
            result = subprocess.run(["cmake", "-S", str(directory), "-B", str(directory / "build")],
                                    capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            config = (directory / "build/CPackConfig.cmake").read_text()
            self.assertIn('set(CPACK_PACKAGE_VERSION "2.4.0-nightly")', config)
            self.assertNotIn("nightly.2026", config)
            result = subprocess.run(["cpack", "--config", str(directory / "build/CPackConfig.cmake"),
                                     "-G", "TGZ", "-B", str(directory / "packages")],
                                    capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertTrue(any("2.4.0-nightly" in p.name for p in (directory / "packages").iterdir()))

    def test_publication_is_pinned_and_fails_closed(self):
        jobs = NIGHTLY["jobs"]
        self.assertEqual(jobs["build-matrix"]["with"]["source_sha"], SHA_EXPR)
        self.assertEqual(jobs["build-matrix"]["with"]["release_type_override"], "nightly")
        for job in ("check-changes", "set-release-type", "generate-metadata", "publish-release"):
            checkout = next(s for s in jobs[job]["steps"] if s.get("name") == "Checkout")
            self.assertEqual(checkout["with"]["ref"], SHA_EXPR)
        publish = next(s for s in jobs["publish-release"]["steps"] if s.get("id") == "release")
        self.assertIn("inputs.dry_run != true", publish["if"])
        self.assertEqual(publish["env"]["SOURCE_SHA"], SHA_EXPR)
        self.assertEqual(publish["env"]["EXPECTED_LAST_NIGHTLY_TAG"],
                         "${{ needs.check-changes.outputs.last-nightly-tag }}")
        script = publish["with"]["script"]
        harness = r"""
        const AsyncFunction = Object.getPrototypeOf(async function(){}).constructor;
        const calls = [];
        const scenario = process.env.SCENARIO;
        const sha = process.env.SOURCE_SHA;
        const expectedBaseline = process.env.EXPECTED_LAST_NIGHTLY_TAG;
        const fail = status => { const e = new Error('API error'); e.status = status; throw e; };
        const state = {
          ref: scenario === 'mismatched-source'
            ? {type:'commit',sha:'b'.repeat(40)}
            : (['matching-orphan-tag', 'matching-partial-draft', 'upload-failure-retry',
                'non-draft-release', 'unexpected-assets', 'newer-publication-race'].includes(scenario)
              ? {type:'commit',sha} : null),
          release: ['matching-partial-draft', 'non-draft-release', 'unexpected-assets',
                    'newer-publication-race'].includes(scenario)
            ? {id:123,tag_name:process.env.NIGHTLY_TAG,
               draft:scenario !== 'non-draft-release',prerelease:true}
            : null,
          assets: ['matching-partial-draft', 'newer-publication-race'].includes(scenario)
            ? [{name:'build-metadata.json',state:'uploaded',size:2}]
            : (scenario === 'unexpected-assets'
              ? [{name:'unexpected.txt',state:'uploaded',size:1}] : []),
          published: scenario === 'newer-publication-race'
            ? [{tag_name:'v2.4.0-nightly.20261128.41',draft:false,prerelease:true,
                published_at:'2026-11-28T09:00:00Z'}]
            : (expectedBaseline
              ? [{tag_name:expectedBaseline,draft:false,prerelease:true,
                  published_at:'2026-11-27T09:00:00Z'}] : []),
          uploadFailures: scenario === 'upload-failure-retry' ? 1 : 0
        };
        const realRequire = require;
        const child = realRequire('child_process');
        const listReleases = async () => {};
        const listAssets = async () => {};
        const fakeRequire = name => name === 'child_process' ? {
          execFileSync: (program, args, options) => {
            if (program !== 'gh') return child.execFileSync(program, args, options);
            if (!process.env.GH_TOKEN) throw new Error('GH_TOKEN is required for gh uploads');
            if (!args.includes('--clobber')) throw new Error('Expected uploads to use --clobber');
            const file = args[3];
            const asset = realRequire('path').basename(file);
            calls.push('upload:' + asset);
            if (state.uploadFailures > 0 && asset === 'package.tgz') {
              state.uploadFailures -= 1;
              fail(500);
            }
            const uploaded = {name:asset,state:'uploaded',size:realRequire('fs').statSync(file).size};
            state.assets = state.assets.filter(existing => existing.name !== asset);
            state.assets.push(uploaded);
          }
        } : realRequire(name);
        const github = {rest: {repos: {
          getReleaseByTag: async () => {
            calls.push('getRelease');
            if (!state.release) fail(404);
            return {data:state.release};
          },
          createRelease: async x => {
            calls.push('createRelease:' + x.target_commitish + ':' + x.draft);
            state.release = {id:123,tag_name:x.tag_name,draft:x.draft,prerelease:x.prerelease};
            return {data:state.release};
          },
          listReleases,
          listReleaseAssets: listAssets,
          getRelease: async () => {calls.push('getDraft'); return {data:state.release};},
          updateRelease: async x => {
            calls.push('publish:' + x.draft);
            state.release.draft = x.draft;
          }
        }, git: {
          createRef: async x => {
            calls.push('createRef:' + x.sha);
            if (state.ref) fail(422);
            state.ref = {type:'commit',sha:x.sha};
          },
          getRef: async () => {
            calls.push('getRef');
            if (!state.ref) fail(404);
            return {data:{object:state.ref}};
          }
        }}, paginate: async method => {
          if (method === listReleases) {
            calls.push('listPublishedReleases');
            return state.published;
          }
          calls.push('listAssets');
          return state.assets;
        }};
        const publish = new AsyncFunction('github', 'context', 'require', process.env.SCRIPT);
        const run = () => publish(
          github, {repo:{owner:'owner',repo:'repo'}}, fakeRequire
        );
        (async () => {
          try {
            await run();
            if (scenario === 'upload-failure-retry') throw new Error('first upload unexpectedly succeeded');
            console.log(JSON.stringify({ok:true,calls,releaseDraft:state.release?.draft,ref:state.ref}));
          } catch (error) {
            if (scenario !== 'upload-failure-retry' || error.message !== 'API error') {
              console.log(JSON.stringify({ok:false,calls,error:error.message,
                releaseDraft:state.release?.draft,ref:state.ref}));
              return;
            }
            const firstAttemptCalls = [...calls];
            try {
              await run();
              console.log(JSON.stringify({ok:true,calls,firstAttemptCalls,
                releaseDraft:state.release?.draft,ref:state.ref}));
            } catch (retryError) {
              console.log(JSON.stringify({ok:false,calls,firstAttemptCalls,error:retryError.message,
                releaseDraft:state.release?.draft,ref:state.ref}));
            }
          }
        })();
        """
        with tempfile.TemporaryDirectory(dir=ROOT) as temp:
            directory = Path(temp)
            subprocess.run(["git", "init", "-q", str(directory)], check=True)
            subprocess.run(["git", "-C", str(directory), "-c", "user.name=Test", "-c",
                            "user.email=test@example.com", "commit", "-q", "--allow-empty", "-m", "test"], check=True)
            sha = subprocess.check_output(["git", "-C", str(directory), "rev-parse", "HEAD"], text=True).strip()
            (directory / "release-artifacts").mkdir()
            (directory / "release-artifacts/package.tgz").write_text("data")
            (directory / "release-notes.md").write_text("notes")
            for scenario in ("success", "wrong-source", "wrong-metadata", "matching-orphan-tag",
                             "matching-partial-draft", "mismatched-source", "non-draft-release",
                             "unexpected-assets", "upload-failure-retry", "newer-publication-race"):
                with self.subTest(scenario=scenario):
                    (directory / "build-metadata.json").write_text(json.dumps({
                        "git": {"commit": "b" * 40 if scenario == "wrong-metadata" else sha},
                        "version": {"version": "2.4.0", "release_type": "nightly"},
                    }))
                    result = subprocess.run(["node", "-e", harness], cwd=directory, capture_output=True, text=True,
                                            env={**os.environ, "SCRIPT": script, "SCENARIO": scenario,
                                                 "SOURCE_SHA": "b" * 40 if scenario == "wrong-source" else sha,
                                                 "GH_TOKEN": "test-token",
                                                 "EXPECTED_LAST_NIGHTLY_TAG":
                                                     "" if scenario in ("success", "newer-publication-race")
                                                     else "v2.4.0-nightly.20261127.1",
                                                 "NIGHTLY_TAG": "v2.4.0-nightly.20261128.42",
                                                 "BUILD_VERSION": "2.4.0"})
                    self.assertEqual(result.returncode, 0, result.stderr)
                    data = json.loads(result.stdout)
                    expected_success = scenario in ("success", "matching-orphan-tag",
                                                    "matching-partial-draft", "upload-failure-retry")
                    self.assertEqual(data["ok"], expected_success, data)
                    if scenario == "success":
                        self.assertIn("createRef:" + sha, data["calls"])
                        self.assertIn("createRelease:" + sha + ":true", data["calls"])
                        self.assertEqual(data["calls"].count("publish:false"), 1)
                    if scenario == "matching-orphan-tag":
                        self.assertNotIn("createRef:" + sha, data["calls"])
                        self.assertIn("createRelease:" + sha + ":true", data["calls"])
                    if scenario == "matching-partial-draft":
                        self.assertNotIn("createRelease:" + sha + ":true", data["calls"])
                        self.assertEqual(data["calls"].count("listAssets"), 2)
                        self.assertEqual(data["calls"].count("publish:false"), 1)
                    if scenario == "upload-failure-retry":
                        self.assertNotIn("publish:false", data["firstAttemptCalls"])
                        self.assertEqual(data["calls"].count("getRelease"), 2)
                        self.assertEqual(data["calls"].count("publish:false"), 1)
                    if scenario in ("mismatched-source", "non-draft-release", "unexpected-assets"):
                        self.assertNotIn("publish:false", data["calls"])
                        self.assertFalse(any(c.startswith("upload:") for c in data["calls"]))
                    if scenario in ("wrong-source", "wrong-metadata"):
                        self.assertNotIn("publish:false", data["calls"])
                    if scenario == "newer-publication-race":
                        self.assertNotIn("publish:false", data["calls"])
                        self.assertIn("listPublishedReleases", data["calls"])
                        self.assertTrue(data["releaseDraft"])
                        self.assertEqual(data["ref"]["sha"], sha)

    def test_release_notes_preserve_special_commit_subjects(self):
        script = next(s["run"] for s in NIGHTLY["jobs"]["publish-release"]["steps"]
                      if s.get("id") == "release-notes")
        with tempfile.TemporaryDirectory(dir=ROOT) as temp:
            directory = Path(temp)
            subprocess.run(["git", "init", "-q", str(directory)], check=True)
            subject = r'''pipes | ampersand & slash \ quotes "' '''
            subprocess.run(["git", "-C", str(directory), "-c", "user.name=Test", "-c",
                            "user.email=test@example.com", "commit", "-q", "--allow-empty", "-m", subject], check=True)
            (directory / "build-metadata.json").write_text('{"release":"nightly"}\n')
            result = subprocess.run(["bash", "-euo", "pipefail", "-c", script], cwd=directory,
                                    env={**os.environ, "LAST_NIGHTLY": "", "SOURCE_BRANCH": "develop",
                                         "GITHUB_RUN_NUMBER": "42", "GITHUB_SERVER_URL": "https://github.com",
                                         "GITHUB_REPOSITORY": "owner/repo"}, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn(subject.strip(), (directory / "release-notes.md").read_text())

    def test_tracker_and_downstream_are_guarded(self):
        jobs = NIGHTLY["jobs"]
        pending = jobs["mark-release-pending"]
        self.assertEqual(pending["needs"], "check-changes")
        for token in ("needs.check-changes.result == 'success'",
                      "needs.check-changes.outputs.has-changes == 'true'",
                      "github.event.workflow_run.event == 'schedule'", "inputs.dry_run != true"):
            self.assertIn(token, pending["if"])
        final = jobs["finalize-release-status"]
        for token in ("needs.check-changes.result == 'failure'",
                      "needs.check-changes.result == 'success'",
                      "needs.check-changes.outputs.has-changes == 'true'"):
            self.assertIn(token, final["if"])
        status = next(s for s in final["steps"] if s.get("id") == "status")
        self.assertIn('TRACKING_RESULT', status["run"])
        tracking = jobs["track-downstream"]
        self.assertIn("needs.publish-release.result == 'success'", tracking["if"])
        self.assertIn("inputs.dry_run != true", tracking["if"])
        self.assertEqual(tracking["permissions"], {"contents": "read", "issues": "write"})
        self.assertEqual(tracking["steps"][0]["with"]["ref"], "${{ github.workflow_sha }}")
        self.assertEqual(tracking["steps"][1]["env"]["NIGHTLY_TAG"],
                         "${{ needs.generate-metadata.outputs.nightly-tag }}")
        self.assertIn("track-downstream", jobs["run-summary"]["needs"])
        self.assertIn("track-downstream", final["needs"])
        self.assertIn("mark-release-pending", final["needs"])
        docker_steps = jobs["publish-docker"]["steps"]
        channel = next(i for i, s in enumerate(docker_steps) if s.get("name") == "Set nightly Docker build channel")
        build = next(i for i, s in enumerate(docker_steps) if s.get("name") == "Build staged Docker runtime candidate")
        self.assertLess(channel, build)
        self.assertIn("nightly > RELEASE_TYPE", docker_steps[channel]["run"])

    def test_manifest_promotion_requires_published_release_and_validated_candidates(self):
        jobs = NIGHTLY["jobs"]
        promotion = jobs["publish-docker-manifest"]
        self.assertIn("publish-release", promotion["needs"])
        self.assertNotIn("publish-release", jobs["publish-docker"]["needs"])
        expression = promotion["if"]
        for release, docker, metadata, event, dry_run, cancelled, expected in (
            ("success", "success", "success", "workflow_run", False, False, True),
            ("failure", "success", "success", "workflow_run", False, False, False),
            ("skipped", "success", "success", "workflow_run", False, False, False),
            ("success", "failure", "success", "workflow_run", False, False, False),
            ("success", "success", "failure", "workflow_run", False, False, False),
            ("success", "success", "success", "workflow_dispatch", True, False, False),
            ("success", "success", "success", "workflow_dispatch", False, False, True),
            ("success", "success", "success", "workflow_run", False, True, False),
        ):
            with self.subTest(release=release, docker=docker, metadata=metadata,
                              event=event, dry_run=dry_run, cancelled=cancelled):
                condition = expression
                for name, value in {
                    "always()": True,
                    "cancelled()": cancelled,
                    "needs.publish-release.result": release,
                    "needs.publish-docker.result": docker,
                    "needs.generate-metadata.result": metadata,
                    "github.event_name": event,
                    "inputs.dry_run": dry_run,
                    "true": True,
                }.items():
                    condition = condition.replace(name, repr(value))
                condition = condition.replace("&&", " and ").replace("||", " or ")
                condition = condition.replace("!=", " != ").replace("!", " not ")
                # Restore the inequality operator after replacing logical negation.
                condition = condition.replace(" not =", " != ")
                self.assertEqual(eval(condition.strip(), {"__builtins__": {}}), expected)

    def test_change_gate_uses_only_published_ancestor_releases(self):
        script = next(s["run"] for s in NIGHTLY["jobs"]["check-changes"]["steps"]
                      if s.get("id") == "check")
        with tempfile.TemporaryDirectory(dir=ROOT) as temp:
            directory = Path(temp)
            subprocess.run(["git", "init", "-q", str(directory)], check=True)
            git = ["git", "-C", str(directory), "-c", "user.name=Test",
                   "-c", "user.email=test@example.com"]
            subprocess.run(git + ["commit", "-q", "--allow-empty", "-m", "published"], check=True)
            subprocess.run(git + ["tag", "v2.4.0-nightly.20261127.1"], check=True)
            subprocess.run(git + ["tag", "v2.4.0-nightly.20261127"], check=True)
            subprocess.run(git + ["commit", "-q", "--allow-empty", "-m", "failed draft"], check=True)
            subprocess.run(git + ["tag", "v2.4.0-nightly.20261128.2"], check=True)
            tools = directory / "tools"
            tools.mkdir()
            gh = tools / "gh"
            gh.write_text('#!/bin/sh\n[ "${GH_FAIL:-}" != yes ] || exit 1\nprintf "%s\\n" "$PUBLISHED"\n')
            gh.chmod(0o755)
            output = directory / "output"
            for published, expected_tag, changes in (
                ("v2.4.0-nightly.20261127.1", "v2.4.0-nightly.20261127.1", "true"),
                ("v2.4.0-nightly.20261127", "v2.4.0-nightly.20261127", "true"),
                ("", "", "true"),
                ("v2.4.0-nightly.20261128.2", "v2.4.0-nightly.20261128.2", "false"),
            ):
                with self.subTest(published=published):
                    output.write_text("")
                    result = subprocess.run(["bash", "-euo", "pipefail", "-c", script], cwd=directory,
                                            env={**os.environ, "PATH": f"{tools}:{os.environ['PATH']}",
                                                 "GITHUB_OUTPUT": str(output), "GITHUB_REPOSITORY": "owner/repo",
                                                 "PUBLISHED": published}, capture_output=True, text=True)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    values = dict(line.split("=", 1) for line in output.read_text().splitlines())
                    self.assertEqual(values["last-nightly-tag"], expected_tag)
                    self.assertEqual(values["has-changes"], changes)
            result = subprocess.run(["bash", "-euo", "pipefail", "-c", script], cwd=directory,
                                    env={**os.environ, "PATH": f"{tools}:{os.environ['PATH']}",
                                         "GITHUB_OUTPUT": str(output), "GITHUB_REPOSITORY": "owner/repo",
                                         "GH_FAIL": "yes"}, capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
        self.assertIn(".draft == false", script)
        self.assertIn(".prerelease == true", script)
        self.assertIn("--paginate", script)

    @unittest.skipUnless(shutil.which("jq"), "requires jq")
    def test_baseline_query_excludes_drafts_and_non_prereleases(self):
        script = next(s["run"] for s in NIGHTLY["jobs"]["check-changes"]["steps"]
                      if s.get("id") == "check")
        query = script.split("--jq \\\n", 1)[1].split("'", 2)[1]
        result = subprocess.run(
            ["jq", "-r", query], input=json.dumps([
                {"tag_name": "v2.4.0-nightly.20261125.1", "draft": False, "prerelease": True},
                {"tag_name": "v2.4.0-nightly.20261126.2", "draft": True, "prerelease": True},
                {"tag_name": "v2.4.0-nightly.20261127.3", "draft": False, "prerelease": False},
            ]), capture_output=True, text=True,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.strip(), "v2.4.0-nightly.20261125.1")


if __name__ == "__main__":
    unittest.main()
