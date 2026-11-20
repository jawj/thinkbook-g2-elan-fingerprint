# ELAN 04f3:0c4b fingerprint reader on Fedora (ThinkBook 14 G2 ARE)

Set up 2026-11-20 on Fedora 45.

## Why this exists

Stock libfprint's `elan` driver handles 04f3:0c4b as a *swipe* sensor, but it is
a *press* sensor, so stock fprintd enrols garbage and never matches. Lenovo's
proprietary libfprint-tod plugin drives it correctly as a press sensor.

## What's installed

| Package | Source | Purpose |
|---|---|---|
| `libfprint-tod` (+ `-selinux`) | COPR `grahamwhiteuk/libfprint-tod` | libfprint fork that loads vendor plugins; replaces Fedora's `libfprint` |
| `libfprint-2-tod1-elan` | built locally from `src/` (RPMs in `rpms/`) | the Lenovo blob plus two private shims |

The local package contains:

- `/usr/lib64/libfprint-2/tod-1/libfprint-2-tod1-elan.so`: Lenovo blob from
  <https://github.com/TonyHoyle/libfprint-2-tod1-elan> (commit f2aabc25, from
  Lenovo's E14 Gen 4 Ubuntu driver). sha256 `34087826…8d30605`, checked at build.
  It is patched with patchelf (RUNPATH, two renamed imports, one added NEEDED).
- `/usr/lib64/libfprint-2-tod1-elan/libcrypto.so.1.1`: shim (`src/crypto11-shim.c`).
  The blob links against OpenSSL 1.1, which Fedora no longer ships, but it only uses
  9 EVP functions (AES-256-CBC, SHA-256, EVP_BytesToKey). The shim exports
  those under the `OPENSSL_1_1_0` version and forwards them to the system OpenSSL.
  It is visible only to the blob, through RUNPATH.
- `/usr/lib64/libfprint-2-tod1-elan/libefd-led.so`: shim (`src/efd-led-wrap.c`).
  The blob never turns on the sensor LED. The shim sends LED-on (`40 31`)
  before each wait-for-finger (`40 3f`), as the open-source driver does. The blob's
  own stop command (`00 0b`) turns it off again.

Blob inspection showed no network, exec, file or dlopen calls. It uses SysV shm
and named semaphores, which the `libfprint-tod-selinux` policy allows.

## Rebuilding (for example after an OpenSSL soname bump)

```
sudo dnf install gcc openssl-devel rpm-build patchelf libgusb-devel libusb1-devel
rpmbuild -bb --define "_topdir $PWD/rpmbuild" --define "_sourcedir $PWD/src" src/libfprint-2-tod1-elan.spec
sudo dnf install rpmbuild/RPMS/x86_64/libfprint-2-tod1-elan-*.rpm
```

## Checks

```
fprintd-list $USER           # should say "ELAN Fingerprint Sensor (press)"
fprintd-verify
journalctl -u fprintd -b
sudo ausearch -m avc -ts today | grep fprint
```

## Known issues and fixes

- **"Device disabled to prevent overheating"** (reported by others; persists across
  fprintd restarts): reset the USB device, or suspend and resume.
- **Enrolments are driver-specific.** Prints enrolled with the stock driver don't
  work with this one, and vice versa. Re-enrol after switching.
- The reader re-enumerates on every S3 resume. That's normal, and fprintd handles it.
- If a Fedora `fprintd` update ever needs a newer libfprint than the COPR provides,
  dnf will report a conflict rather than silently break. Wait for the COPR to catch up.

## Rollback to stock Fedora

```
fprintd-delete $USER
sudo dnf remove libfprint-2-tod1-elan
sudo dnf swap libfprint-tod libfprint     # or: sudo dnf install rpms/libfprint-1.94.100-1.fc45.x86_64.rpm
sudo dnf copr disable grahamwhiteuk/libfprint-tod
```
