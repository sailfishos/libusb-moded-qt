Name:       libusb-moded-qt5

Summary:    A library of Qt5 bindings for usb_moded
Version:    1.8
Release:    1
License:    BSD
URL:        https://github.com/sailfishos/libusb-moded-qt
Source0:    %{name}-%{version}.tar.bz2

Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
Requires:       usb-moded >= 0.86.0+mer39
BuildRequires:  usb-moded-devel >= 0.86.0+mer39
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5DBus)
BuildRequires:  pkgconfig(usb_moded)

%{!?qtc_qmake5:%define qtc_qmake5 %qmake5}
%{!?qtc_make:%define qtc_make make}

%description
This package contains Qt bindings for usb_moded

%package devel
Summary:    Development files for usb_moded Qt bindings
Requires:   %{name} = %{version}-%{release}

%description devel
This package contains the development header files for usb_moded Qt bindings.

%prep
%setup -q -n %{name}-%{version}

%build
%qtc_qmake5 "VERSION=%{version}"
%qtc_make %{?_smp_mflags}

%install
%qmake5_install

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%license LICENSE.BSD
%{_libdir}/%{name}.so.*

%files devel
%{_libdir}/pkgconfig/usb-moded-qt5.pc
%{_libdir}/%{name}.so
%{_includedir}/usb-moded-qt5/*.h
