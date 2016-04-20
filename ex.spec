#
# Sccsid @(#)ex.spec	1.8 (gritter) 7/12/05
#
Summary: A port of the traditional ex/vi editors
Name: ex
Version: 040420
Release: 1
License: BSD
Source: %{name}-%{version}.tar.bz2
Group: System Environment/Base
Vendor: Gunnar Ritter <gunnarr@acm.org>
URL: <http://ex-vi.sourceforge.net>
BuildRoot: %{_tmppath}/%{name}-root

Requires: /etc/termcap

# prefix applies to bindir, libexecdir, and mandir.
%define	prefix		/usr
%define	bindir		%{prefix}/5bin
%define	libexecdir	%{prefix}/5lib
%define	mandir		%{prefix}/share/man/5man

%define	preservedir	/var/preserve

# install command
%define	ucbinstall	install

%define	cflags		-Os -fomit-frame-pointer

%define	makeflags	PREFIX=%{prefix} BINDIR=%{bindir} LIBEXECDIR=%{libexecdir} MANDIR=%{mandir} PRESERVEDIR=%{preservedir} INSTALL=%{ucbinstall} RPMCFLAGS="%{cflags}"

%description
This is a port of the traditional ex and vi editor implementation as
found on 2BSD and 4BSD. It was enhanced to support most of the additions
in System V and POSIX.2, and international character sets like UTF-8 and
many East Asian encodings.

%prep
rm -rf %{buildroot}
%setup

%build
make %{makeflags}

%install
make DESTDIR=%{buildroot} %{makeflags} install

%clean
cd ..; rm -rf %{_builddir}/%{name}-%{version}
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%doc Changes LICENSE README TODO
%{bindir}/*
%{libexecdir}/*
%{mandir}/man1/*
%{preservedir}
