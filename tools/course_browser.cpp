#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <windows.h>
#include <shellapi.h>
#include <bcrypt.h>
#include <cctype>
#include <cwctype>
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <vector>
namespace fs = std::filesystem;
std::string utf8(const std::wstring& s) {
  int n = WideCharToMultiByte(CP_UTF8, 0, s.data(), int(s.size()), nullptr, 0, nullptr, nullptr);
  std::string r(n, 0);
  WideCharToMultiByte(CP_UTF8, 0, s.data(), int(s.size()), r.data(), n, nullptr, nullptr);
  return r;
}
std::wstring wide(const std::string& s) {
  int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), int(s.size()), nullptr, 0);
  if (!n && !s.empty()) throw std::runtime_error("Invalid UTF-8");
  std::wstring r(n, 0);
  MultiByteToWideChar(CP_UTF8, 0, s.data(), int(s.size()), r.data(), n);
  return r;
}
std::string read(const fs::path& p) {
  std::ifstream f(p, std::ios::binary);
  return {std::istreambuf_iterator<char>(f), {}};
}
std::string quote(const std::string& s) {
  std::string o = "\"";
  for (unsigned char c : s) {
    if (c == '"' || c == '\\') o += '\\';
    if (c >= 32)
      o += char(c);
    else
      o += ' ';
  }
  return o + '"';
}
std::string lower(std::string s) {
  for (auto& c : s)
    if (c >= 'A' && c <= 'Z') c = char(c + 32);
  return s;
}
std::string html(const std::string& s) {
  std::string o;
  for (char c : s) {
    if (c == '&')
      o += "&amp;";
    else if (c == '<')
      o += "&lt;";
    else if (c == '"')
      o += "&quot;";
    else
      o += c;
  }
  return o;
}
std::string urlEncode(const std::string& s) {
  std::string o;
  const char* hex = "0123456789ABCDEF";
  for (unsigned char c : s) {
    if (isalnum(c) || c == '/' || c == '_' || c == '-' || c == '.')
      o += char(c);
    else {
      o += '%';
      o += hex[c >> 4];
      o += hex[c & 15];
    }
  }
  return o;
}
std::string trim(std::string s) {
  auto a = s.find_first_not_of(" \t\r\n"), b = s.find_last_not_of(" \t\r\n");
  return a == std::string::npos ? "" : s.substr(a, b - a + 1);
}
std::string decode(const std::string& s) {
  std::string r;
  for (size_t i = 0; i < s.size(); ++i) {
    if (s[i] == '%') {
      if (i + 2 >= s.size() || !isxdigit(static_cast<unsigned char>(s[i + 1])) ||
          !isxdigit(static_cast<unsigned char>(s[i + 2])))
        throw std::runtime_error("Bad URL");
      r += char(std::stoi(s.substr(i + 1, 2), nullptr, 16));
      i += 2;
    } else
      r += s[i];
  }
  if (r.find('\0') != r.npos || r.find('\\') != r.npos) throw std::runtime_error("Bad path");
  return r;
}
bool sendAll(SOCKET s, const std::string& bytes) {
  size_t at = 0;
  while (at < bytes.size()) {
    int n = send(s, bytes.data() + at, int(std::min(bytes.size() - at, size_t(65536))), 0);
    if (n <= 0) return false;
    at += size_t(n);
  }
  return true;
}
void reply(SOCKET s, int status, const std::string& body, const std::string& type = "application/json; charset=utf-8") {
  std::string head =
      "HTTP/1.1 " + std::to_string(status) + (status == 200 ? " OK" : " Error") + "\r\nContent-Type: " + type +
      "\r\nContent-Length: " + std::to_string(body.size()) +
      "\r\nConnection: close\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\nContent-Security-Policy: "
      "default-src 'self'; script-src 'self'; style-src 'self' 'unsafe-inline'; img-src 'self' data:; object-src "
      "'none'; base-uri 'none'; frame-ancestors 'none'\r\n\r\n";
  if (sendAll(s, head)) sendAll(s, body);
}
struct Child {
  HANDLE handle = nullptr;
  DWORD id = 0;
};
struct Browser {
  fs::path root;
  std::string token, host;
  std::vector<std::string> lessons;
  std::map<std::string, std::string> trackDirs;
  std::map<std::string, Child> children;
  bool serving = true;
  ~Browser() {
    for (auto& p : children)
      if (p.second.handle) CloseHandle(p.second.handle);
  }
  fs::path exe(const std::string& lesson, const std::string& config) const {
    return root / wide(trackDirs.at(lesson)) / L"build" / L"bin" / wide(config) / wide(lesson + ".exe");
  }
  fs::path log(const std::string& key) const { return root / L"build" / L"launcher-logs" / wide(key + ".log"); }
  std::string status() {
    std::string s = "{\"token\":" + quote(token) + ",\"lessons\":[";
    bool first = true;
    for (const auto& lesson : lessons)
      for (const auto& config : {std::string("Debug"), std::string("Release")}) {
        if (!first) s += ',';
        first = false;
        auto key = lesson + "-" + config;
        auto c = children.find(key);
        DWORD code = 0;
        bool running = false, failed = false;
        if (c != children.end() && GetExitCodeProcess(c->second.handle, &code)) {
          running = code == STILL_ACTIVE;
          failed = !running && code != 0;
        }
        s += "{\"started\":" + std::string(c != children.end() ? "true" : "false") + ",\"name\":" + quote(lesson) +
             ",\"config\":" + quote(config) +
             ",\"available\":" + (fs::is_regular_file(exe(lesson, config)) ? "true" : "false") +
             ",\"running\":" + (running ? "true" : "false") + ",\"failed\":" + (failed ? "true" : "false") +
             ",\"exitCode\":" + std::to_string(code) + "}";
      }
    return s + "]}";
  }
  bool valid(const std::string& lesson, const std::string& config) const {
    return std::find(lessons.begin(), lessons.end(), lesson) != lessons.end() &&
           (config == "Debug" || config == "Release");
  }
  void run(SOCKET socket, const std::string& body) {
    auto pos = body.find('|');
    if (pos == body.npos) {
      reply(socket, 400, "{}");
      return;
    }
    auto lesson = body.substr(0, pos), config = body.substr(pos + 1);
    if (!valid(lesson, config)) {
      reply(socket, 400, "{\"message\":\"课程不存在\"}");
      return;
    }
    auto key = lesson + "-" + config;
    auto& child = children[key];
    DWORD code = 0;
    if (child.handle && GetExitCodeProcess(child.handle, &code) && code == STILL_ACTIVE) {
      reply(socket, 200, "{\"message\":\"该课程已在运行，请切换到示例窗口\"}");
      return;
    }
    if (!fs::is_regular_file(exe(lesson, config))) {
      reply(socket, 404, "{\"message\":\"该版本尚未编译，请先构建教程\"}");
      return;
    }
    if (child.handle) {
      CloseHandle(child.handle);
      child.handle = nullptr;
    }
    fs::create_directories(log(key).parent_path());
    SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, TRUE};
    HANDLE output = CreateFileW(log(key).c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL, nullptr);
    HANDLE input =
        CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, nullptr);
    if (output == INVALID_HANDLE_VALUE || input == INVALID_HANDLE_VALUE) {
      if (output != INVALID_HANDLE_VALUE) CloseHandle(output);
      if (input != INVALID_HANDLE_VALUE) CloseHandle(input);
      reply(socket, 500, "{\"message\":\"无法创建运行日志\"}");
      return;
    }
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = output;
    si.hStdError = output;
    si.hStdInput = input;
    PROCESS_INFORMATION pi{};
    auto path = exe(lesson, config);
    std::wstring command = L"\"" + path.wstring() + L"\"";
    BOOL ok = CreateProcessW(path.c_str(), command.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr,
                             path.parent_path().c_str(), &si, &pi);
    CloseHandle(output);
    CloseHandle(input);
    if (!ok) {
      reply(socket, 500, "{\"message\":\"Windows 无法启动程序\"}");
      return;
    }
    CloseHandle(pi.hThread);
    child = {pi.hProcess, pi.dwProcessId};
    reply(socket, 200, "{\"message\":\"已启动，Esc 退出示例\"}");
  }
  void request(SOCKET socket) {
    DWORD timeout = 2000;
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&timeout), sizeof(timeout));
    setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<char*>(&timeout), sizeof(timeout));
    std::string data;
    std::array<char, 4096> buffer{};
    size_t end;
    while ((end = data.find("\r\n\r\n")) == data.npos) {
      int n = recv(socket, buffer.data(), int(buffer.size()), 0);
      if (n <= 0) return;
      data.append(buffer.data(), n);
      if (data.size() > 16384) {
        reply(socket, 400, "{}");
        return;
      }
    }
    std::istringstream lines(data.substr(0, end));
    std::string method, path, version, line;
    lines >> method >> path >> version;
    std::getline(lines, line);
    std::map<std::string, std::string> headers;
    while (std::getline(lines, line)) {
      auto p = line.find(':');
      if (p != line.npos) {
        auto key = lower(trim(line.substr(0, p)));
        if (headers.count(key)) {
          reply(socket, 400, "{}");
          return;
        }
        headers[key] = trim(line.substr(p + 1));
      }
    }
    if (headers["host"] != host) {
      reply(socket, 403, "{}");
      return;
    }
    if (headers.count("transfer-encoding")) {
      reply(socket, 400, "{}");
      return;
    }
    size_t length = 0;
    if (headers.count("content-length")) {
      if (!std::regex_match(headers["content-length"], std::regex("[0-9]{1,3}"))) {
        reply(socket, 400, "{}");
        return;
      }
      length = std::stoul(headers["content-length"]);
    }
    if (length > 128) {
      reply(socket, 400, "{}");
      return;
    }
    auto body = data.substr(end + 4);
    while (body.size() < length) {
      int n = recv(socket, buffer.data(), int(std::min(length - body.size(), buffer.size())), 0);
      if (n <= 0) return;
      body.append(buffer.data(), n);
    }
    body.resize(length);
    if (method == "POST") {
      if (headers["x-course-token"] != token || (headers.count("origin") && headers["origin"] != "http://" + host)) {
        reply(socket, 403, "{}");
        return;
      }
      if (path == "/api/run")
        run(socket, body);
      else if (path == "/api/shutdown") {
        reply(socket, 200, "{}");
        serving = false;
      } else
        reply(socket, 404, "{}");
      return;
    }
    if (method != "GET") {
      reply(socket, 405, "{}");
      return;
    }
    if (path == "/api/status") {
      reply(socket, 200, status());
      return;
    }
    if (path.rfind("/api/log/", 0) == 0) {
      auto key = path.substr(9);
      auto p = key.rfind('-');
      if (p == key.npos || !valid(key.substr(0, p), key.substr(p + 1))) {
        reply(socket, 404, "{}");
        return;
      }
      reply(socket, 200, read(log(key)), "text/plain; charset=utf-8");
      return;
    }
    auto q = path.find('?');
    if (q != path.npos) path.resize(q);
    path = decode(path);
    if (path == "/") path = "/index.html";
    if (path.empty() || path[0] != '/') {
      reply(socket, 404, "{}");
      return;
    }
    fs::path relative(wide(path.substr(1)));
    for (const auto& part : relative)
      if (part == ".." || part == "." || part == ".git" || part == "build" || part == "backups" ||
          part.wstring().find(':') != part.wstring().npos) {
        reply(socket, 403, "{}");
        return;
      }
    auto first = utf8(relative.begin()->wstring());
    bool allowed =
        (first == "OpenGL" || first == "Vulkan" || first == "DirectX12" || first == "docs" || first == "web" ||
         first == "shared" || path == "/index.html" || path == "/README.html" || path == "/THIRD_PARTY.html" || path == "/LICENSE" || path == "/NOTICE");
    auto file = fs::weakly_canonical(root / relative);
    auto actual = fs::relative(file, root);
    for (const auto& part : actual)
      if (part == "..") allowed = false;
    if (allowed && fs::is_directory(file)) {
      std::string listing =
          "<!doctype html><html lang=\"zh-CN\"><meta "
          "charset=\"utf-8\"><title>课程文件</title><style>body{background:#101722;color:#dce5ef;font:17px/1.9 "
          "system-ui;max-width:1000px;margin:40px auto}a{color:#65c8ff}</style><h1>课程文件</h1><p><a "
          "href=\"/index.html\">课程首页</a></p><ul>";
      std::vector<fs::path> entries;
      for (const auto& item : fs::directory_iterator(file)) entries.push_back(item.path());
      std::sort(entries.begin(), entries.end());
      for (const auto& item : entries) {
        auto rel = fs::relative(item, root);
        bool safe = true;
        for (const auto& part : rel)
          if (part == "..") safe = false;
        if (!safe) continue;
        auto link = "/" + urlEncode(utf8(rel.generic_wstring()));
        listing += "<li><a href=\"" + html(link) + "\">" + html(utf8(item.filename().wstring())) +
                   (fs::is_directory(item) ? "/" : "") + "</a></li>";
      }
      reply(socket, 200, listing + "</ul></html>", "text/html; charset=utf-8");
      return;
    }
    if (!allowed || !fs::is_regular_file(file)) {
      reply(socket, 404, "{}");
      return;
    }
    auto ext = lower(utf8(file.extension().wstring()));
    std::string type = "text/plain; charset=utf-8";
    if (ext == ".html")
      type = "text/html; charset=utf-8";
    else if (ext == ".js")
      type = "text/javascript; charset=utf-8";
    else if (ext == ".css")
      type = "text/css; charset=utf-8";
    else if (ext == ".png")
      type = "image/png";
    else if (ext == ".jpg" || ext == ".jpeg")
      type = "image/jpeg";
    else if (ext == ".json" || ext == ".gltf")
      type = "application/json; charset=utf-8";
    reply(socket, 200, read(file), type);
  }
};
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR args, int) try {
  wchar_t filename[32768];
  GetModuleFileNameW(nullptr, filename, 32768);
  fs::path root = fs::path(filename).parent_path();
  while (!root.empty() && !(fs::exists(root / L"catalog.tsv") && fs::is_directory(root / L"OpenGL"))) {
    auto parent = root.parent_path();
    if (parent == root) throw std::runtime_error("Place the launcher inside the tutorial directory");
    root = parent;
  }
  if (root.empty()) throw std::runtime_error("Tutorial directory not found");
  unsigned long long hash = 1469598103934665603ull;
  for (wchar_t c : root.wstring()) {
    hash ^= towlower(c);
    hash *= 1099511628211ull;
  }
  std::wstring mutexName = L"Local\\OpenGLCourseBrowser-" + std::to_wstring(hash);
  HANDLE mutex = CreateMutexW(nullptr, TRUE, mutexName.c_str());
  bool already = GetLastError() == ERROR_ALREADY_EXISTS;
  if (!mutex) throw std::runtime_error("Cannot create launcher lock");
  bool noBrowser = std::wstring(args).find(L"--no-browser") != std::wstring::npos;
  auto portFile = root / L"build" / L"course-browser.port";
  fs::create_directories(portFile.parent_path());
  if (already) {
    for (int i = 0; i < 20 && !fs::exists(portFile); ++i) Sleep(50);
    auto port = trim(read(portFile));
    if (!std::regex_match(port, std::regex("[0-9]{1,5}"))) throw std::runtime_error("Launcher is starting; try again");
    if (!noBrowser) {
      auto url = wide("http://127.0.0.1:" + port + "/");
      ShellExecuteW(nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
    CloseHandle(mutex);
    return 0;
  }
  WSADATA wsa;
  if (WSAStartup(MAKEWORD(2, 2), &wsa)) throw std::runtime_error("Winsock failed");
  SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  SetHandleInformation(reinterpret_cast<HANDLE>(listener), HANDLE_FLAG_INHERIT, 0);
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = 0;
  if (bind(listener, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR ||
      listen(listener, 8) == SOCKET_ERROR)
    throw std::runtime_error("Cannot start local tutorial server");
  int size = sizeof(addr);
  getsockname(listener, reinterpret_cast<sockaddr*>(&addr), &size);
  Browser server;
  server.root = root;
  server.host = "127.0.0.1:" + std::to_string(ntohs(addr.sin_port));
  std::array<unsigned char, 32> random{};
  if (BCryptGenRandom(nullptr, random.data(), ULONG(random.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0)
    throw std::runtime_error("Cannot create launch token");
  const char* hex = "0123456789abcdef";
  for (auto c : random) {
    server.token += hex[c >> 4];
    server.token += hex[c & 15];
  }
  std::istringstream catalog(read(root / L"catalog.tsv"));
  std::string row;
  while (std::getline(catalog, row)) {
    auto a = row.find('\t'), b = row.find('\t', a + 1);
    if (a == row.npos || b == row.npos) continue;
    auto name = row.substr(0, a), folder = row.substr(a + 1, b - a - 1);
    if (!std::regex_match(name, std::regex("(vk|nv|dx)?[0-9]{2}_[a-z0-9_]+"))) continue;
    if (folder != "OpenGL" && folder != "Vulkan/native" && folder != "Vulkan/nvpro" && folder != "DirectX12") continue;
    server.lessons.push_back(name);
    server.trackDirs[name] = folder;
  }
  std::sort(server.lessons.begin(), server.lessons.end());
  {
    std::ofstream f(portFile);
    f << ntohs(addr.sin_port);
  }
  if (!noBrowser) {
    auto url = wide("http://" + server.host + "/");
    ShellExecuteW(nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
  }
  while (server.serving) {
    SOCKET client = accept(listener, nullptr, nullptr);
    if (client == INVALID_SOCKET) break;
    SetHandleInformation(reinterpret_cast<HANDLE>(client), HANDLE_FLAG_INHERIT, 0);
    try {
      server.request(client);
    } catch (const std::exception&) {
      reply(client, 400, "{}");
    }
    closesocket(client);
  }
  closesocket(listener);
  WSACleanup();
  fs::remove(portFile);
  ReleaseMutex(mutex);
  CloseHandle(mutex);
  return 0;
} catch (const std::exception& e) {
  MessageBoxW(nullptr, wide(e.what()).c_str(), L"OpenGL 教程启动器", MB_OK | MB_ICONERROR);
  return 1;
}
