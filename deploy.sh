#!/usr/bin/env bash
# Deploy the sim to GitHub Pages (bittlex-sim.petoi.com).
#
# Pages serves docs/ on master, so a deploy is: production build into docs/,
# commit it as "build", push to origin/master.
#
# Usage: ./deploy.sh
set -euo pipefail

readonly BRANCH="master"
readonly REMOTE="origin"

cd "$(dirname "$0")"

die() {
  echo "deploy: $*" >&2
  exit 1
}

# --- preflight -------------------------------------------------------------

current_branch="$(git rev-parse --abbrev-ref HEAD)"
[[ "$current_branch" == "$BRANCH" ]] || die "on '$current_branch', switch to '$BRANCH' first"

# The build commit should contain only the build output, so start clean.
if [[ -n "$(git status --porcelain)" ]]; then
  git status --short >&2
  die "working tree is not clean; commit or stash first"
fi

# A dev server (`npm run start` / `npm run prod`) watches the sources and reruns
# wasm-pack into the same ../pkg the build uses; the two clobber each other and
# the build can bundle a half-written or stale wasm. Refuse while one is running.
project_dir="$(pwd -P)"
for pid in $(pgrep -f 'rspack|wasm-pack' || true); do
  cwd="$(lsof -a -p "$pid" -d cwd -Fn 2>/dev/null | sed -n 's/^n//p')"
  if [[ "$cwd" == "$project_dir" || "$cwd" == "$project_dir"/* ]]; then
    ps -o pid=,command= -p "$pid" >&2
    die "a dev server or wasm-pack build is running in this project; stop it first"
  fi
done

git fetch --quiet "$REMOTE" "$BRANCH"
if ! git merge-base --is-ancestor "$REMOTE/$BRANCH" HEAD; then
  die "local $BRANCH is behind $REMOTE/$BRANCH; pull first"
fi

# --- build -----------------------------------------------------------------

echo "==> building into docs/"
(cd www && npm run build)

# The build wipes docs/ and recreates it from www/static; make sure the custom
# domain survived, or Pages drops it.
[[ -f docs/CNAME ]] || die "docs/CNAME missing after build"

# Anything changed outside docs/ means the build touched source (e.g. a
# rewritten package-lock.json); stop rather than ship it silently.
outside_docs="$(git status --porcelain -- . ':(exclude)docs')"
if [[ -n "$outside_docs" ]]; then
  echo "$outside_docs" >&2
  die "build changed files outside docs/; review them before deploying"
fi

# --- commit + push ---------------------------------------------------------

git add -A docs
if git diff --cached --quiet; then
  echo "==> build output unchanged, nothing to deploy"
  exit 0
fi

git diff --cached --stat | tail -n 1
git commit --quiet -m "build"
git push "$REMOTE" "$BRANCH"

echo "==> deployed $(git rev-parse --short HEAD) to $REMOTE/$BRANCH"
