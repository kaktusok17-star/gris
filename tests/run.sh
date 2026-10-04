#!/usr/bin/env bash
# tests/run.sh — тесты для gris
#
# Использование:
#   ./tests/run.sh
#   make test

set -u

# ── Пути ──────────────────────────────────────────────────
PROJECT="$(cd "$(dirname "$0")/.." && pwd)"
GRIS="$PROJECT/gris"
TMP="/tmp/gris-tests"
ROOT="$TMP/root"
REPO="$TMP/repo"
BUILD="$TMP/build"

# ── Счётчики ──────────────────────────────────────────────
PASS=0
FAIL=0

# ── Утилиты вывода ────────────────────────────────────────
section() { echo; echo "── $1 ──"; }
pass()    { PASS=$((PASS+1)); printf '  \033[32m✓\033[0m %s\n' "$1"; }
fail()    { FAIL=$((FAIL+1)); printf '  \033[31m✗\033[0m %s\n' "$1"; }

assert_eq() {
    if [ "$2" = "$3" ]; then
        pass "$1"
    else
        fail "$1"
        printf '      expected: %s\n' "$2"
        printf '      actual:   %s\n' "$3"
    fi
}

assert_contains() {
    case "$3" in
        *"$2"*) pass "$1" ;;
        *) fail "$1"
           printf '      needle: %s\n' "$2"
           printf '      in:     %s\n' "$3"
        ;;
    esac
}

assert_not_contains() {
    case "$3" in
        *"$2"*) fail "$1"
                printf '      не должно содержать: %s\n' "$2"
        ;;
        *) pass "$1" ;;
    esac
}

sha256_of() {
    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$1" | awk '{print $1}'
    else
        shasum -a 256 "$1" | awk '{print $1}'
    fi
}

# gris с настроенным окружением
gr() {
    GRIS_ROOT="$ROOT" GRIS_REPO="file://$REPO" "$GRIS" "$@"
}

# ── Подготовка пакетов ────────────────────────────────────
mkpkg() {
    local name="$1" ver="$2" deps="${3:-}"
    local dir="$BUILD/$name"
    rm -rf "$dir"
    mkdir -p "$dir/usr/bin"
    printf '#!/bin/sh\necho %s\n' "$name" > "$dir/usr/bin/$name"
    chmod 755 "$dir/usr/bin/$name"
    {
        echo "pkgname = $name"
        echo "pkgver = $ver"
        echo "arch = x86_64"
        echo "license = MIT"
        [ -n "$deps" ] && echo "depends = $deps"
    } > "$dir/.PKGINFO"
    ( cd "$TMP" && "$GRIS" build "$dir" >/dev/null )
    mv "$TMP/$name-$ver-x86_64.gris" "$REPO/"
}

mkindex() {
    ( cd "$REPO"
      : > core.index
      for f in *.gris; do
          [ -f "$f" ] || continue
          name=$(tar -xOJf "$f" --wildcards '*PKGINFO' | awk -F' = ' '/^pkgname/{print $2}')
          ver=$(tar -xOJf "$f" --wildcards '*PKGINFO' | awk -F' = ' '/^pkgver/{print $2}')
          arch=$(tar -xOJf "$f" --wildcards '*PKGINFO' | awk -F' = ' '/^arch/{print $2}')
          deps=$(tar -xOJf "$f" --wildcards '*PKGINFO' | awk -F' = ' '/^depends/{print $2}')
          sha=$(sha256_of "$f")
          url="file://$REPO/$f"
          echo "$name|$ver|$arch|$deps|1024|$url|$sha" >> core.index
      done
    )
}

reset_env() {
    rm -rf "$ROOT"
}

# ── Старт ─────────────────────────────────────────────────
if [ ! -x "$GRIS" ]; then
    echo "Ошибка: $GRIS не найден или не исполняемый."
    echo "Сначала: make"
    exit 1
fi

rm -rf "$TMP"
mkdir -p "$ROOT" "$REPO" "$BUILD"

echo "gris test suite"
echo "project: $PROJECT"
echo "tmp:     $TMP"

# ═══════════════════════════════════════════════════════════
section "basic"

out=$("$GRIS" --version)
assert_contains "--version печатает версию" "gris " "$out"

out=$("$GRIS" --help)
assert_contains "--help содержит короткие флаги" "-Syu" "$out"

out=$("$GRIS" --help)
assert_contains "--help содержит длинные команды" "install" "$out"

"$GRIS" zzz >/dev/null 2>&1
rc=$?
assert_eq "неизвестная команда → rc=2" "2" "$rc"

"$GRIS" >/dev/null 2>&1
rc=$?
assert_eq "без аргументов → rc=2" "2" "$rc"

# ═══════════════════════════════════════════════════════════
section "build"

mkpkg bar 1.0-1
mkpkg foo 1.0-1 "bar>=1.0"
mkindex

[ -f "$REPO/bar-1.0-1-x86_64.gris" ] && pass "bar.gris собран" || fail "bar.gris собран"
[ -f "$REPO/foo-1.0-1-x86_64.gris" ] && pass "foo.gris собран" || fail "foo.gris собран"

flist=$(tar -xOJf "$REPO/bar-1.0-1-x86_64.gris" --wildcards '*FILELIST')
assert_contains "FILELIST содержит usr/bin/bar" "usr/bin/bar" "$flist"
assert_not_contains "FILELIST не содержит .PKGINFO" ".PKGINFO" "$flist"

# ═══════════════════════════════════════════════════════════
section "sync + search"

out=$(gr -Sy 2>&1)
assert_contains "-Sy скачивает индекс" "ok" "$out"
[ -f "$ROOT/var/lib/gris/repo/core.index" ] \
    && pass "core.index на месте" \
    || fail "core.index на месте"

out=$(gr -Ss '^b' 2>&1)
assert_contains "-Ss ^b находит bar" "bar" "$out"

out=$(gr -Ss '^f' 2>&1)
assert_contains "-Ss ^f находит foo" "foo" "$out"

gr -Ss 'zzz' >/dev/null 2>&1
rc=$?
assert_eq "-Ss zzz → rc=1" "1" "$rc"

# ═══════════════════════════════════════════════════════════
section "install"

reset_env
gr -Sy >/dev/null

out=$(gr -S bar 2>&1)
assert_contains "-S bar устанавливает" "installed: bar 1.0-1" "$out"
[ -f "$ROOT/usr/bin/bar" ] && pass "файл bar на месте" || fail "файл bar на месте"

out=$(gr -S bar 2>&1)
assert_contains "second -S bar → already installed" "already installed" "$out"

out=$(gr -S foo 2>&1)
assert_contains "-S foo ставит foo" "installed: foo 1.0-1" "$out"
assert_not_contains "-S foo НЕ переустанавливает bar" "installed: bar" "$out"

[ -f "$ROOT/usr/bin/foo" ] && pass "файл foo на месте" || fail "файл foo на месте"

# ═══════════════════════════════════════════════════════════
section "query"

out=$(gr -Q)
assert_contains "-Q видит bar" "bar 1.0-1" "$out"
assert_contains "-Q видит foo" "foo 1.0-1" "$out"

out=$(gr -Qi bar)
assert_contains "-Qi показывает Name" "Name" "$out"
assert_contains "-Qi показывает версию" "1.0-1" "$out"

out=$(gr -Ql bar)
assert_contains "-Ql показывает файлы" "usr/bin/bar" "$out"

out=$(gr info bar)
assert_contains "длинная info работает" "bar" "$out"

# ═══════════════════════════════════════════════════════════
section "files (неустановленный пакет)"

# Свежее окружение: ставим ТОЛЬКО bar. foo остаётся неустановленным.
reset_env
gr -Sy >/dev/null
gr -S bar >/dev/null
rm -rf "$ROOT/var/lib/gris/cache"

out=$(gr files foo 2>&1)
assert_contains "files foo скачивает для неустановленного" "downloading:" "$out"
assert_contains "files foo показывает путь" "usr/bin/foo" "$out"

out=$(gr -Q)
assert_not_contains "files НЕ устанавливает пакет" "foo" "$out"

out=$(gr files bar 2>&1)
assert_not_contains "files установленного — без downloading" "downloading" "$out"

gr files zzz >/dev/null 2>&1
rc=$?
assert_eq "files zzz → rc=1" "1" "$rc"

# ═══════════════════════════════════════════════════════════
section "remove"

# Ставим оба, чтобы проверить обратную зависимость
gr -S foo >/dev/null

out=$(gr -R bar 2>&1)
rc=$?
assert_contains "-R bar refuses (foo depends)" "required by" "$out"
assert_eq "-R bar → rc=1" "1" "$rc"

out=$(gr -R foo 2>&1)
assert_contains "-R foo удаляет" "removed: foo" "$out"

out=$(gr -R --force bar 2>&1)
assert_contains "-R --force bar удаляет" "removed: bar" "$out"

out=$(gr -Q)
assert_eq "-Q пусто после удаления" "" "$out"

# ═══════════════════════════════════════════════════════════
section "upgrade"

reset_env
gr -Sy >/dev/null
gr -S bar >/dev/null

rm -f "$REPO/bar-"*.gris
mkpkg bar 1.1-1
mkindex
gr -Sy >/dev/null

out=$(gr -Syu 2>&1)
assert_contains "-Syu апгрейдит bar" "upgrading: bar (1.0-1 -> 1.1-1)" "$out"
assert_contains "-Syu ставит новую версию" "installed: bar 1.1-1" "$out"

out=$(gr -Q)
assert_contains "-Q видит новую версию" "bar 1.1-1" "$out"

out=$(gr -Syu 2>&1)
assert_contains "повторный -Syu → up to date" "up to date" "$out"

# ═══════════════════════════════════════════════════════════
section "clean"

gr -S bar >/dev/null 2>&1
out=$(gr -Sc 2>&1)
assert_contains "-Sc чистит кэш" "cache cleaned" "$out"

# ═══════════════════════════════════════════════════════════
# Итог
echo
echo "════════════════════════════════════"
printf "PASS: \033[32m%d\033[0m\n" "$PASS"
printf "FAIL: \033[31m%d\033[0m\n" "$FAIL"
echo "════════════════════════════════════"

[ "$FAIL" -eq 0 ]