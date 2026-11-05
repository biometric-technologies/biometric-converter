#!/bin/sh
#
# Tests for the converter library and command-line tool.
#
#   tests/run.sh <build directory>
#
# Uses example/sample_image.wsq. The comparison with NBIS mindtct and the
# PNG checks need mindtct, dwsq and python3 on the PATH, and are skipped
# otherwise. With VALGRIND=1 a few conversions also run under valgrind;
# tests/valgrind.supp lists a known leak in BiomDI.

set -u

BUILD=$(cd "${1:-build}" && pwd)
SRC=$(cd "$(dirname "$0")/.." && pwd)
SAMPLE=$SRC/example/sample_image.wsq
CONVERT=$BUILD/convert
TEST=$BUILD/test_converter

# Use the library from the build directory, not an installed one
LD_LIBRARY_PATH="$BUILD${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export LD_LIBRARY_PATH

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
FAILED=0

fail() {
    echo "FAIL: $*"
    FAILED=1
}

have() {
    command -v "$1" >/dev/null 2>&1
}

# Resolution in the header of an ANSI record (pixels per centimetre)
ansi_res() {
    od -An -tu1 -j20 -N4 "$1" | awk '{ printf "%dx%d\n", $1 * 256 + $2, $3 * 256 + $4 }'
}

echo "== Library checks"
"$TEST" "$SAMPLE" 2>/dev/null || fail "test_converter"

echo "== Command-line tool"
for to in ANSI ISO ISONC ISOCC; do
    if "$CONVERT" -i "$SAMPLE" -ti WSQ -o "$WORK/sample.$to" -to "$to" >/dev/null 2>&1 &&
        [ -s "$WORK/sample.$to" ]; then
        echo "  image -> $to: $(wc -c < "$WORK/sample.$to") bytes"
    else
        fail "convert image -> $to"
    fi
done
for ti in ANSI ISO ISONC ISOCC; do
    for to in ANSI ISO ISONC ISOCC; do
        "$CONVERT" -i "$WORK/sample.$ti" -ti "$ti" -o "$WORK/conv.$ti.$to" -to "$to" \
            >/dev/null 2>&1 || fail "convert $ti -> $to"
    done
done
echo "  all 16 minutiae conversions ran"
if "$CONVERT" -i "$SAMPLE" -ti WSQ -o "$WORK/bad" -to XYZ >/dev/null 2>&1; then
    fail "unknown output type accepted"
fi
if "$CONVERT" -i "$SAMPLE" -ti ANSI -o "$WORK/bad" -to ISO >/dev/null 2>&1; then
    fail "image accepted as an ANSI record"
fi

echo "== Comparison with NBIS mindtct"
if have mindtct; then
    # mindtct -m1 writes x, y (pixels, origin top left), the angle in
    # 2-degree units and the quality, as in an ANSI INCITS 378 record
    mindtct -m1 "$SAMPLE" "$WORK/mindtct" >/dev/null 2>&1
    "$TEST" --dump ANSI "$WORK/sample.ANSI" > "$WORK/converter.xyt"
    paste -d ' ' "$WORK/mindtct.xyt" "$WORK/converter.xyt" | awk -v n1="$(wc -l < "$WORK/mindtct.xyt")" \
        -v n2="$(wc -l < "$WORK/converter.xyt")" '
        function abs(v) { return v < 0 ? -v : v }
        {
            dx = abs($1 - $5); dy = abs($2 - $6); dq = abs($4 - $8)
            dt = abs($3 - $7); if (dt > 90) dt = 180 - dt
            if (dx > mx) mx = dx; if (dy > my) my = dy
            if (dt > mt) mt = dt; if (dq > mq) mq = dq
            if (dx == 0 && dy == 0 && dt == 0) same++
        }
        END {
            printf "  mindtct %d minutiae, converter %d; identical x, y and angle: %d\n", n1, n2, same
            printf "  largest difference: x %d px, y %d px, angle %d units, quality %d\n", mx, my, mt, mq
            exit !(n1 == n2 && mx <= 1 && my <= 1 && mt <= 1 && mq <= 2)
        }' || fail "minutiae differ from mindtct"
else
    echo "  skipped: mindtct not found"
fi

echo "== PNG input"
if have dwsq && have python3; then
    cp "$SAMPLE" "$WORK/png.wsq"
    dwsq raw "$WORK/png.wsq" -r >/dev/null 2>&1
    # Write the decoded pixels as PNG, without and with a pHYs chunk
    python3 - "$WORK/png.raw" 545 622 "$WORK" <<'EOF'
import struct, sys, zlib
raw, w, h, out = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
pix = open(raw, "rb").read()
rows = b"".join(b"\0" + pix[y * w:(y + 1) * w] for y in range(h))
def chunk(t, d):
    return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xffffffff)
def png(path, phys):
    data = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 0, 0, 0, 0))
    if phys:
        data += chunk(b"pHYs", struct.pack(">IIB", phys, phys, 1))
    data += chunk(b"IDAT", zlib.compress(rows)) + chunk(b"IEND", b"")
    open(path, "wb").write(data)
png(out + "/nores.png", 0)
png(out + "/1000ppi.png", 39370)
EOF
    "$CONVERT" -i "$WORK/nores.png" -ti PNG -o "$WORK/nores.ansi" -to ANSI >/dev/null 2>&1 ||
        fail "convert PNG without resolution"
    "$CONVERT" -i "$WORK/1000ppi.png" -ti PNG -o "$WORK/1000ppi.ansi" -to ANSI >/dev/null 2>&1 ||
        fail "convert PNG at 1000 ppi"
    r1=$(ansi_res "$WORK/nores.ansi")
    r2=$(ansi_res "$WORK/1000ppi.ansi")
    echo "  no pHYs chunk: $r1 px/cm; pHYs 39370 px/m (1000 ppi): $r2 px/cm"
    [ "$r1" = "197x197" ] || fail "PNG without resolution: $r1, expected 197x197"
    [ "$r2" = "394x394" ] || fail "PNG at 1000 ppi: $r2, expected 394x394"
    # Same pixels and resolution as the WSQ file: the same minutiae
    cmp -s "$WORK/nores.ansi" "$WORK/sample.ANSI" ||
        fail "PNG and WSQ of the same image give different records"
else
    echo "  skipped: dwsq or python3 not found"
fi

if [ "${VALGRIND:-0}" = 1 ]; then
    echo "== valgrind"
    for args in "-i $SAMPLE -ti WSQ -o $WORK/vg.iso -to ISO" \
                "-i $WORK/sample.ANSI -ti ANSI -o $WORK/vg.cc -to ISOCC" \
                "-i $WORK/sample.ISONC -ti ISONC -o $WORK/vg.iso -to ISO"; do
        # shellcheck disable=SC2086
        valgrind -q --leak-check=full --errors-for-leak-kinds=definite \
            --suppressions="$SRC/tests/valgrind.supp" \
            --error-exitcode=99 "$CONVERT" $args >/dev/null 2>"$WORK/vg.log" ||
            { cat "$WORK/vg.log"; fail "valgrind: convert $args"; }
    done
    echo "  done"
fi

if [ "$FAILED" -ne 0 ]; then
    echo "Some tests failed"
    exit 1
fi
echo "All tests passed"
