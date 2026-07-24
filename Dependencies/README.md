# Dependencies

This folder is reserved for local dependency checkouts that should not be committed.

Expected local layout:

```text
Dependencies/
  vcpkg/      Local vcpkg clone, ignored by Git
```

Bootstrap from the repository root:

```powershell
git clone https://github.com/microsoft/vcpkg.git Dependencies/vcpkg
.\Dependencies\vcpkg\bootstrap-vcpkg.bat
```
