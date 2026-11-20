# Lenovo/ELAN proprietary TOD plugin for the ELAN 04f3:0c4b (press) fingerprint
# sensor, with a private OpenSSL 1.1 -> system OpenSSL compatibility shim
# and an LED shim that lights the sensor while it waits for a finger.
%global debug_package %{nil}
%global compatdir %{_libdir}/libfprint-2-tod1-elan
# Keep the shim private: don't advertise libcrypto.so.1.1 to the rest of the system
%global __provides_exclude_from ^%{compatdir}/.*$
%global __requires_exclude ^(libcrypto\\.so\\.1\\.1|libefd-led\\.so).*$

Name:           libfprint-2-tod1-elan
Version:        0.0.1
Release:        2%{?dist}
Summary:        Proprietary ELAN TOD fingerprint driver (04f3:0c42, 04f3:0c4b)
License:        Proprietary
URL:            https://github.com/TonyHoyle/libfprint-2-tod1-elan
# Blob sha256 34087826dee386a2919e8b0cd2663d1c249be1f8f1a3951bc1f2c88038d30605
# (upstream commit f2aabc25b1cffa28b69bebefeccf772910c51a84)
Source0:        libfprint-2-tod1-elan.so
Source1:        crypto11-shim.c
Source2:        crypto11-shim.map
Source3:        efd-led-wrap.c
ExclusiveArch:  x86_64
BuildRequires:  gcc
BuildRequires:  openssl-devel
BuildRequires:  patchelf
BuildRequires:  pkgconfig(gusb)
BuildRequires:  pkgconfig(libusb-1.0)
Requires:       libfprint-tod%{?_isa}
Requires:       fprintd

%description
Lenovo's closed-source libfprint-tod plugin for ELAN press-type fingerprint
sensors 04f3:0c42 and 04f3:0c4b (e.g. ThinkBook 14 G2 ARE), packaged for
Fedora. The plugin links against OpenSSL 1.1; a minimal private shim forwards
the nine EVP functions it uses to the system OpenSSL. A second shim turns on
the sensor LED before each wait-for-finger, as the open-source elan driver does.

%prep
%setup -q -c -T
cp -p %{SOURCE0} %{SOURCE1} %{SOURCE2} %{SOURCE3} .
echo "34087826dee386a2919e8b0cd2663d1c249be1f8f1a3951bc1f2c88038d30605  libfprint-2-tod1-elan.so" | sha256sum -c

%build
gcc %{optflags} -fPIC -shared -Wl,-soname,libcrypto.so.1.1 \
    -Wl,--version-script=crypto11-shim.map %{build_ldflags} \
    -o libcrypto.so.1.1 crypto11-shim.c -l:libcrypto.so.4
gcc %{optflags} -fPIC -shared -Wl,-soname,libefd-led.so %{build_ldflags} \
    $(pkg-config --cflags gusb libusb-1.0) -o libefd-led.so efd-led-wrap.c \
    $(pkg-config --libs gusb libusb-1.0)
cat > rename.map <<'EOM'
g_usb_device_bulk_transfer_async efdwrap_g_usb_device_bulk_transfer_async
libusb_submit_transfer efdwrap_libusb_submit_transfer
EOM
patchelf --rename-dynamic-symbols rename.map --add-needed libefd-led.so \
    --set-rpath %{compatdir} libfprint-2-tod1-elan.so

%install
install -D -m 0755 libfprint-2-tod1-elan.so %{buildroot}%{_libdir}/libfprint-2/tod-1/libfprint-2-tod1-elan.so
install -D -m 0755 libcrypto.so.1.1 %{buildroot}%{compatdir}/libcrypto.so.1.1
install -D -m 0755 libefd-led.so %{buildroot}%{compatdir}/libefd-led.so

%files
%{_libdir}/libfprint-2/tod-1/libfprint-2-tod1-elan.so
%dir %{compatdir}
%{compatdir}/libcrypto.so.1.1
%{compatdir}/libefd-led.so

%changelog
* Fri Nov 20 2026 George MacKerron <george.mackerron@gmail.com> - 0.0.1-2
- Add LED shim: light the sensor while waiting for a finger

* Fri Nov 20 2026 George MacKerron <george.mackerron@gmail.com> - 0.0.1-1
- Initial local package
