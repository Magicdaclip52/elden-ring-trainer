#include "memory/ProcessMemory.h"

#include "core/Logger.h"
#include "core/Utf8.h"

#include <sstream>

namespace er {

std::string lastWinError() {
    const DWORD code = GetLastError();
    wchar_t* buf = nullptr;
    const DWORD n = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&buf), 0, nullptr);
    std::ostringstream o;
    o << "Win32 " << code;
    if (n && buf) {
        std::wstring w(buf, buf + n);
        while (!w.empty() && (w.back() == L'\r' || w.back() == L'\n' || w.back() == L' ')) {
            w.pop_back();
        }
        o << ": " << wideToUtf8(w);
        LocalFree(buf);
    }
    return o.str();
}

ProcessMemory::~ProcessMemory() {
    restoreAll();
    close();
}

bool ProcessMemory::open(DWORD pid) {
    close();
    HANDLE h = OpenProcess(
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION,
        FALSE, pid);
    if (!h) {
        Logger::instance().error("OpenProcess pid=" + std::to_string(pid) + " failed: " + lastWinError());
        return false;
    }
    handle_ = h;
    pid_ = pid;
    return true;
}

void ProcessMemory::close() {
    if (handle_) {
        CloseHandle(handle_);
        handle_ = nullptr;
    }
    pid_ = 0;
    originals_.clear();
}

bool ProcessMemory::read(uptr addr, void* dst, usize size) const {
    if (!handle_ || !dst || size == 0) {
        return false;
    }
    SIZE_T got = 0;
    if (!ReadProcessMemory(handle_, reinterpret_cast<LPCVOID>(addr), dst, size, &got) || got != size) {
        return false;
    }
    return true;
}

bool ProcessMemory::write(uptr addr, const void* src, usize size) {
    if (!handle_ || !src || size == 0) {
        return false;
    }
    SIZE_T put = 0;
    if (WriteProcessMemory(handle_, reinterpret_cast<LPVOID>(addr), src, size, &put) && put == size) {
        FlushInstructionCache(handle_, reinterpret_cast<LPCVOID>(addr), size);
        return true;
    }

    DWORD oldProtect = 0;
    if (!VirtualProtectEx(handle_, reinterpret_cast<LPVOID>(addr), size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        Logger::instance().error("VirtualProtectEx failed at 0x" + toHex(addr) + " " + lastWinError());
        return false;
    }
    put = 0;
    const bool ok = WriteProcessMemory(handle_, reinterpret_cast<LPVOID>(addr), src, size, &put) && put == size;
    DWORD tmp = 0;
    VirtualProtectEx(handle_, reinterpret_cast<LPVOID>(addr), size, oldProtect, &tmp);
    if (ok) {
        FlushInstructionCache(handle_, reinterpret_cast<LPCVOID>(addr), size);
    } else {
        Logger::instance().error("WriteProcessMemory failed at 0x" + toHex(addr) + " " + lastWinError());
    }
    return ok;
}

bool ProcessMemory::patch(uptr addr, std::span<const u8> bytes) {
    if (bytes.empty()) {
        return false;
    }
    if (!originals_.contains(addr)) {
        std::vector<u8> orig(bytes.size());
        if (!read(addr, orig.data(), orig.size())) {
            Logger::instance().error("patch: cannot read original bytes");
            return false;
        }
        originals_.emplace(addr, std::move(orig));
    }
    return write(addr, bytes.data(), bytes.size());
}

bool ProcessMemory::restore(uptr addr) {
    const auto it = originals_.find(addr);
    if (it == originals_.end()) {
        return true;
    }
    const bool ok = write(addr, it->second.data(), it->second.size());
    if (ok) {
        originals_.erase(it);
    }
    return ok;
}

void ProcessMemory::restoreAll() {
    if (!handle_) {
        originals_.clear();
        return;
    }
    for (auto it = originals_.begin(); it != originals_.end();) {
        if (write(it->first, it->second.data(), it->second.size())) {
            it = originals_.erase(it);
        } else {
            Logger::instance().error("restoreAll failed for one patch");
            ++it;
        }
    }
}

}  // namespace er
