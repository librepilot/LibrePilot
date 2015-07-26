
%global gitrev g07deb66

Name:           LibrePilot
Summary:        Ground Control Station
Version:        0.0
Release:        1.git%{gitrev}%{?dist}

Group:          Applications/Scientific
License:        GPLv3+
URL:            http://forum.librepilot.org/

Source0:        https://github.com/librepilot/%{name}/archive/%{name}-%{version}-%{gitrev}.tar.gz

BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)


BuildRequires:  libusbx-devel
BuildRequires:  mesa-libGL-devel
BuildRequires:  qt5-qtbase-devel
BuildRequires:  qt5-qtdeclarative-devel
BuildRequires:  qt5-qtmultimedia-devel
BuildRequires:  qt5-qtquick1-devel
BuildRequires:  qt5-qtscript-devel
BuildRequires:  qt5-qtserialport-devel
BuildRequires:  qt5-qtsvg-devel
BuildRequires:  qt5-qttools-devel
BuildRequires:  dwz
BuildRequires:  pkgconfig
BuildRequires:  python
BuildRequires:  SDL-devel
BuildRequires:  systemd-devel

Requires:       libusbx
Requires:       SDL
Requires:       qt5-qtdeclarative
Requires:       qt5-qtmultimedia
Requires:       qt5-qtscript
Requires:       qt5-qtserialport
Requires:       qt5-qtsvg


%description
LibrePilot is a next-generation Open Source UAV autopilot created by the
LibrePilot Community. It is a highly capable platform for
multirotors, helicopters, fixed wing aircraft, and other vehicles.
It has been designed from the ground up by a community of passionate developers
from around the globe, with its core design principles being quality, safety,
and ease of use. 


%prep
%setup -q -n %{name}-%{version}-%{gitrev}


%build
make %{?_smp_mflags} gcs QMAKE=qmake-qt5 CC=%{__cc} CXX=%{__cxx} libbasename=%{_lib}
#make -j1 opfw_resource


%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT \
    prefix=%{_prefix} \
    libbasename=%{_lib} \
    enable-udev-rules=yes \
    udevrulesdir=%{_udevrulesdir}


%clean
rm -rf $RPM_BUILD_ROOT


%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig


%files
%doc README.txt GPLv3.txt
%{_bindir}/librepilot-gcs
%{_udevrulesdir}/45-librepilot.rules
%{_datadir}/applications/librepilot.desktop
%{_datadir}/librepilot-gcs//*
%{_datadir}/pixmaps/librepilot.png
%{_libdir}/librepilot-gcs/plugins/OpenPilot/*.pluginspec
%{_libdir}/librepilot-gcs/plugins/OpenPilot/*.so
#
%{_libdir}/librepilot-gcs/libAggregation.so.1*
%{_libdir}/librepilot-gcs/libExtensionSystem.so.1*
%{_libdir}/librepilot-gcs/libGLC_lib.so.1*
%{_libdir}/librepilot-gcs/libopmapwidget.so.1*
%{_libdir}/librepilot-gcs/libQScienceSpinBox.so.1*
%{_libdir}/librepilot-gcs/libQtConcurrent.so.1*
%{_libdir}/librepilot-gcs/libQwt.so.1*
%{_libdir}/librepilot-gcs/libsdlgamepad.so.1*
%{_libdir}/librepilot-gcs/libUtils.so.1*
%{_libdir}/librepilot-gcs/libVersionInfo.so.1*
#
%{_libdir}/librepilot-gcs/lib*.so


%changelog
