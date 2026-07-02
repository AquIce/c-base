# Usage

## Run CI (with tests, sanitize on)

```sh
./build.py ci
````

## Any Build

```sh
./build.py configure --profile {profile}
./build.py build
./build.py test
```

---

## Run examples

```sh
./build.py show
```

Runs all example executables from `build/examples/` directly (not via CTest).

---

| Profile    | Build Type | Tests | Examples | Sanitizers | Purpose                                                                            |
| ---------- | ---------- | ----- | -------- | ---------- | ---------------------------------------------------------------------------------- |
| `dev`      | `Debug`    | ON    | ON       | ON         | Full local development setup with debugging, testing, and examples enabled         |
| `sanitize` | `Debug`    | ON    | OFF      | ON         | Debug + tests + sanitizers for catching memory/safety issues without example noise |
| `release`  | `Release`  | OFF   | OFF      | OFF        | Optimized production build with everything non-essential disabled                  |
| `ci`       | `Debug`    | ON    | OFF      | ON         | Continuous integration build focused on correctness and safety checks              |
| `eg`       | `Debug`    | OFF   | ON       | ON         | Example-only build for demonstration and experimentation (no tests)                |

---

| Command     | What it does                                                         |
| ----------- | -------------------------------------------------------------------- |
| `configure` | Runs CMake configuration step using a selected profile (`--profile`) |
| `build`     | Builds the project in `build/` using CMake                           |
| `test`      | Runs `ctest` inside `build/` (verbose output enabled)                |
| `clean`     | Deletes the entire `build/` directory                                |
| `rebuild`   | `clean → configure → build` using selected profile                   |
| `ci`        | Full CI pipeline: `clean → configure(ci) → build → test`             |
| `show`      | Builds project and runs example executables directly                 |
