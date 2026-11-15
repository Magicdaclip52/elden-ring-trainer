#pragma once

#include "core/Types.h"
#include "game/GameVersion.h"
#include "memory/ProcessMemory.h"
#include "memory/SignatureScanner.h"

#include <mutex>
#include <string>

namespace er {

struct AttachSnapshot {
    bool attached = false;
    DWORD pid = 0;
    uptr base = 0;
    usize moduleSize = 0;
    uptr textBase = 0;
    usize textSize = 0;
    std::string path;
    GameVersion version;
    bool eacLoaded = false;
    std::string status;
    std::string textPreviewHex;
};

class GameAttach {
public:
    GameAttach();

    void tick();
    void detach();

    AttachSnapshot snapshot() const;
    ProcessMemory& memory() { return memory_; }
    SignatureScanner& scanner() { return scanner_; }

private:
    bool findProcess(DWORD& pidOut);
    bool readModuleInfo();
    bool readTextSection();
    bool detectEac();
    void enableDebugPrivilege();

    ProcessMemory memory_;
    SignatureScanner scanner_;
    AttachSnapshot state_{};
    mutable std::mutex mutex_;
    bool privilegeTried_ = false;
};

}  // namespace er
