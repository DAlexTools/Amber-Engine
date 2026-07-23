# External Dependencies

This folder is reserved for local dependency checkouts that should not be committed.

Expected local layout:

```text
external/
  vcpkg/      Local vcpkg clone, ignored by Git
```

Bootstrap from the repository root:

```powershell
git clone https://github.com/microsoft/vcpkg.git external/vcpkg
.\external\vcpkg\bootstrap-vcpkg.bat
```
