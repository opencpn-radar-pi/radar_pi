Name: opencpn
Summary: Chartplotter and GPS navigation software
Version: 4.8.0
Release: 4.1%{?dist}
License: GPLv2+

BuildRequires: bzip2-devel
BuildRequires: cmake
BuildRequires: desktop-file-utils
BuildRequires: elfutils-libelf-devel
BuildRequires: expat-devel
BuildRequires: gcc-c++
BuildRequires: gettext
BuildRequires: libcurl-devel
BuildRequires: mesa-libGL-devel
BuildRequires: mesa-libGLU-devel
BuildRequires: portaudio-devel
BuildRequires: redhat-lsb-core
BuildRequires: tar
BuildRequires: tinyxml-devel
BuildRequires: compat-wxGTK3-gtk2-devel
BuildRequires: xz-devel
BuildRequires: xz-lzma-compat

%description
Empty package to catch build dependecies for OpenCPN

%prep

%build

%install

%changelog
* Sun Apr 28 2019 Alec Leamas <leamas.alec@gmail.com> - 4.8.0-4.1
- rebuilt


