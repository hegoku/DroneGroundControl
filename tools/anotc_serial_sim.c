// ANOTC serial performance simulator.
//
// Build:
//   cc -O2 -Wall -Wextra -std=c11 -D_DARWIN_C_SOURCE \
//      -o /tmp/anotc_serial_sim tools/anotc_serial_sim.c
//
// Run:
//   /tmp/anotc_serial_sim --rate-hz 2000
//
// The program prints a PTY slave path. Start the app with:
//   DGC_EXTRA_SERIAL_PORTS=/dev/ttysXXX ./appDroneGroundControl
// then choose that port in the Serial selector and connect.

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#if !defined(CLOCK_MONOTONIC)
#define CLOCK_MONOTONIC 6
#endif

enum {
    ANOTC_HEAD = 0xAB,
    ANOTC_SRC = 0xFE,
    ANOTC_DST = 0x01,
    ANOTC_FRAME_EULER = 0x03,
    ANOTC_HEADER_SIZE = 6,
    ANOTC_CHECKSUM_SIZE = 2,
    EULER_PAYLOAD_SIZE = 7,
    EULER_FRAME_SIZE = ANOTC_HEADER_SIZE + EULER_PAYLOAD_SIZE + ANOTC_CHECKSUM_SIZE
};

static volatile sig_atomic_t g_stop = 0;

static void handle_signal(int signo)
{
    (void)signo;
    g_stop = 1;
}

static uint64_t now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((uint64_t)ts.tv_sec * 1000000000ULL) + (uint64_t)ts.tv_nsec;
}

static void sleep_until_ns(uint64_t deadline_ns)
{
    for (;;) {
        uint64_t current = now_ns();
        if (current >= deadline_ns) {
            return;
        }

        uint64_t remaining = deadline_ns - current;
        struct timespec req;
        req.tv_sec = (time_t)(remaining / 1000000000ULL);
        req.tv_nsec = (long)(remaining % 1000000000ULL);
        if (nanosleep(&req, NULL) == 0 || errno != EINTR) {
            return;
        }
    }
}

static void put_le16(uint8_t *dst, int16_t value)
{
    uint16_t raw = (uint16_t)value;
    dst[0] = (uint8_t)(raw & 0xFF);
    dst[1] = (uint8_t)((raw >> 8) & 0xFF);
}

static void append_checksum(uint8_t *frame, size_t frame_without_checksum_len)
{
    uint8_t sum = 0;
    uint8_t add = 0;
    for (size_t i = 0; i < frame_without_checksum_len; ++i) {
        sum = (uint8_t)(sum + frame[i]);
        add = (uint8_t)(add + sum);
    }
    frame[frame_without_checksum_len] = sum;
    frame[frame_without_checksum_len + 1] = add;
}

static void build_euler_frame(uint8_t *frame, uint32_t sequence)
{
    int16_t roll = (int16_t)((int32_t)(sequence % 7200) - 3600);
    int16_t pitch = (int16_t)((int32_t)((sequence * 2U) % 3600) - 1800);
    int16_t yaw = (int16_t)((sequence * 3U) % 36000U);

    frame[0] = ANOTC_HEAD;
    frame[1] = ANOTC_SRC;
    frame[2] = ANOTC_DST;
    frame[3] = ANOTC_FRAME_EULER;
    frame[4] = (uint8_t)(EULER_PAYLOAD_SIZE & 0xFF);
    frame[5] = (uint8_t)((EULER_PAYLOAD_SIZE >> 8) & 0xFF);

    put_le16(frame + 6, roll);
    put_le16(frame + 8, pitch);
    put_le16(frame + 10, yaw);
    frame[12] = 1;

    append_checksum(frame, ANOTC_HEADER_SIZE + EULER_PAYLOAD_SIZE);
}

static int write_all(int fd, const uint8_t *data, size_t size)
{
    size_t written = 0;
    while (written < size) {
        ssize_t rc = write(fd, data + written, size - written);
        if (rc < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        written += (size_t)rc;
    }
    return 0;
}

static void usage(const char *argv0)
{
    fprintf(stderr,
            "Usage: %s [--rate-hz N] [--batch N] [--duration-sec N]\n"
            "\n"
            "Defaults: --rate-hz 2000 --batch 20 --duration-sec 0\n"
            "duration-sec 0 means run until Ctrl-C.\n",
            argv0);
}

int main(int argc, char **argv)
{
    double rate_hz = 2000.0;
    unsigned batch_frames = 20;
    double duration_sec = 0.0;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--rate-hz") == 0 && i + 1 < argc) {
            rate_hz = strtod(argv[++i], NULL);
        } else if (strcmp(argv[i], "--batch") == 0 && i + 1 < argc) {
            batch_frames = (unsigned)strtoul(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--duration-sec") == 0 && i + 1 < argc) {
            duration_sec = strtod(argv[++i], NULL);
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            usage(argv[0]);
            return 0;
        } else {
            usage(argv[0]);
            return 2;
        }
    }

    if (rate_hz <= 0.0 || batch_frames == 0) {
        usage(argv[0]);
        return 2;
    }

    int master_fd = posix_openpt(O_RDWR | O_NOCTTY);
    if (master_fd < 0 || grantpt(master_fd) != 0 || unlockpt(master_fd) != 0) {
        perror("failed to create pseudo terminal");
        return 1;
    }

    char *slave_name = ptsname(master_fd);
    if (!slave_name) {
        perror("ptsname");
        close(master_fd);
        return 1;
    }

    struct termios tio;
    if (tcgetattr(master_fd, &tio) == 0) {
        cfmakeraw(&tio);
        tcsetattr(master_fd, TCSANOW, &tio);
    }

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    size_t batch_size = (size_t)batch_frames * EULER_FRAME_SIZE;
    uint8_t *batch = (uint8_t *)malloc(batch_size);
    if (!batch) {
        fprintf(stderr, "failed to allocate %zu bytes\n", batch_size);
        close(master_fd);
        return 1;
    }

    fprintf(stdout, "PTY slave: %s\n", slave_name);
    fprintf(stdout, "App env:   DGC_EXTRA_SERIAL_PORTS=%s\n", slave_name);
    fprintf(stdout, "Target:    %.0f frames/sec, %u frames/batch, %u bytes/frame\n",
            rate_hz,
            batch_frames,
            EULER_FRAME_SIZE);
    fflush(stdout);

    uint64_t start = 0;
    uint64_t next_deadline = 0;
    uint64_t last_report = 0;
    uint64_t sent_frames = 0;
    uint32_t sequence = 0;
    const uint64_t batch_period_ns = (uint64_t)(((double)batch_frames * 1000000000.0) / rate_hz);
    const uint64_t duration_ns = duration_sec > 0.0
        ? (uint64_t)(duration_sec * 1000000000.0)
        : 0;

    while (!g_stop) {
        for (unsigned i = 0; i < batch_frames; ++i) {
            build_euler_frame(batch + ((size_t)i * EULER_FRAME_SIZE), sequence++);
        }

        if (write_all(master_fd, batch, batch_size) != 0) {
            perror("write");
            break;
        }

        uint64_t current = now_ns();
        if (start == 0) {
            start = current;
            next_deadline = current;
            last_report = current;
        }

        sent_frames += batch_frames;
        next_deadline += batch_period_ns > 0 ? batch_period_ns : 1;

        if (duration_ns > 0 && current - start >= duration_ns) {
            break;
        }

        if (current < next_deadline) {
            sleep_until_ns(next_deadline);
        } else {
            next_deadline = current;
        }

        current = now_ns();
        if (current - last_report >= 1000000000ULL) {
            double elapsed = (double)(current - start) / 1000000000.0;
            double actual_rate = elapsed > 0.0 ? (double)sent_frames / elapsed : 0.0;
            fprintf(stderr,
                    "sent=%llu frames actual=%.0f Hz throughput=%.2f MB/s\n",
                    (unsigned long long)sent_frames,
                    actual_rate,
                    (actual_rate * (double)EULER_FRAME_SIZE) / (1024.0 * 1024.0));
            last_report = current;
        }
    }

    if (start != 0) {
        uint64_t end = now_ns();
        double elapsed = (double)(end - start) / 1000000000.0;
        double actual_rate = elapsed > 0.0 ? (double)sent_frames / elapsed : 0.0;
        fprintf(stderr,
                "FINAL sent=%llu frames elapsed=%.3f sec actual=%.0f Hz throughput=%.2f MB/s\n",
                (unsigned long long)sent_frames,
                elapsed,
                actual_rate,
                (actual_rate * (double)EULER_FRAME_SIZE) / (1024.0 * 1024.0));
    }

    free(batch);
    close(master_fd);
    return 0;
}
