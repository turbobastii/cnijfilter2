%define VERSION 6.30
%define RELEASE 1

%ifarch x86_64
%define	_machine_type "MACHINETYPE=x86_64"
%define	_cflags "CFLAGS=-m64 -march=x86-64"
%define	_arc _x86_64
%define _com_libdir /usr/lib64
%else
%ifarch aarch64
%define	_machine_type "MACHINETYPE=aarch64"
%define _cflags ""
%define	_arc _aarch64
%define _com_libdir /usr/lib64
%else
%ifarch mips64el
%define	_machine_type "MACHINETYPE=mips64el"
%define _cflags ""
%define	_arc _mips64
%define _com_libdir /usr/lib64
%else
%define	_machine_type "MACHINETYPE=i686"
%define	_cflags "CFLAGS=-m32 -march=i686"
%define	_arc _i686
%define _com_libdir /usr/lib
%endif
%endif
%endif

%define _prefix	/usr
%define _bindir %{_prefix}/bin
%define _ppddir /usr
%define _libdir /usr/lib

%define COM_LIBS libcnnet2 libcnbpcnclapicom2 libcnbpnet20 libcnbpnet30
%define CNLIBS /bjlib2

Summary: IJ Printer Driver Ver.%{VERSION} for Linux
Name: cnijfilter2
Version: %{VERSION}
Release: %{RELEASE}
License: See the LICENSE*.txt file.
Vendor: CANON INC.
Group: Applications/Publishing
Source0: cnijfilter2-source-%{version}-%{release}.tar.gz
BuildRequires: cups-devel
Requires:  cups


%description
IJ Printer Driver for Linux. 
This IJ Printer Driver provides printing functions for Canon Inkjet
printers operating under the CUPS (Common UNIX Printing System) environment.


%prep
%setup -q -n  cnijfilter2-source-%{version}-%{release}


%build
if [ %{nobuild} != '1' ]; then
	cd cmdtocanonij2
		./autogen.sh %{_machine_type} %{_cflags} --prefix=/usr --libdir=%{_libdir} --datadir=%{_prefix}/share LDFLAGS="-L../../com/libs_bin%{_arc}"
		make
	cd -

	cd cmdtocanonij3
		./autogen.sh %{_machine_type} %{_cflags} --prefix=/usr --libdir=%{_libdir} --datadir=%{_prefix}/share LDFLAGS="-L../../com/libs_bin%{_arc}"
		make
	cd -

	cd cnijbe2
		./autogen.sh %{_machine_type} %{_cflags} --prefix=/usr --libdir=%{_libdir} --enable-progpath=%{_bindir} 
		make
	cd -

	cd lgmon3
		./autogen.sh %{_machine_type} %{_cflags} --prefix=%{_prefix} --enable-libpath=%{_libdir}%{CNLIBS} --enable-progpath=%{_bindir} --datadir=%{_prefix}/share LDFLAGS="-L../../com/libs_bin%{_arc}"
		make
	cd -

	cd rastertocanonij
		./autogen.sh %{_machine_type} %{_cflags} --prefix=/usr --libdir=%{_libdir} --enable-progpath=%{_bindir}
		make
	cd -

	cd tocanonij
		./autogen.sh %{_machine_type} %{_cflags} --prefix=%{_prefix}
		make
	cd -

	cd tocnpwg
		./autogen.sh %{_machine_type} %{_cflags} --prefix=%{_prefix}
		make
	cd -
fi


%install
mkdir -p ${RPM_BUILD_ROOT}%{_com_libdir}
mkdir -p ${RPM_BUILD_ROOT}%{_libdir}%{CNLIBS}
mkdir -p ${RPM_BUILD_ROOT}%{_bindir}
mkdir -p ${RPM_BUILD_ROOT}%{_libdir}/cups/filter
mkdir -p ${RPM_BUILD_ROOT}%{_libdir}/cups/backend
mkdir -p ${RPM_BUILD_ROOT}%{_ppddir}/share/cups/model
mkdir -p ${RPM_BUILD_ROOT}%{_prefix}/share/cnijlgmon3
mkdir -p ${RPM_BUILD_ROOT}%{_prefix}/share/cmdtocanonij2
mkdir -p ${RPM_BUILD_ROOT}%{_prefix}/share/cmdtocanonij3
mkdir -p ${RPM_BUILD_ROOT}%{_prefix}/share/locale/de/LC_MESSAGES
mkdir -p ${RPM_BUILD_ROOT}%{_prefix}/share/locale/fr/LC_MESSAGES
mkdir -p ${RPM_BUILD_ROOT}%{_prefix}/share/locale/ja/LC_MESSAGES
mkdir -p ${RPM_BUILD_ROOT}%{_prefix}/share/locale/zh/LC_MESSAGES

install -c -m 644 com/ini/cnnet.ini ${RPM_BUILD_ROOT}%{_libdir}%{CNLIBS}
install -c -m 755 com/libs_bin%{_arc}/*.so.* ${RPM_BUILD_ROOT}%{_com_libdir}

install -m 644 ppd/*.ppd ${RPM_BUILD_ROOT}%{_ppddir}/share/cups/model

if [ %{nobuild} != '1' ]; then
	cd cmdtocanonij2
		make install DESTDIR=${RPM_BUILD_ROOT}
	cd -

	cd cmdtocanonij3
		make install DESTDIR=${RPM_BUILD_ROOT}
	cd -

	cd cnijbe2
		make install DESTDIR=${RPM_BUILD_ROOT}
	cd -

	cd lgmon3
		make install DESTDIR=${RPM_BUILD_ROOT}
	cd -

	cd rastertocanonij
		make install DESTDIR=${RPM_BUILD_ROOT}
	cd -

	cd tocanonij
		make install DESTDIR=${RPM_BUILD_ROOT}
	cd -

	cd tocnpwg
		make install DESTDIR=${RPM_BUILD_ROOT}
	cd -
else
	install -c -m 755 com/libs_bin%{_arc}/cmdtocanonij2 ${RPM_BUILD_ROOT}%{_libdir}/cups/filter
	install -c -m 755 com/libs_bin%{_arc}/cmdtocanonij3 ${RPM_BUILD_ROOT}%{_libdir}/cups/filter
	install -c -m 755 com/libs_bin%{_arc}/rastertocanonij ${RPM_BUILD_ROOT}%{_libdir}/cups/filter
	install -c -m 755 com/libs_bin%{_arc}/cnijbe2 ${RPM_BUILD_ROOT}%{_libdir}/cups/backend
	install -c -m 755 com/libs_bin%{_arc}/tocanonij ${RPM_BUILD_ROOT}%{_bindir}
	install -c -m 755 com/libs_bin%{_arc}/tocnpwg ${RPM_BUILD_ROOT}%{_bindir}
	install -c -m 755 com/libs_bin%{_arc}/cnijlgmon3 ${RPM_BUILD_ROOT}%{_bindir}
	install -c -m 644 cmdtocanonij2/utilfiles/*.utl ${RPM_BUILD_ROOT}%{_prefix}/share/cmdtocanonij2
	install -c -m 644 cmdtocanonij3/utilfiles/*.utl ${RPM_BUILD_ROOT}%{_prefix}/share/cmdtocanonij3
	install -c -m 644 lgmon3/keytext/*.res ${RPM_BUILD_ROOT}%{_prefix}/share/cnijlgmon3
	install -c -m 644 com/libs_bin%{_arc}/de/LC_MESSAGES/cnijlgmon3.mo ${RPM_BUILD_ROOT}%{_prefix}/share/locale/de/LC_MESSAGES
	install -c -m 644 com/libs_bin%{_arc}/fr/LC_MESSAGES/cnijlgmon3.mo ${RPM_BUILD_ROOT}%{_prefix}/share/locale/fr/LC_MESSAGES
	install -c -m 644 com/libs_bin%{_arc}/ja/LC_MESSAGES/cnijlgmon3.mo ${RPM_BUILD_ROOT}%{_prefix}/share/locale/ja/LC_MESSAGES
	install -c -m 644 com/libs_bin%{_arc}/zh/LC_MESSAGES/cnijlgmon3.mo ${RPM_BUILD_ROOT}%{_prefix}/share/locale/zh/LC_MESSAGES
fi

%clean
rm -rf $RPM_BUILD_ROOT


%post
if [ -x /sbin/ldconfig ]; then
	/sbin/ldconfig
fi

%postun
for LIBS in %{COM_LIBS}
do
	if [ -h %{_com_libdir}/${LIBS}.so ]; then
		rm -f %{_com_libdir}/${LIBS}.so
	fi	
done
if [ "$1" = 0 ] ; then
	rmdir -p --ignore-fail-on-non-empty %{_libdir}%{CNLIBS}
fi
if [ -x /sbin/ldconfig ]; then
	/sbin/ldconfig
fi


%files
%defattr(-,root,root)
%{_ppddir}/share/cups/model/canon*.ppd
%{_libdir}/cups/filter/cmdtocanonij2
%{_libdir}/cups/filter/cmdtocanonij3
%{_libdir}/cups/filter/rastertocanonij
%{_libdir}/cups/backend/cnijbe2
%{_bindir}/tocanonij
%{_bindir}/tocnpwg
%{_bindir}/cnijlgmon3
%{_com_libdir}/libcnbpcnclapicom2.so*
%{_com_libdir}/libcnnet2.so*
%{_com_libdir}/libcnbpnet20.so*
%{_com_libdir}/libcnbpnet30.so*
%attr(644, lp, lp) %{_libdir}%{CNLIBS}/cnnet.ini
%{_prefix}/share/locale/*/LC_MESSAGES/cnijlgmon3.mo
%{_prefix}/share/cnijlgmon3/*
%{_prefix}/share/cmdtocanonij2/*
%{_prefix}/share/cmdtocanonij3/*

%doc doc/LICENSE-cnijfilter-%{VERSION}JP.txt
%doc doc/LICENSE-cnijfilter-%{VERSION}EN.txt
%doc doc/LICENSE-cnijfilter-%{VERSION}SC.txt
%doc doc/LICENSE-cnijfilter-%{VERSION}FR.txt

%doc lproptions/lproptions-*.txt

%changeLog
