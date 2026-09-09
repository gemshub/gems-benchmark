#!/bin/bash
# tools/ci.sh - the CI lane. Build GEMS3K, build the gate, run it.
#
# ============================================================================
# WHAT THIS IS FOR, AND WHAT IT IS NOT FOR
# ============================================================================
#
# This repo has two jobs and they want opposite things from a run:
#
#   BENCHMARK  work on the algorithm. Long sweeps, six solver modes, the freeze,
#              jitter-unstable projects, wall-time totals, exploratory settings.
#              Minutes to hours. Everything already here does this, and NOTHING
#              in this commit changes any of it: plain `ctest` still runs the
#              whole suite and every tool behaves exactly as before.
#   CI         one question, asked on every push: did this commit change an
#              answer or an iteration count where those are supposed to be
#              constants? Seconds. Never flaky, or it will be ignored.
#
# This script is the second one.
#
# ============================================================================
# IT USES AN INSTALLED GEMS3K IF THERE IS ONE, AND BUILDS ONE IF THERE IS NOT
# ============================================================================
#
# gems-benchmark is meant to be pulled INTO other repos - GEMS3K's own CI, or
# GEMSGUI's - and in those the library is already built and installed by the time
# anything here runs. Rebuilding it there would be wasted minutes and, worse, a
# DIFFERENT library from the one the parent repo just produced, which is the one
# actually under test. So:
#
#   1. --prefix DIR, or $GEMS3K_PREFIX          explicit, wins over everything
#   2. $CONDA_PREFIX, if it carries a GEMS3K    the parent-repo and CI-runner
#                                               case. conda-install-dependencies.sh
#                                               installs there, and so do GEMS3K's
#                                               and GEMSGUI's own environments.
#   3. otherwise, BUILD it from source          the standalone-developer case
#
# So inside GEMS3K or GEMSGUI, with their environment active, `tools/ci.sh` runs
# the gate against what they already installed and builds no library at all.
# --build-gems3k forces a build even when an install is found; --no-gems3k
# refuses to build and fails if no install can be resolved.
#
# WHAT IS ALWAYS CHECKED, whichever branch was taken: the resolved install must be
# NATIVE-ONLY, and the library the gate actually loaded must come from it. Neither
# is assumed - see "proving" below. An installed GEMS3K that turns out to carry
# Optima is refused with a message rather than silently widening the lane.
#
# ============================================================================
# WHEN IT DOES BUILD, IT BUILDS AT STOCK DEFAULTS. THAT IS THE DESIGN.
# ============================================================================
#
# Owner instruction, 2026-09-08: *"the ci should build gems and be default
# without any build config - should be just in native"*.
#
# It costs nothing to obey, because GEMS3K's own defaults already ARE the CI
# lane's requirements - checked, not assumed:
#
#     USE_OPTIMA_SOLVER   OFF   (GEMS3K/GEMS3K/CMakeLists.txt:27)
#     USE_THERMOFUN       ON    (GEMS3K/CMakeLists.txt:35)
#
# So this script passes NO SOLVER OPTION AT ALL. The solver is entirely at its
# defaults, which is the instruction.
#
# It does pass two SCOPE reductions, and they are not solver configuration:
#
#     BUILD_SOLMOD=OFF   the standalone TSolMod library. Nothing in the gate uses
#                        it, and at stock ON it does not currently build here at
#                        all: tsolmod4rkt/solmodfactory.h fails on
#                        "ThermoFun/ThermoFun.h: No such file or directory".
#                        tools/build-gems3k.sh has passed this for the same reason
#                        since before the CI lane existed.
#     BUILD_TOOLS=OFF    GEMS3K's own command-line tools. The gate links the
#                        library, not them.
#
# Both are overridable with --gems3k-cmake if a run wants the full build. Neither
# changes a line of solver behaviour; they change how much gets compiled.
#
# WHY THAT MATTERS MORE THAN IT LOOKS. With Optima absent from the library:
#
#   * difftestFindDeps.cmake auto-detects GEMS3K_HAS_OPTIMA = FALSE from the
#     imported target's own declared dependencies, so tests/CMakeLists.txt
#     REGISTERS NO OPTIMA CASE AT ALL and passes -DUSE_OPTIMA_SOLVER to nothing.
#     The lane is native by CONSTRUCTION rather than by selection - `ctest -L ci`
#     and plain `ctest` in a CI tree are both Optima-free.
#   * There is no Optima::Optima to resolve, so the GEMS3KConfig.cmake packaging
#     gap that bites every Optima-enabled install cannot arise here.
#
# And THERMOFUN STAYS ON because the corpus needs it: 21 of the 66 recorded
# projects ship a `-fun.json` (14 in gems3k, 4 in gems3k-fail, 3 in
# gems3k-proposed). Turning it off would not make the lane "more default" - it
# would make a third of the record unreadable. It is on because that is the
# GEMS3K default, which is the whole point of passing no configuration.
#
# It builds into ITS OWN PREFIX inside the build directory, never into
# /tmp/g3k-prefix. That install is the BENCHMARK's, it is Optima-enabled, and
# other sessions run gates against it - a CI script must not touch it.
#
# ============================================================================
# USAGE
# ============================================================================
#
#   tools/ci.sh                          resolve or build GEMS3K, build the gate, run it
#   tools/ci.sh --prefix DIR             use THIS GEMS3K install; never builds one
#   tools/ci.sh --build-gems3k           build GEMS3K even if an install was found
#   tools/ci.sh --no-gems3k              never build one; fail if none is found
#   tools/ci.sh --gems3k-src DIR         where GEMS3K's source is, when building
#   tools/ci.sh --gems3k-cmake ARG       extra cmake arg for the GEMS3K build (repeatable)
#   tools/ci.sh --build-dir DIR          where to work (default: build-ci)
#   tools/ci.sh --no-build               skip the benchmark build too
#   tools/ci.sh --extra-prefix DIR       add a dependency prefix (repeatable)
#   tools/ci.sh --full                   run the WHOLE suite instead of the ci
#                                        label - the benchmark lane. Not for CI.
#
# In a parent repo that has already installed GEMS3K, the whole invocation is:
#
#     conda activate <that repo's environment>
#     path/to/gems-benchmark/tools/ci.sh
#
# Run it from anywhere; it cd's to the repository root, because every path in the
# gate is repo-relative - the same convention every tool and test here uses, and
# running the solvus benchmark from the wrong directory reports "Tc = nan,
# failed = 301", a file-open error that reads exactly like a catastrophic
# regression (CLAUDE.md s4).
#
# ============================================================================
# THE MACHINE TRAPS THIS ENCODES
# ============================================================================
#
# All four were hit while writing it, and each failed somewhere that named
# neither the cause nor the cure:
#
#   1. GEMS3KTargets.cmake sets NO InterfaceIncludeDirectories, so
#      find_package(GEMS3K) yields a link target and no include path. The
#      compiler then finds some OTHER include/GEMS3K from an implicit system
#      directory - there is a stale 30-header tree in /usr/local/include - and
#      reports a missing member on a class nobody edited. Hence -I<prefix>/include.
#   2. Nor any rpath. Hence -Wl,-rpath,<prefix>/lib.
#   3. The system toolchain linking a conda-built library gives
#      "undefined reference to std::ios_base_library_init()@GLIBCXX_3.4.32".
#      The repo assumes the conda environment is active; this detects it.
#   4. Several coexisting GEMS3K installs, and libGEMS3K.so carries a classic
#      DT_RPATH that LD_LIBRARY_PATH cannot override (CLAUDE.md s5), so a wrong
#      resolution is SILENT. The linked library is printed AND asserted below.
set -euo pipefail

BUILD_DIR=build-ci
GEMS3K_SRC=${GEMS3K_SRC:-}
PREFIX=""
DO_GEMS3K=1
FORCE_GEMS3K=0
FORCE_NO_BUILD_G3K=0
DO_BUILD=1
LABEL=(-L ci)
EXTRA=()
G3K_EXTRA=()

# The test binaries behind the "ci" label. An explicit list rather than a derived
# one, so that adding a case to the label is a two-line decision taken in one
# commit, and so that a rename fails loudly here instead of silently building
# nothing. Must stay in step with the LABELS ci lines in tests/CMakeLists.txt.
CI_TARGETS=(test_ci_baseline test_volume_consistency)

while [ $# -gt 0 ]; do
  case "$1" in
    --build-dir)    BUILD_DIR="$2";   shift 2 ;;
    --gems3k-src)   GEMS3K_SRC="$2";  shift 2 ;;
    --prefix)       PREFIX="$2"; DO_GEMS3K=0; FORCE_NO_BUILD_G3K=1; shift 2 ;;
    --extra-prefix) EXTRA+=("$2");    shift 2 ;;
    --gems3k-cmake) G3K_EXTRA+=("$2"); shift 2 ;;
    --no-gems3k)    DO_GEMS3K=0; FORCE_NO_BUILD_G3K=1; shift ;;
    --build-gems3k) FORCE_GEMS3K=1; shift ;;
    --no-build)     DO_BUILD=0;  shift ;;
    --full)         LABEL=();    shift ;;
    -h|--help)      sed -n '2,100p' "$0"; exit 0 ;;
    *) echo "tools/ci.sh: unknown argument: $1" >&2; exit 2 ;;
  esac
done

cd "$(dirname "$0")/.."
ROOT=$(pwd)
BUILD_DIR=$(mkdir -p "$BUILD_DIR" && cd "$BUILD_DIR" && pwd)

HAVE_TESTS=0
[ -f "$ROOT/tests/CMakeLists.txt" ] && HAVE_TESTS=1

echo "== gems-benchmark CI lane"
echo "   repo   $ROOT"
echo "   gate   $([ "$HAVE_TESTS" = 1 ] && echo 'ctest -L ci' || echo 'ci_baseline --check')"
echo "   build  $BUILD_DIR"

# ---------------------------------------------------------------------------
# Resolve the GEMS3K install: use one if it is already there, build one if not.
# ---------------------------------------------------------------------------
has_gems3k() { [ -n "$1" ] && [ -f "$1/lib/cmake/GEMS3K/GEMS3KConfig.cmake" ]; }
SOURCE_OF_PREFIX=""

if [ -n "$PREFIX" ]; then
    SOURCE_OF_PREFIX="--prefix"
elif [ "$FORCE_GEMS3K" = 1 ]; then
    PREFIX="$BUILD_DIR/gems3k-prefix"; SOURCE_OF_PREFIX="--build-gems3k"
elif has_gems3k "${CONDA_PREFIX:-}"; then
    # THE PARENT-REPO CASE. GEMS3K's and GEMSGUI's environments carry an installed
    # GEMS3K, and so does anything that ran a conda-install-dependencies.sh. When
    # this repo is pulled into one of those, the library under test is the one
    # they just installed - rebuilding it here would burn minutes to produce a
    # DIFFERENT library from the one the parent actually ships.
    PREFIX="$CONDA_PREFIX"; SOURCE_OF_PREFIX="\$CONDA_PREFIX"
    DO_GEMS3K=0
elif has_gems3k "$BUILD_DIR/gems3k-prefix"; then
    PREFIX="$BUILD_DIR/gems3k-prefix"; SOURCE_OF_PREFIX="this build dir"
    DO_GEMS3K=0
else
    PREFIX="$BUILD_DIR/gems3k-prefix"; SOURCE_OF_PREFIX="to be built"
fi

# --no-gems3k / --prefix mean "do not build one". Say so plainly rather than
# building anyway or failing three steps later with a missing-file error.
if [ "$FORCE_NO_BUILD_G3K" = 1 ] && ! has_gems3k "$PREFIX"; then
    echo "tools/ci.sh: no GEMS3K install at $PREFIX, and building one was refused" >&2
    echo "  (--prefix and --no-gems3k both mean 'do not build'). Either point" >&2
    echo "  --prefix at an install, activate an environment that carries one, or" >&2
    echo "  drop the flag and let this build GEMS3K from source." >&2
    exit 1
fi
[ "$FORCE_NO_BUILD_G3K" = 1 ] && DO_GEMS3K=0
has_gems3k "$PREFIX" && [ "$FORCE_GEMS3K" = 0 ] && DO_GEMS3K=0

echo "   gems3k $PREFIX   [$SOURCE_OF_PREFIX]"
[ "$DO_GEMS3K" = 0 ] && echo "          using the installed library - not building it"

# ---------------------------------------------------------------------------
# Toolchain. Honour CC/CXX if the caller set them; otherwise prefer the conda
# environment's compilers over the system ones - see trap 3 above.
# ---------------------------------------------------------------------------
if [ -z "${CXX:-}" ]; then
  for e in "${CONDA_PREFIX:-}" "${CONDA_PREFIX:-}/envs/gems-benchmark" \
           "${CONDA_PREFIX:-}/../gems-benchmark"; do
    if [ -n "$e" ] && [ -x "$e/bin/x86_64-conda-linux-gnu-c++" ]; then
      CC="$e/bin/x86_64-conda-linux-gnu-cc"
      CXX="$e/bin/x86_64-conda-linux-gnu-c++"
      break
    fi
  done
fi
[ -n "${CXX:-}" ] && echo "   cxx    $CXX" \
                  || echo "   cxx    (cmake default)"

# Dependency prefixes, in the order tools/build-gems3k.sh documents: conda base
# first, then anything the caller adds. Getting it the other way round resolves
# spdlog/fmt - and, worse, ThermoFun and its stale sibling GEMS3K headers - from
# the wrong place.
CPP=""
[ -n "${CONDA_PREFIX:-}" ] && CPP="$CONDA_PREFIX"
[ -n "${CMAKE_PREFIX_PATH:-}" ] && CPP="${CPP:+$CPP;}$CMAKE_PREFIX_PATH"
for e in ${EXTRA+"${EXTRA[@]}"}; do CPP="${CPP:+$CPP;}$e"; done

# ---------------------------------------------------------------------------
# 1. GEMS3K, at stock defaults
# ---------------------------------------------------------------------------
if [ "$DO_GEMS3K" = 1 ]; then
  # Find the source. Several layouts are plausible once this repo is pulled into
  # another one, so look for the marker file rather than assuming a path:
  #   ../GEMS3K      a sibling checkout - the standalone developer's layout
  #   ..             this repo nested INSIDE the GEMS3K repo
  #   ../../GEMS3K   both nested under a common parent
  # ms_multi.h is the marker because a bare CMakeLists.txt says nothing.
  if [ -z "$GEMS3K_SRC" ]; then
    for cand in "$ROOT/../GEMS3K" "$ROOT/.." "$ROOT/../../GEMS3K"; do
      if [ -f "$cand/GEMS3K/ms_multi.h" ]; then GEMS3K_SRC="$cand"; break; fi
    done
  fi
  if [ -z "$GEMS3K_SRC" ] || [ ! -f "$GEMS3K_SRC/GEMS3K/ms_multi.h" ]; then
    if [ -n "$GEMS3K_SRC" ]; then
      echo "tools/ci.sh: $GEMS3K_SRC is not a GEMS3K source tree" >&2
      echo "  (no GEMS3K/ms_multi.h in it)." >&2
    else
      echo "tools/ci.sh: no GEMS3K source found - looked for GEMS3K/ms_multi.h in" >&2
      echo "  ../GEMS3K, .., ../../GEMS3K - and no installed GEMS3K was resolved." >&2
    fi
    echo "  Either activate an environment that carries an installed GEMS3K," >&2
    echo "  pass --prefix DIR, or pass --gems3k-src DIR / set \$GEMS3K_SRC." >&2
    exit 1
  fi
  GEMS3K_SRC=$(cd "$GEMS3K_SRC" && pwd)
  GB="$BUILD_DIR/gems3k-build"
  echo "== building GEMS3K at STOCK DEFAULTS (no solver options passed)"
  echo "   source $GEMS3K_SRC"
  mkdir -p "$GB"
  # NO SOLVER OPTION IS PASSED. USE_OPTIMA_SOLVER and USE_THERMOFUN are left at
  # GEMS3K's own defaults (OFF and ON), which is precisely what the lane wants.
  # BUILD_SOLMOD / BUILD_TOOLS are scope, not behaviour - see the note at the top.
  cmake -S "$GEMS3K_SRC" -B "$GB" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$PREFIX" \
        -DBUILD_SOLMOD=OFF \
        -DBUILD_TOOLS=OFF \
        ${G3K_EXTRA+"${G3K_EXTRA[@]}"} \
        ${CPP:+-DCMAKE_PREFIX_PATH="$CPP"} \
        ${CC:+-DCMAKE_C_COMPILER="$CC"} \
        ${CXX:+-DCMAKE_CXX_COMPILER="$CXX"} > "$BUILD_DIR/gems3k-configure.log" 2>&1 || {
    echo "GEMS3K configure FAILED - tail of $BUILD_DIR/gems3k-configure.log:" >&2
    tail -40 "$BUILD_DIR/gems3k-configure.log" >&2; exit 1; }

  # Unfiltered, and gated on the compiler's EXIT STATUS. Both directions of the
  # /error/ grep mistake are on record (CLAUDE.md s4): filtering once hid a
  # printf/-Wformat mismatch that printed stack garbage for a whole column, and a
  # success check looking for the word "error" matches this repo's own verror.h
  # on every translation unit that includes it, manufacturing a failure that did
  # not happen.
  # `cmd | tee f || true` LOOKS like it preserves the status - it does not: the
  # `|| true` runs a simple command, which RESETS PIPESTATUS, so the check below
  # always read 0 and the failure sailed straight past. Capture it first.
  set +e
  cmake --build "$GB" -j"$(nproc)" 2>&1 | tee "$BUILD_DIR/gems3k-build.log"
  rc=${PIPESTATUS[0]}
  set -e
  if [ "$rc" != 0 ]; then
    echo "GEMS3K build FAILED" >&2
    # Name the one failure that is neither the environment's fault nor this
    # script's, because its message points at a line rather than at the cause.
    if grep -q "insBudgetTried' was not declared" "$BUILD_DIR/gems3k-build.log"; then
      echo >&2
      echo "  THE GEMS3K SOURCE DOES NOT COMPILE AT ITS OWN DEFAULTS." >&2
      echo "  'std::vector<char> insBudgetTried' is declared INSIDE the" >&2
      echo "  #ifdef USE_OPTIMA_SOLVER block in GEMS3K/ms_multi.h (opened at" >&2
      echo "  line 1981), but it is used on the NATIVE path -" >&2
      echo "  ipm_chemical.cpp:1605,1610 and ipm_main.cpp:803 - so a build with" >&2
      echo "  USE_OPTIMA_SOLVER=OFF, which is the default, cannot succeed." >&2
      echo "  Introduced in GEMS3K commit d7c9a10. The fix is to move that one" >&2
      echo "  declaration out of the #ifdef; it is a native-path member." >&2
      echo "  Until then, run the lane against an existing install:" >&2
      echo "      tools/ci.sh --prefix /path/to/a/gems3k/install" >&2
    fi
    if grep -q "ThermoFun/ThermoFun.h: No such file" "$BUILD_DIR/gems3k-build.log"; then
      echo >&2
      echo "  tsolmod4rkt cannot find ThermoFun's headers. This script already" >&2
      echo "  passes -DBUILD_SOLMOD=OFF for that reason; if you overrode it with" >&2
      echo "  --gems3k-cmake, that is why." >&2
    fi
    exit 1
  fi
  cmake --install "$GB" > "$BUILD_DIR/gems3k-install.log" 2>&1
fi

has_gems3k "$PREFIX" || {
  echo "tools/ci.sh: no GEMS3K install at $PREFIX after resolution" >&2; exit 1; }

# ---------------------------------------------------------------------------
# 2. PROVE the library is native-only, rather than trusting the default
# ---------------------------------------------------------------------------
# Same principle as tools/trace_selfcheck.sh, which this script does NOT call and
# does not depend on: a build flag is a proxy, the artefact is the thing. If a
# future GEMS3K flips the default, or someone points --prefix at an Optima build,
# this is what notices - and it matters, because with Optima present the whole
# benchmark tree starts registering Optima cases and compiling -DUSE_OPTIMA_SOLVER
# into binaries the CI label was chosen to exclude.
echo "== proving the linked GEMS3K is native-only"
if grep -q "Optima::Optima" "$PREFIX/lib/cmake/GEMS3K/GEMS3KTargets.cmake" 2>/dev/null; then
  echo "tools/ci.sh: $PREFIX was built WITH the Optima solver." >&2
  echo "  The CI lane is native-only (AIA/SIA). Build GEMS3K at stock defaults" >&2
  echo "  - USE_OPTIMA_SOLVER is OFF by default - or point --prefix elsewhere." >&2
  exit 1
fi
echo "   GEMS3KTargets.cmake declares no Optima::Optima - native only"

# ---------------------------------------------------------------------------
# 2b. Does this install actually carry the API the gate compiles against?
# ---------------------------------------------------------------------------
# Worth checking UP FRONT, and worth checking by API rather than by version.
#
# An install resolved from $CONDA_PREFIX belongs to whoever set that environment
# up, and it can be older than this benchmark needs. Without this check the run
# spends two minutes compiling and then fails inside src/metrics_collector.cpp
# with "class TNode has no member named otherPMM" - a message that points at this
# repo's own source rather than at the library that is actually too old.
#
# AND NOT BY VERSION NUMBER, because the version string does not track the API:
# measured 2026-09-08, three installs all report "4.5.5" and they carry DIFFERENT
# APIs - one has TNode::otherPMM(), another does not, and neither has
# Get_GibbsEnergy(). Asking the header for each member is the thing; the version
# is printed for the log and nothing is decided on it.
#
# The list is every TNode member the CI lane's two binaries call that is not
# ancient. Keep it in step with include/difftest/ci_baseline.h and
# include/difftest/volume_consistency.h - a member added there and forgotten here
# only costs a slower, less clear failure, never a wrong result.
G3K_VER=$(grep -oE '"[0-9]+\.[0-9]+\.[0-9]+"' "$PREFIX/include/GEMS3K/gems3k_version.h" 2>/dev/null | head -1)
echo "   version ${G3K_VER:-unknown} (informational - it does not track the API)"
REQUIRED_API="otherPMM GEM_CalcTime Get_GibbsEnergy Get_pH Get_Eh cVs cMs Ph_Moles Ph_Volume"
MISSING=""
for sym in $REQUIRED_API; do
  grep -q "$sym" "$PREFIX/include/GEMS3K/node.h" 2>/dev/null || MISSING="$MISSING $sym"
done
if [ -n "$MISSING" ]; then
  echo "tools/ci.sh: the GEMS3K installed at" >&2
  echo "    $PREFIX" >&2
  echo "  is too old for this benchmark. Its TNode is missing:$MISSING" >&2
  echo >&2
  if [ "$SOURCE_OF_PREFIX" = "\$CONDA_PREFIX" ]; then
    echo "  It was picked up from \$CONDA_PREFIX, i.e. it is the active environment's" >&2
    echo "  own GEMS3K. Either update that environment's GEMS3K, or build one here:" >&2
    echo "      tools/ci.sh --build-gems3k" >&2
  else
    echo "  Point --prefix at a newer install, or build one here:" >&2
    echo "      tools/ci.sh --build-gems3k" >&2
  fi
  exit 1
fi
echo "   required TNode API present ($(echo $REQUIRED_API | wc -w) members checked)"

# ---------------------------------------------------------------------------
# 3. The gate
# ---------------------------------------------------------------------------
if [ "$DO_BUILD" = 1 ]; then
  # TWO WAYS TO RUN THE GATE, and which one is available depends on the checkout.
  #
  # The asserting binary lives in tests/, and the CTest registration with it. That
  # directory is not part of every checkout - in this repository it is worked on
  # locally and lands separately - so this script does not require it. When
  # tests/CMakeLists.txt is present the lane runs through `ctest -L ci`, which is
  # the preferred form; when it is not, the RECORDER ITSELF IS THE GATE:
  # `ci_baseline --check` re-measures, scores against the frozen record and exits
  # 1 on any finding. Same header, same tolerances, same severity ranking - the
  # scoring lives in include/difftest/ci_baseline.h precisely so the two forms
  # cannot disagree.
  #
  # BUILD_TOOLS is therefore ON in the fallback and OFF in the ctest path. Off
  # where possible is not only for speed: tools/CMakeLists.txt file(COPY)s the
  # whole Resources/gems3k corpus into the build tree, which a gate has no use for.
  cmake -S . -B "$BUILD_DIR" \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_TESTS=$([ "$HAVE_TESTS" = 1 ] && echo ON || echo OFF) \
        -DBUILD_TOOLS=$([ "$HAVE_TESTS" = 1 ] && echo OFF || echo ON) \
        -DCMAKE_PREFIX_PATH="$PREFIX${CPP:+;$CPP}" \
        -DGEMS3K_DIR="$PREFIX/lib/cmake/GEMS3K" \
        -DCMAKE_CXX_FLAGS="-I$PREFIX/include" \
        -DCMAKE_EXE_LINKER_FLAGS="-Wl,-rpath,$PREFIX/lib" \
        ${CC:+-DCMAKE_C_COMPILER="$CC"} \
        ${CXX:+-DCMAKE_CXX_COMPILER="$CXX"} > "$BUILD_DIR/configure.log" 2>&1 || {
    echo "configure FAILED - tail of $BUILD_DIR/configure.log:" >&2
    tail -40 "$BUILD_DIR/configure.log" >&2; exit 1; }

  grep -E 'Found (GEMS3K|ThermoFun|ChemicalFun)|GEMS3K has no Optima|enabling Optima' \
       "$BUILD_DIR/configure.log" | sed 's/^-- /   /' || true

  # Build ONLY the CI lane's own targets. The "ci" label already selects cases
  # whose binaries contain no Optima code; building by target name means nothing
  # else is even PRODUCED here, so there is nothing in this tree to run by
  # mistake. --full builds everything, because then everything is what gets run.
  # Which targets. --full means "run the whole suite", so build everything;
  # otherwise build just what the lane needs. In the fallback that is the
  # recorder; in the ctest path it is the labelled tests' binaries, filtered to
  # the ones this checkout's tests/CMakeLists.txt actually defines, so a partial
  # tests/ directory produces a smaller lane rather than a build error.
  TARGETS=()
  if [ ${#LABEL[@]} -ne 0 ]; then
    if [ "$HAVE_TESTS" = 1 ]; then
      KNOWN=$(cmake --build "$BUILD_DIR" --target help 2>/dev/null || true)
      for t in "${CI_TARGETS[@]}"; do
        case "$KNOWN" in *"$t"*) TARGETS+=("$t") ;; esac
      done
      [ ${#TARGETS[@]} -eq 0 ] && { echo "tools/ci.sh: none of the CI targets exist in this checkout" >&2; exit 1; }
    else
      TARGETS=(ci_baseline)
    fi
  fi

  set +e
  if [ ${#TARGETS[@]} -eq 0 ]; then
    cmake --build "$BUILD_DIR" -j"$(nproc)" 2>&1 | tee "$BUILD_DIR/build.log"
  else
    cmake --build "$BUILD_DIR" -j"$(nproc)" --target "${TARGETS[@]}" 2>&1 \
      | tee "$BUILD_DIR/build.log"
  fi
  rc=${PIPESTATUS[0]}
  set -e
  if [ "$rc" != 0 ]; then
    echo "build FAILED" >&2
    if grep -q 'GLIBCXX_3\.4\.3' "$BUILD_DIR/build.log"; then
      echo "  That GLIBCXX symbol means the SYSTEM toolchain is linking a" >&2
      echo "  conda-built libGEMS3K.so. Activate the environment first:" >&2
      echo "      conda activate gems-benchmark && tools/ci.sh" >&2
    fi
    if grep -qE 'gems3k_version\.h|otherPMM' "$BUILD_DIR/build.log"; then
      # The resolved prefix was already checked to HAVE this API (step 2b), so if
      # the compiler cannot see it, some OTHER include/GEMS3K is being read first.
      echo "  $PREFIX carries this API - the step-2b check passed - so STALE GEMS3K" >&2
      echo "  HEADERS ARE SHADOWING IT. Usually ThermoFun resolving from a prefix that" >&2
      echo "  also holds an old include/GEMS3K (here, /usr/local). Add the" >&2
      echo "  prefix holding the ThermoFun you mean:" >&2
      echo "      --extra-prefix \$CONDA_ROOT/envs/xgems-jupyter" >&2
    fi
    exit 1
  fi
fi

echo "== linked library"
GATE="$BUILD_DIR/bin/test_ci_baseline"
[ -x "$GATE" ] || GATE="$BUILD_DIR/bin/ci_baseline"
[ -x "$GATE" ] || { echo "no gate binary in $BUILD_DIR/bin - build first, or drop --no-build" >&2; exit 1; }
ldd "$GATE" | grep -iE 'gems3k|thermofun|optima' | sed 's/^/   /' || true

# Assert it, rather than only printing it. A print is a record for whoever reads
# the log; this is what stops a run that resolved the wrong install from producing
# numbers at all.
GOT=$(ldd "$GATE" | awk '/libGEMS3K/{print $3}')
case "$GOT" in
  "$PREFIX"/*) ;;
  *) echo "tools/ci.sh: linked libGEMS3K is $GOT, not from $PREFIX" >&2
     echo "  A wrong resolution here is SILENT - libGEMS3K.so carries a classic" >&2
     echo "  DT_RPATH, so LD_LIBRARY_PATH cannot override it (CLAUDE.md s5)." >&2
     exit 1 ;;
esac

if [ "$HAVE_TESTS" = 1 ]; then
  echo "== ctest ${LABEL[*]:-(whole suite)}"
  ctest --test-dir "$BUILD_DIR" "${LABEL[@]}" --output-on-failure
else
  echo "== ci_baseline --check  (no tests/CMakeLists.txt in this checkout)"
  "$BUILD_DIR/bin/ci_baseline" --check "$ROOT/tests/ci-baseline.txt"
fi
