#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <time.h>
#include <sys/time.h>
#include <linux/uinput.h>

#define SERIAL_PORT "/dev/ttyACM0"
#define BAUD_RATE B115200

long long get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)(tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;
}

void emit(int fd, int type, int code, int val) {
    struct input_event ie;
    memset(&ie, 0, sizeof(ie));
    ie.type = type;
    ie.code = code;
    ie.value = val;
    // timestamp is ignored by uinput anyway
    write(fd, &ie, sizeof(ie));
}

int setup_uinput() {
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if(fd < 0) {
        perror("Could not open /dev/uinput (run with sudo)");
        return -1;
    }

    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_KEYBIT, KEY_LEFT);
    ioctl(fd, UI_SET_KEYBIT, KEY_RIGHT);
    ioctl(fd, UI_SET_KEYBIT, KEY_UP);
    ioctl(fd, UI_SET_KEYBIT, KEY_DOWN);
    ioctl(fd, UI_SET_KEYBIT, KEY_ENTER);
    ioctl(fd, UI_SET_KEYBIT, KEY_LEFTCTRL);
    ioctl(fd, UI_SET_KEYBIT, KEY_ESC);
    ioctl(fd, UI_SET_KEYBIT, KEY_J);
    ioctl(fd, UI_SET_KEYBIT, KEY_SPACE);

    struct uinput_user_dev uud;
    memset(&uud, 0, sizeof(uud));
    snprintf(uud.name, UINPUT_MAX_NAME_SIZE, "Arduino Virtual Keyboard");
    uud.id.bustype = BUS_USB;
    uud.id.vendor  = 0x1234;
    uud.id.product = 0x5678;
    uud.id.version = 1;

    if (write(fd, &uud, sizeof(uud)) < 0) {
        perror("Failed to write to uinput device");
        close(fd);
        return -1;
    }

    if (ioctl(fd, UI_DEV_CREATE) < 0) {
        perror("Failed to create uinput device");
        close(fd);
        return -1;
    }

    return fd;
}

int configure_serial(int fd) {
    struct termios tty;
    if(tcgetattr(fd, &tty) != 0) return -1;

    cfsetospeed(&tty, BAUD_RATE);
    cfsetispeed(&tty, BAUD_RATE);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_iflag &= ~IGNBRK;
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 1; // 0.1s timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) return -1;
    return 0;
}

int main() {
    int uinp_fd = setup_uinput();
    if (uinp_fd < 0) return 1;

    int serial_fd = -1;
    printf("Setting up Virtual Keyboard Controller...\n");

    long long last_heartbeat = 0;

    char buf[256];
    int n = 0;

    int last_left = 0, last_right = 0, last_up = 0, last_down = 0;
    int last_btn1 = 0, last_btn2 = 0, last_btn3 = 0, last_btn4 = 0, last_joySw = 0;

    while (1) {
        if (serial_fd < 0) {
            serial_fd = open(SERIAL_PORT, O_RDWR | O_NOCTTY);
            if (serial_fd < 0) {
                printf("Waiting for Arduino Uno on %s...\n", SERIAL_PORT);
                sleep(2);
                continue;
            }
            configure_serial(serial_fd);
            printf("Connected to %s! Ready to play.\n", SERIAL_PORT);
        }

        long long now = get_time_ms();
        if (now - last_heartbeat > 250) {
            int res = write(serial_fd, "CONNECTED\n", 10);
            if (res < 0) {
                close(serial_fd);
                serial_fd = -1;
                continue;
            }
            last_heartbeat = now;
        }

        char c;
        int res = read(serial_fd, &c, 1);
        if (res > 0) {
            if (c == '\n' || c == '\r') {
                buf[n] = '\0';
                if (n > 0) {
                    int joyX, joyY, joySw, btn1, btn2, btn3, btn4;
                    if (sscanf(buf, "%d,%d,%d,%d,%d,%d,%d", &joyX, &joyY, &joySw, &btn1, &btn2, &btn3, &btn4) == 7) {
                        
                        joyX = 1023 - joyX; // invert X axis (left/right)
                        
                        int left = (joyX < 300) ? 1 : 0;
                        int right = (joyX > 700) ? 1 : 0;
                        int up = (joyY < 300) ? 1 : 0;
                        int down = (joyY > 700) ? 1 : 0;

                        int state_changed = 0;

                        if (left != last_left) { emit(uinp_fd, EV_KEY, KEY_LEFT, left); last_left = left; state_changed = 1; }
                        if (right != last_right) { emit(uinp_fd, EV_KEY, KEY_RIGHT, right); last_right = right; state_changed = 1; }
                        if (up != last_up) { emit(uinp_fd, EV_KEY, KEY_UP, up); last_up = up; state_changed = 1; }
                        if (down != last_down) { emit(uinp_fd, EV_KEY, KEY_DOWN, down); last_down = down; state_changed = 1; }

                        // Buttons mapped for conjuring_realistic_sprites_project (Joueur 1):
                        // btn1 (Button 1) -> Attack (Enter)
                        if (btn1 != last_btn1) { emit(uinp_fd, EV_KEY, KEY_ENTER, btn1); last_btn1 = btn1; state_changed = 1; }
                        // btn2 (Button 2) -> Sprint (Left Ctrl)
                        if (btn2 != last_btn2) { emit(uinp_fd, EV_KEY, KEY_LEFTCTRL, btn2); last_btn2 = btn2; state_changed = 1; }
                        // btn3 (Button 3) -> Quitter / Pause (Esc)
                        if (btn3 != last_btn3) { emit(uinp_fd, EV_KEY, KEY_ESC, btn3); last_btn3 = btn3; state_changed = 1; }
                        // btn4 (Button 4) -> Joueur 2 apparaitre (J)
                        if (btn4 != last_btn4) { emit(uinp_fd, EV_KEY, KEY_J, btn4); last_btn4 = btn4; state_changed = 1; }
                        // joySw (Joystick Button) -> Jump alternatife (Space)
                        if (joySw != last_joySw) { emit(uinp_fd, EV_KEY, KEY_SPACE, joySw); last_joySw = joySw; state_changed = 1; }

                        if (state_changed) {
                            emit(uinp_fd, EV_SYN, SYN_REPORT, 0);
                        }
                    }
                }
                n = 0; // reset buffer
            } else {
                if (n < 255) buf[n++] = c;
            }
        } else if (res < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            // Disconnected
            close(serial_fd);
            serial_fd = -1;
        }
    }

    ioctl(uinp_fd, UI_DEV_DESTROY);
    close(uinp_fd);
    if(serial_fd >= 0) close(serial_fd);
    return 0;
}
