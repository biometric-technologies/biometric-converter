/*
 * Tests for libconverter.
 *
 *   test_converter <image>             run the checks below on an image
 *   test_converter --dump <type> <file> print the minutiae of a record as
 *                                      "x y angle quality" lines, with the
 *                                      angle in the record's own units
 *
 * Checks: image to every minutiae format; every fmr2fmr direction and round
 * trips, comparing minutiae positions and angles with the ANSI record made
 * from the image; error reporting; and several threads converting at once.
 */
#include <sys/queue.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <biomdi.h>
#include <biomdimacro.h>
#include <fmr.h>

#include <converter.h>

#define NTYPES 4
static const char *type_names[NTYPES] = {"ANSI", "ISO", "ISONC", "ISOCC"};
static const int type_stds[NTYPES] = {
        FMR_STD_ANSI, FMR_STD_ISO, FMR_STD_ISO_NORMAL_CARD, FMR_STD_ISO_COMPACT_CARD
};

// Resolution used for the card formats, pixels per centimetre
#define CARD_RES 197

#define MAX_MINUTIAE 256

struct minutia {
    double x, y;    // pixels
    double angle;   // degrees
};

static int failures = 0;

#define CHECK(cond, ...)                                    \
    do {                                                    \
        if (!(cond)) {                                      \
            failures++;                                     \
            printf("FAIL: ");                               \
            printf(__VA_ARGS__);                            \
            printf("\n");                                   \
        }                                                   \
    } while (0)

static unsigned char *
read_file(const char *path, int *len) {
    FILE *fp;
    unsigned char *buf;
    long n;

    if ((fp = fopen(path, "rb")) == NULL) {
        perror(path);
        exit(2);
    }
    fseek(fp, 0, SEEK_END);
    n = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    buf = malloc(n > 0 ? n : 1);
    if (fread(buf, 1, n, fp) != (size_t) n) {
        perror(path);
        exit(2);
    }
    fclose(fp);
    *len = (int) n;
    return buf;
}

static int
type_index(const char *name) {
    int i;

    for (i = 0; i < NTYPES; i++)
        if (strcmp(name, type_names[i]) == 0)
            return i;
    return -1;
}

// Parse a record with BiomDI; returns the FMR or NULL.
static FMR *
parse(const unsigned char *buf, int len, int t) {
    FMR *fmr;
    BDB bdb;

    if (new_fmr(type_stds[t], &fmr) != 0)
        return NULL;
    INIT_BDB(&bdb, (uint8_t *) buf, len);
    if (scan_fmr(&bdb, fmr) != READ_OK) {
        free_fmr(fmr);
        return NULL;
    }
    return fmr;
}

// Minutiae of a record in pixels and degrees; returns the count or -1.
static int
minutiae(const unsigned char *buf, int len, int t, struct minutia *m) {
    FMR *fmr;
    FVMR *fvmr;
    FMD *fmd;
    double xres, yres, unit, aunit;
    int n = 0;

    if ((fmr = parse(buf, len, t)) == NULL)
        return -1;
    if (type_stds[t] == FMR_STD_ISO_NORMAL_CARD)
        unit = 0.01;
    else if (type_stds[t] == FMR_STD_ISO_COMPACT_CARD)
        unit = 0.1;
    else
        unit = 0;    // pixels
    xres = unit > 0 ? CARD_RES : fmr->x_resolution;
    yres = unit > 0 ? CARD_RES : fmr->y_resolution;
    if (type_stds[t] == FMR_STD_ANSI)
        aunit = FMD_ANSI_ANGLE_UNIT;
    else if (type_stds[t] == FMR_STD_ISO_COMPACT_CARD)
        aunit = FMD_ISOCC_ANGLE_UNIT;
    else
        aunit = FMD_ISO_ANGLE_UNIT;

    fvmr = TAILQ_FIRST(&fmr->finger_views);
    if (fvmr != NULL) {
        TAILQ_FOREACH(fmd, &fvmr->minutiae_data, list) {
            if (n == MAX_MINUTIAE)
                break;
            if (unit > 0) {
                m[n].x = fmd->x_coord * unit * xres / 10.0;
                m[n].y = fmd->y_coord * unit * yres / 10.0;
            } else {
                m[n].x = fmd->x_coord;
                m[n].y = fmd->y_coord;
            }
            m[n].angle = fmd->angle * aunit;
            n++;
        }
    }
    free_fmr(fmr);
    return n;
}

static double
angle_diff(double a, double b) {
    double d = fmod(fabs(a - b), 360.0);
    return d > 180.0 ? 360.0 - d : d;
}

// The minutiae of the reference that fit into the compact card format
static int
isocc_subset(const struct minutia *ref, int n, struct minutia *out) {
    int i, k = 0;

    for (i = 0; i < n; i++) {
        double x = (10.0 * ref[i].x / CARD_RES) / 0.1;
        double y = (10.0 * ref[i].y / CARD_RES) / 0.1;
        if (((unsigned short) (x + 0.5) <= 255) && ((unsigned short) (y + 0.5) <= 255))
            out[k++] = ref[i];
    }
    return k;
}

// Compare minutiae index by index; updates the largest errors seen.
static void
compare(const char *what, const struct minutia *ref, int nref,
        const struct minutia *m, int n, double pos_tol, double ang_tol,
        double *max_pos, double *max_ang) {
    int i;
    double dp, da, mp = 0, ma = 0;

    CHECK(n == nref, "%s: %d minutiae, expected %d", what, n, nref);
    if (n != nref)
        return;
    for (i = 0; i < n; i++) {
        dp = fmax(fabs(m[i].x - ref[i].x), fabs(m[i].y - ref[i].y));
        da = angle_diff(m[i].angle, ref[i].angle);
        mp = fmax(mp, dp);
        ma = fmax(ma, da);
    }
    CHECK(mp <= pos_tol, "%s: position differs by %.2f px (limit %.2f)", what, mp, pos_tol);
    CHECK(ma <= ang_tol, "%s: angle differs by %.2f degrees (limit %.2f)", what, ma, ang_tol);
    *max_pos = fmax(*max_pos, mp);
    *max_ang = fmax(*max_ang, ma);
    printf("  %-16s %3d minutiae, max position error %.2f px, max angle error %.2f deg\n",
           what, n, mp, ma);
}

// Quantisation limits of a format: position in pixels at 500 ppi, angle in degrees
static double
pos_tol(int t) {
    return type_stds[t] == FMR_STD_ISO_COMPACT_CARD ? 1.0 : 0.5;
}

static double
ang_tol(int t) {
    switch (type_stds[t]) {
        case FMR_STD_ANSI:
            return 1.0;
        case FMR_STD_ISO_COMPACT_CARD:
            return 3.4;    // BiomDI rounds compact card angles up by half a degree
        default:
            return 0.71;
    }
}

struct thread_arg {
    unsigned char *image;
    int ilen;
    unsigned char *expected[NTYPES];
    int elen[NTYPES];
    int mismatches;
    int errors;
};

static void *
thread_main(void *p) {
    struct thread_arg *a = p;
    unsigned char *out;
    int olen, i, t;

    for (i = 0; i < 4; i++) {
        for (t = 0; t < NTYPES; t++) {
            if (img2fmr(a->image, a->ilen, (char *) type_names[t], &out, &olen) != 0) {
                a->errors++;
                continue;
            }
            if ((olen != a->elen[t]) || (memcmp(out, a->expected[t], olen) != 0))
                a->mismatches++;
            converter_free(out);
        }
        // Minutiae conversions and an error path at the same time
        if (fmr2fmr(a->expected[0], a->elen[0], &out, &olen, "ANSI", "ISOCC") != 0)
            a->errors++;
        else {
            if ((olen != a->elen[3]) || (memcmp(out, a->expected[3], olen) != 0))
                a->mismatches++;
            converter_free(out);
        }
        if ((img2fmr(a->expected[0], a->elen[0], "ANSI", &out, &olen) == 0) ||
            (converter_last_error()[0] == '\0'))
            a->errors++;
    }
    return NULL;
}

static int
dump(const char *type, const char *path) {
    unsigned char *buf;
    int len, t;
    FMR *fmr;
    FVMR *fvmr;
    FMD *fmd;

    if ((t = type_index(type)) < 0) {
        fprintf(stderr, "unknown type %s\n", type);
        return 2;
    }
    buf = read_file(path, &len);
    if ((fmr = parse(buf, len, t)) == NULL) {
        fprintf(stderr, "could not parse %s\n", path);
        return 1;
    }
    TAILQ_FOREACH(fvmr, &fmr->finger_views, list)
        TAILQ_FOREACH(fmd, &fvmr->minutiae_data, list)
            printf("%d %d %d %d\n", fmd->x_coord, fmd->y_coord, fmd->angle, fmd->quality);
    free_fmr(fmr);
    free(buf);
    return 0;
}

int
main(int argc, char *argv[]) {
    unsigned char *image, *rec[NTYPES], *out, *back;
    int ilen, rlen[NTYPES], olen, blen;
    struct minutia ref[MAX_MINUTIAE], refcc[MAX_MINUTIAE], m[MAX_MINUTIAE];
    int nref, nrefcc, n, t, u, i, nonzero, rc;
    double max_pos = 0, max_ang = 0, ptol, atol;
    char what[64];
    const struct minutia *exp;
    int nexp;
    pthread_t threads[8];
    struct thread_arg targ[8];

    if ((argc == 4) && (strcmp(argv[1], "--dump") == 0))
        return dump(argv[2], argv[3]);
    if (argc != 2) {
        fprintf(stderr, "usage: test_converter <image>\n"
                        "       test_converter --dump <type> <record>\n");
        return 2;
    }
    image = read_file(argv[1], &ilen);

    printf("Image to minutiae records\n");
    for (t = 0; t < NTYPES; t++) {
        rc = img2fmr(image, ilen, (char *) type_names[t], &rec[t], &rlen[t]);
        CHECK(rc == 0, "img2fmr to %s: %s", type_names[t], converter_last_error());
        if (rc != 0)
            return 1;
        CHECK(minutiae(rec[t], rlen[t], t, m) >= 0, "img2fmr to %s: BiomDI cannot parse the result",
              type_names[t]);
    }

    nref = minutiae(rec[0], rlen[0], 0, ref);
    CHECK(nref > 0, "no minutiae in the ANSI record");
    for (i = 0, nonzero = 0; i < nref; i++)
        if ((ref[i].x > 0) && (ref[i].y > 0))
            nonzero++;
    CHECK(nonzero == nref, "%d of %d ANSI minutiae have a zero coordinate", nref - nonzero, nref);
    nrefcc = isocc_subset(ref, nref, refcc);
    printf("  ANSI             %3d minutiae (%d within the compact card range)\n", nref, nrefcc);

    for (t = 1; t < NTYPES; t++) {
        n = minutiae(rec[t], rlen[t], t, m);
        snprintf(what, sizeof(what), "image->%s", type_names[t]);
        exp = type_stds[t] == FMR_STD_ISO_COMPACT_CARD ? refcc : ref;
        nexp = type_stds[t] == FMR_STD_ISO_COMPACT_CARD ? nrefcc : nref;
        compare(what, exp, nexp, m, n, pos_tol(t), ang_tol(0) + ang_tol(t), &max_pos, &max_ang);
    }

    printf("Minutiae record conversions (X->Y compared with the image minutiae; "
           "X->Y->X round trip)\n");
    for (t = 0; t < NTYPES; t++) {
        for (u = 0; u < NTYPES; u++) {
            snprintf(what, sizeof(what), "%s->%s", type_names[t], type_names[u]);
            rc = fmr2fmr(rec[t], rlen[t], &out, &olen, (char *) type_names[t],
                         (char *) type_names[u]);
            CHECK(rc == 0, "%s: %s", what, converter_last_error());
            if (rc != 0)
                continue;
            n = minutiae(out, olen, u, m);
            CHECK(n >= 0, "%s: BiomDI cannot parse the result", what);
            if ((type_stds[t] == FMR_STD_ISO_COMPACT_CARD) ||
                (type_stds[u] == FMR_STD_ISO_COMPACT_CARD)) {
                exp = refcc;
                nexp = nrefcc;
            } else {
                exp = ref;
                nexp = nref;
            }
            ptol = pos_tol(t) + pos_tol(u);
            atol = ang_tol(0) + ang_tol(t) + ang_tol(u);
            compare(what, exp, nexp, m, n, ptol, atol, &max_pos, &max_ang);

            // Round trip back to the input type
            snprintf(what, sizeof(what), "%s->%s->%s", type_names[t], type_names[u],
                     type_names[t]);
            rc = fmr2fmr(out, olen, &back, &blen, (char *) type_names[u],
                         (char *) type_names[t]);
            CHECK(rc == 0, "%s: %s", what, converter_last_error());
            if (rc == 0) {
                struct minutia orig[MAX_MINUTIAE];
                int norig = minutiae(rec[t], rlen[t], t, orig);
                n = minutiae(back, blen, t, m);
                if (type_stds[u] == FMR_STD_ISO_COMPACT_CARD)
                    norig = isocc_subset(orig, norig, orig);
                compare(what, orig, norig, m, n, 2 * pos_tol(u),
                        2 * (ang_tol(t) + ang_tol(u)), &max_pos, &max_ang);
                converter_free(back);
            }
            converter_free(out);
        }
    }
    printf("Largest position error %.2f px, largest angle error %.2f degrees\n",
           max_pos, max_ang);

    printf("Errors are reported, not fatal\n");
    rc = img2fmr((unsigned char *) "not an image", 12, "ANSI", &out, &olen);
    CHECK((rc == -1) && (out == NULL) && (olen == 0) && converter_last_error()[0] != '\0',
          "garbage image input");
    printf("  garbage image: %s\n", converter_last_error());
    rc = img2fmr(image, ilen, "XYZ", &out, &olen);
    CHECK((rc == -1) && (out == NULL), "unknown output type");
    printf("  unknown type: %s\n", converter_last_error());
    rc = fmr2fmr(rec[0], rlen[0] / 2, &out, &olen, "ANSI", "ISO");
    CHECK((rc == -1) && (out == NULL), "truncated record");
    printf("  truncated record: %s\n", converter_last_error());
    rc = fmr2fmr(image, ilen, &out, &olen, "ANSI", "ISO");
    CHECK((rc == -1) && (out == NULL), "image passed as a record");
    printf("  image as record: %s\n", converter_last_error());
    rc = fmr2fmr(rec[0], rlen[0], &out, &olen, "ANSI", "ISO");
    CHECK((rc == 0) && (converter_last_error()[0] == '\0'), "error message cleared on success");
    converter_free(out);

    printf("Concurrent calls from 8 threads\n");
    for (i = 0; i < 8; i++) {
        memset(&targ[i], 0, sizeof(targ[i]));
        targ[i].image = image;
        targ[i].ilen = ilen;
        for (t = 0; t < NTYPES; t++) {
            targ[i].expected[t] = rec[t];
            targ[i].elen[t] = rlen[t];
        }
        pthread_create(&threads[i], NULL, thread_main, &targ[i]);
    }
    for (i = 0; i < 8; i++) {
        pthread_join(threads[i], NULL);
        CHECK(targ[i].errors == 0, "thread %d: %d unexpected results", i, targ[i].errors);
        CHECK(targ[i].mismatches == 0, "thread %d: %d outputs differ", i, targ[i].mismatches);
    }

    for (t = 0; t < NTYPES; t++)
        converter_free(rec[t]);
    free(image);

    if (failures > 0) {
        printf("%d check(s) failed\n", failures);
        return 1;
    }
    printf("All checks passed\n");
    return 0;
}
