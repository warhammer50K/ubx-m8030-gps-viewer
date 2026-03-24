#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <chrono>
#include <thread>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

// ── GPS data ──
struct GPS {
    double lat = 0, lon = 0, alt = 0;
    double speed = 0, heading = 0, hdop = 0;
    int fix = 0, sats = 0;
    bool valid = false;
};

static GPS g_gps;
static int g_gga_count = 0;

// ── helpers ──
static speed_t to_speed(int b) {
    switch(b) {
        case 4800:   return B4800;
        case 9600:   return B9600;
        case 19200:  return B19200;
        case 38400:  return B38400;
        case 57600:  return B57600;
        case 115200: return B115200;
        case 230400: return B230400;
        default:     return B9600;
    }
}

static int open_serial(const char* port, int baud) {
    int fd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if(fd < 0) { perror("open"); return -1; }

    struct termios tty{};
    tcgetattr(fd, &tty);
    speed_t spd = to_speed(baud);
    cfsetispeed(&tty, spd);
    cfsetospeed(&tty, spd);
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag &= ~(PARENB | CSTOPB | CRTSCTS);
    tty.c_cflag |= CREAD | CLOCAL;
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY | IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
    tty.c_oflag &= ~OPOST;
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 1;
    tcsetattr(fd, TCSANOW, &tty);
    tcflush(fd, TCIOFLUSH);
    return fd;
}

static std::vector<std::string> split(const std::string& s) {
    std::vector<std::string> f;
    std::string cur;
    for(char c : s) {
        if(c == ',' || c == '*') { f.push_back(cur); cur.clear(); if(c == '*') break; }
        else cur += c;
    }
    return f;
}

static bool verify_checksum(const std::string& line) {
    auto d = line.find('$'), s = line.find('*');
    if(d == std::string::npos || s == std::string::npos || s <= d + 1) return false;
    uint8_t calc = 0;
    for(size_t i = d + 1; i < s; i++) calc ^= (uint8_t)line[i];
    if(s + 2 >= line.size()) return false;
    unsigned exp = 0;
    try { exp = std::stoul(line.substr(s + 1, 2), nullptr, 16); } catch(...) { return false; }
    return calc == exp;
}

static double parse_latlon(const std::string& v, const std::string& dir) {
    if(v.empty() || dir.empty()) return 0;
    double raw = std::stod(v);
    int deg = (int)(raw / 100);
    double result = deg + (raw - deg * 100) / 60.0;
    if(dir == "S" || dir == "W") result = -result;
    return result;
}

static void parse_gga(const std::vector<std::string>& f) {
    if(f.size() < 10) return;
    if(!f[6].empty()) g_gps.fix = std::stoi(f[6]);
    if(!f[7].empty()) g_gps.sats = std::stoi(f[7]);
    if(!f[8].empty()) g_gps.hdop = std::stod(f[8]);
    if(g_gps.fix > 0 && !f[2].empty() && !f[4].empty()) {
        g_gps.lat = parse_latlon(f[2], f[3]);
        g_gps.lon = parse_latlon(f[4], f[5]);
        if(!f[9].empty()) g_gps.alt = std::stod(f[9]);
        g_gps.valid = true;
    } else {
        g_gps.valid = false;
    }
    g_gga_count++;
}

static void parse_rmc(const std::vector<std::string>& f) {
    if(f.size() < 8) return;
    if(!f[7].empty()) g_gps.speed = std::stod(f[7]) * 0.514444;
    if(f.size() > 8 && !f[8].empty()) g_gps.heading = std::stod(f[8]);
}

static void parse_line(const std::string& line) {
    if(!verify_checksum(line)) return;
    auto d = line.find('$');
    if(d == std::string::npos) return;
    auto fields = split(line.substr(d));
    if(fields.empty()) return;
    const auto& id = fields[0];
    if(id.size() >= 5 && id.substr(id.size() - 3) == "GGA") parse_gga(fields);
    if(id.size() >= 5 && id.substr(id.size() - 3) == "RMC") parse_rmc(fields);
}

// ── auto detect ──
static std::string detect_port() {
    const char* prefixes[] = {"/dev/ttyUSB", "/dev/ttyACM"};
    for(auto* pfx : prefixes) {
        for(int i = 0; i < 10; i++) {
            std::string p = std::string(pfx) + std::to_string(i);
            if(access(p.c_str(), F_OK) == 0) return p;
        }
    }
    return "";
}

static int detect_baud(const std::string& port) {
    int bauds[] = {9600, 38400, 115200, 4800};
    for(int b : bauds) {
        int fd = open_serial(port.c_str(), b);
        if(fd < 0) continue;
        char buf[512];
        std::string acc;
        auto t0 = std::chrono::steady_clock::now();
        bool found = false;
        while(!found) {
            auto el = std::chrono::steady_clock::now() - t0;
            if(std::chrono::duration_cast<std::chrono::milliseconds>(el).count() > 2000) break;
            ssize_t n = read(fd, buf, sizeof(buf) - 1);
            if(n > 0) { acc.append(buf, n); if(acc.find("$GP") != std::string::npos || acc.find("$GN") != std::string::npos) found = true; }
            else std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        close(fd);
        if(found) return b;
    }
    return 9600;
}

// ── main ──
int main(int argc, char** argv) {
    std::string port;
    int baud = 0;

    if(argc >= 2) port = argv[1];
    if(argc >= 3) baud = std::atoi(argv[2]);

    if(port.empty()) {
        printf("[*] auto-detecting port...\n");
        port = detect_port();
        if(port.empty()) { fprintf(stderr, "[!] no GPS device found\n"); return 1; }
    }
    printf("[*] port: %s\n", port.c_str());

    if(baud <= 0) {
        printf("[*] auto-detecting baud...\n");
        baud = detect_baud(port);
    }
    printf("[*] baud: %d\n", baud);

    int fd = open_serial(port.c_str(), baud);
    if(fd < 0) return 1;

    printf("[*] listening... (Ctrl+C to quit)\n\n");

    char buf[1024];
    std::string linebuf;

    while(true) {
        ssize_t n = ::read(fd, buf, sizeof(buf) - 1);
        if(n > 0) {
            linebuf.append(buf, n);
            size_t pos;
            while((pos = linebuf.find('\n')) != std::string::npos) {
                std::string line = linebuf.substr(0, pos);
                linebuf.erase(0, pos + 1);
                if(!line.empty() && line.back() == '\r') line.pop_back();
                if(!line.empty() && line[0] == '$') {
                    try { parse_line(line); } catch(...) {}

                    // print every GGA
                    if(line.find("GGA") != std::string::npos) {
                        printf("\033[2K\r");  // clear line
                        if(g_gps.valid)
                            printf("[#%d] fix=%d sats=%d  lat=%.8f  lon=%.8f  alt=%.1fm  hdop=%.1f  spd=%.1fm/s  hdg=%.1f",
                                   g_gga_count, g_gps.fix, g_gps.sats,
                                   g_gps.lat, g_gps.lon, g_gps.alt,
                                   g_gps.hdop, g_gps.speed, g_gps.heading);
                        else
                            printf("[#%d] no fix (sats=%d)", g_gga_count, g_gps.sats);
                        fflush(stdout);
                    }
                }
            }
            if(linebuf.size() > 4096) linebuf.clear();
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    close(fd);
    return 0;
}
