# Presentation guides

One guide per stage of ASTROWORLD. Each guide is a script you can present from. It covers:
- **what the stage asked for** (the course brief)
- **what we built**, with the exact words to say and the actions to show and do
- **where the work lives**: every claim links to the file (and usually the function) that does it

Every guide has the same parts:

| Part | What it's for |
|---|---|
| Quick facts | Branch, tag, speaking time, what you show |
| Before you present | Commands to build and run, keys to practise, fallback images |
| What it asked for | The brief, quoted, then explained in plain words |
| The script | Timed sections with **Say** quotes and **Show / Do** cues |
| Requirement → what → where | A table to point at when someone asks "where did you do X?" |
| Questions you may be asked | Likely questions with short answers |
| The one-minute version | If time runs short |

## The guides

| Stage | Course checkpoint | Guide | Branch |
|---|---|---|---|
| Technical foundation (pure C++) | Checkpoint 1 | [checkpoint1.md](checkpoint1.md) | `checkpoint-1` |
| Technical foundation, live in a window | Checkpoint 1 (live demo variant) | [checkpoint1Window.md](checkpoint1Window.md) | `checkpoint-1-window` |
| Transformation and animation (pure C++) | Checkpoint 2 | [checkpoint2.md](checkpoint2.md) | `checkpoint-2` |
| OpenGL integration | Checkpoint 3 | [checkpoint3.md](checkpoint3.md) | `checkpoint-3` |
| Prototype: all components combined | Prototype stage | [prototype.md](prototype.md) | `prototype` |
| Final: ray tracing and path tracing | Checkpoint 4 (final demo, what works, what doesn't, recommendations) | [final.md](final.md) | `final` |
| Photorealism (side project) | Beyond the brief | [photorealism.md](photorealism.md) | `photorealism` |
| Blender (side project, latest) | Beyond the brief | [blender.md](blender.md) | `blender`, `main` |

Each branch carries its own guide and the guides of every earlier stage. `main` has them all.

## Before any presentation

1. **Check out the stage's branch and rebuild.** `build/` isn't tracked by git, so after switching branches the old programs are still there, built from a *different* stage. Always rebuild:

   ```bash
   rm -rf build && cmake -B build && cmake --build build -j
   ```

   Don't delete `output/`. On the `blender` branch it holds the exported data and the rendered film.
2. **Practise the keys** listed in the guide once.
3. **Keep the fallback images open** (from `docs/screenshots/`) in case a live demo misbehaves.
4. The exact submitted state of each checkpoint is tagged `submitted/<branch>`, for example `git checkout submitted/checkpoint-2`.
