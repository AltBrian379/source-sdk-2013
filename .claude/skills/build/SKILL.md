---
name: build
description: Build the OWTF2 TF2 mod with tools/build.ps1 and fix any compile or link errors. Use after C++ changes, or when the user says build, compile, or /build.
---

# Build the mod

1. Run `powershell -ExecutionPolicy Bypass -File tools/build.ps1`.
   - Add `-Regenerate` if any `.vpc` or `.vgc` file changed since the last build,
     or if new `.cpp`/`.h` files were added to a `.vpc`.
   - Pass through any arguments the user gave, such as `-Config Debug` or `-Full`.
2. If it succeeds, report "build succeeded" and the warning count in one line.
3. If it fails:
   - Start with the first error. Later errors are often caused by earlier ones.
   - Read the failing file around the reported line before changing anything.
   - For errors in code this mod changed (look for `// OWTF2:` comments, or
     `git diff`), fix the cause.
   - For errors in untouched Valve code, the cause is usually the toolchain
     (missing Visual Studio component, Windows SDK, or Python), not the code.
     Explain what's missing instead of editing Valve's code.
   - Never edit generated `.sln` or `.vcxproj` files, or anything in
     `src/thirdparty/`.
   - Rebuild after each fix. Stop after 3 failed attempts on the same error and
     report what you tried.
4. Full log: `tools/logs/last-build.log`.
