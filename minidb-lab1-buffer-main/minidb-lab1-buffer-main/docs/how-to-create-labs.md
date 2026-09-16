# How to create MiniDB lab repos

Instructor playbook for this org repo (`UM6P-CC/minidb`).  
Students never work in this repo. Each lab becomes its **own** GitHub repo cut from a handout branch.

## Where things live

| Thing | Location |
|-------|----------|
| This guide | `docs/how-to-create-labs.md` (on branch `instructor`) |
| Instructor working branch | `instructor` (branched from `main`; do not use `main` for lab prep) |
| Per-lab handout | branch `handout/<lab-id>` (from `instructor`) |
| Per-lab solution | branch `solution/<lab-id>` (from `instructor`, private to this repo only) |
| Student-facing repo | **new** repo in the same org, e.g. `UM6P-CC/minidb-lab1-buffer` |

**Rule:** keep `main` clean. All lab prep happens on `instructor` and lab branches.  
**Rule:** never add students as collaborators on `minidb` (they would see all branches).

---

## Branch model

```
main                 # stable reference (leave alone for day-to-day lab work)
  └── instructor     # one-time + ongoing instructor prep
        ├── handout/lab1-buffer
        ├── solution/lab1-buffer
        ├── handout/lab2-...
        └── solution/lab2-...
```

Suggested lab ids:

- `lab1-buffer` — getting started + buffer (CLOCK)
- `lab2-btree` — later
- `lab3-join` — later

---

## One-time setup (do once on `instructor`)

1. Checkout and push the instructor branch:
   ```bash
   git checkout main
   git pull
   git checkout -b instructor   # already created locally if you followed this guide
   git push -u origin instructor
   ```
2. On `instructor`, prepare shared infrastructure you want in every handout:
   - CI workflow that builds and runs tests on PRs
   - Clear student README template (build / run / submit via PR)
   - Any shared scripts or grading helpers
3. Commit those shared changes on `instructor` only (not on `main` until you deliberately merge).

Code changes that blank student work (e.g. remove CLOCK) are done **per lab** on `handout/<lab-id>`, not necessarily as a permanent change to `main`.

---

## Create a new lab (repeat each time)

Example: Lab 1 buffer.

### A. Solution branch (instructor only)

```bash
git checkout instructor
git pull
git checkout -b solution/lab1-buffer

# Ensure CLOCK (or whatever the lab requires) is fully implemented + tests pass.
# Commit on this branch. Never push this tree into a student repo.

git push -u origin solution/lab1-buffer
```

### B. Handout branch (what students get)

```bash
git checkout solution/lab1-buffer
git checkout -b handout/lab1-buffer

# Strip or stub only the student parts (e.g. CLOCK), keep LRU + rest of DB.
# Keep tests that fail until the student implements the hole.
# Update README for this lab only (objectives, commands, PR instructions).
# Remove solution-only files if any.

git push -u origin handout/lab1-buffer
```

Checklist before publishing:

- [ ] DB builds and runs (getting started works)
- [ ] Student hole is clearly incomplete (tests fail in the expected way)
- [ ] No solution code left in the handout tree
- [ ] CI config is present and green for “build + compile tests” (tests may fail until solution)
- [ ] README explains: clone/fork, build, implement, open PR

### C. Publish as a separate student repo in the org

Students get a **new empty repo** in the org. You push **only the handout branch tree** into it as `main`.

```bash
# 1) On GitHub (org UM6P-CC): create a new PRIVATE or PUBLIC repo, e.g.
#    minidb-lab1-buffer
#    Do NOT initialize with README (empty repo).

# 2) From this instructor clone, push handout → that repo's main:
git checkout handout/lab1-buffer

git push git@github.com:UM6P-CC/minidb-lab1-buffer.git HEAD:main

# Optional: also push as a named branch for your records
# git push git@github.com:UM6P-CC/minidb-lab1-buffer.git HEAD:handout
```

Alternative using `gh`:

```bash
gh repo create UM6P-CC/minidb-lab1-buffer --private --description "MiniDB Lab 1: getting started + buffer"
git checkout handout/lab1-buffer
git push git@github.com:UM6P-CC/minidb-lab1-buffer.git HEAD:main
```

**Where it goes:** same GitHub organization as this repo (`UM6P-CC`), **new repository name per lab**.  
It is **not** a branch students check out from `minidb`; it is a standalone repo whose `main` is a snapshot of `handout/<lab-id>`.

History tip (recommended for handouts): if you want a clean student history with no solution commits reachable, create an orphan snapshot:

```bash
git checkout handout/lab1-buffer
git checkout --orphan handout/lab1-buffer-clean
git add -A
git commit -m "Lab 1 handout: getting started + buffer manager"
git push git@github.com:UM6P-CC/minidb-lab1-buffer.git HEAD:main
```

### D. Student access (pick one)

**Preferred: fork → PR**

- Keep `minidb-lab1-buffer` readable by students (public, or private with read access).
- Each student/team **forks** the lab repo.
- They push to their fork and open a **PR into `UM6P-CC/minidb-lab1-buffer`** (or into a `submissions` workflow you define).
- CI runs on the PR; you review the PR.

**Alternative: collaborator write access**

- Add students as collaborators on the lab repo (not on `minidb`).
- Messier for many students; prefer forks.

**Never:** add students to `UM6P-CC/minidb`.

### E. After the lab (optional)

- Keep `solution/lab1-buffer` in this private instructor repo for grading reference.
- Tag releases if useful: `lab1-handout-2026`, `lab1-solution-2026` on the instructor repo only.

---

## Updating a lab after students already have the repo

1. Fix on `handout/<lab-id>` (and mirror on `solution/<lab-id>` if needed) in **this** repo.
2. Push updates to the student lab repo:
   ```bash
   git checkout handout/lab1-buffer
   git push git@github.com:UM6P-CC/minidb-lab1-buffer.git HEAD:main
   ```
3. Tell students to pull/rebase or re-fork if you rewrote history.

---

## Lab 1 concrete target (next work session)

| Item | Action |
|------|--------|
| Branch `instructor` | Shared CI + docs (this file) |
| Branch `solution/lab1-buffer` | CLOCK complete |
| Branch `handout/lab1-buffer` | CLOCK stubbed; LRU + DB kept; tests included |
| Org repo `UM6P-CC/minidb-lab1-buffer` | Published from handout only |
| This repo `UM6P-CC/minidb` | Instructors/TAs only |

---

## Quick commands cheat sheet

```bash
# Start from instructor
git checkout instructor && git pull

# New lab pair
git checkout -b solution/<lab-id>
# ... implement / verify ...
git push -u origin solution/<lab-id>

git checkout -b handout/<lab-id>
# ... stub student parts ...
git push -u origin handout/<lab-id>

# Publish student repo (after creating empty org repo on GitHub)
git push git@github.com:UM6P-CC/minidb-<lab-id>.git handout/<lab-id>:main
```
